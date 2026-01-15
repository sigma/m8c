// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT
#ifndef USE_LIBUSB
#include "audio.h"
#include "../sdl_compat.h"

#ifdef USE_SDL2
// ============================================================================
// SDL2 Audio Implementation
// Uses ring buffer and callback-based audio
// ============================================================================

#include <string.h>

// Ring buffer for audio routing
#define AUDIO_RING_BUFFER_SIZE (44100 * 2 * 2 * 2) // ~2 seconds stereo 16-bit
static Uint8 *audio_ring_buffer = NULL;
static size_t ring_read_pos = 0;
static size_t ring_write_pos = 0;
static size_t ring_buffer_used = 0;
static SDL_mutex *ring_mutex = NULL;

static SDL_AudioDeviceID audio_dev_out = 0;
static SDL_AudioDeviceID audio_dev_in = 0;
static unsigned int audio_paused = 0;
static unsigned int audio_initialized = 0;

static void SDLCALL audio_callback_sdl2(void *userdata, Uint8 *stream, int len) {
  (void)userdata;

  SDL_LockMutex(ring_mutex);

  size_t available = ring_buffer_used;
  size_t to_copy = (size_t)len < available ? (size_t)len : available;

  if (to_copy > 0) {
    size_t first_part = AUDIO_RING_BUFFER_SIZE - ring_read_pos;
    if (first_part >= to_copy) {
      memcpy(stream, audio_ring_buffer + ring_read_pos, to_copy);
      ring_read_pos = (ring_read_pos + to_copy) % AUDIO_RING_BUFFER_SIZE;
    } else {
      memcpy(stream, audio_ring_buffer + ring_read_pos, first_part);
      memcpy(stream + first_part, audio_ring_buffer, to_copy - first_part);
      ring_read_pos = to_copy - first_part;
    }
    ring_buffer_used -= to_copy;
  }

  // Fill remaining with silence
  if (to_copy < (size_t)len) {
    memset(stream + to_copy, 0, len - to_copy);
  }

  SDL_UnlockMutex(ring_mutex);
}

// Called periodically to capture input audio and push to ring buffer
static void audio_capture_sdl2(void) {
  if (!audio_initialized || audio_paused)
    return;

  Uint8 temp[4096];
  Uint32 available = SDL_GetQueuedAudioSize(audio_dev_in);

  while (available > 0) {
    Uint32 to_read = available > sizeof(temp) ? sizeof(temp) : available;
    Uint32 got = SDL_DequeueAudio(audio_dev_in, temp, to_read);
    if (got == 0)
      break;

    SDL_LockMutex(ring_mutex);

    size_t space = AUDIO_RING_BUFFER_SIZE - ring_buffer_used;
    size_t to_write = got < space ? got : space;

    if (to_write > 0) {
      size_t first_part = AUDIO_RING_BUFFER_SIZE - ring_write_pos;
      if (first_part >= to_write) {
        memcpy(audio_ring_buffer + ring_write_pos, temp, to_write);
        ring_write_pos = (ring_write_pos + to_write) % AUDIO_RING_BUFFER_SIZE;
      } else {
        memcpy(audio_ring_buffer + ring_write_pos, temp, first_part);
        memcpy(audio_ring_buffer, temp + first_part, to_write - first_part);
        ring_write_pos = to_write - first_part;
      }
      ring_buffer_used += to_write;
    }

    SDL_UnlockMutex(ring_mutex);
    available -= got;
  }
}

void audio_toggle(const char *output_device_name, unsigned int audio_buffer_size) {
  if (!audio_initialized) {
    audio_initialize(output_device_name, audio_buffer_size);
    return;
  }
  if (audio_paused) {
    SDL_PauseAudioDevice(audio_dev_out, 0);
    SDL_PauseAudioDevice(audio_dev_in, 0);
  } else {
    SDL_PauseAudioDevice(audio_dev_in, 1);
    SDL_PauseAudioDevice(audio_dev_out, 1);
  }
  audio_paused = !audio_paused;
  SDL_Log(audio_paused ? "Audio paused" : "Audio resumed");
}

int audio_initialize(const char *output_device_name, const unsigned int audio_buffer_size) {
  int m8_device_index = -1;  // Use -1 to indicate "not found"
  int output_device_index = -1;

  if (!SDL_Init(SDL_INIT_AUDIO)) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL Audio init failed: %s", SDL_GetError());
    return 0;
  }

  // Find M8 input device
  int num_capture = SDL_GetNumAudioDevices(1);
  SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "Found %d capture devices", num_capture);

  for (int i = 0; i < num_capture; i++) {
    const char *name = SDL_GetAudioDeviceName(i, 1);
    SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "Capture device %d: %s", i, name);
    if (name && SDL_strstr(name, "M8") != NULL) {
      SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "M8 Audio Input device found: %s", name);
      m8_device_index = i;
    }
  }

  if (m8_device_index < 0) {
    SDL_Log("Cannot find M8 audio input device");
    return 0;
  }

  // Find output device
  if (output_device_name != NULL) {
    int num_playback = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < num_playback; i++) {
      const char *name = SDL_GetAudioDeviceName(i, 0);
      if (name && SDL_strcasestr(name, output_device_name) != NULL) {
        SDL_Log("Requested output device found: %s", name);
        output_device_index = i;
        break;
      }
    }
  }

  // Allocate ring buffer
  audio_ring_buffer = SDL_malloc(AUDIO_RING_BUFFER_SIZE);
  if (!audio_ring_buffer) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to allocate ring buffer");
    return 0;
  }
  memset(audio_ring_buffer, 0, AUDIO_RING_BUFFER_SIZE);

  ring_mutex = SDL_CreateMutex();
  if (!ring_mutex) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to create mutex");
    SDL_free(audio_ring_buffer);
    return 0;
  }

  // Open output device with callback
  SDL_AudioSpec want_out, have_out;
  SDL_zero(want_out);
  want_out.freq = 44100;
  want_out.format = AUDIO_S16LSB;
  want_out.channels = 2;
  want_out.samples = audio_buffer_size > 0 ? audio_buffer_size : 1024;
  want_out.callback = audio_callback_sdl2;

  const char *out_name = output_device_index >= 0 ? SDL_GetAudioDeviceName(output_device_index, 0) : NULL;
  audio_dev_out = SDL_OpenAudioDevice(out_name, 0, &want_out, &have_out, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

  if (audio_dev_out == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open output device: %s", SDL_GetError());
    SDL_DestroyMutex(ring_mutex);
    SDL_free(audio_ring_buffer);
    return 0;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "Opened output: %dHz, %d channels, %d samples",
              have_out.freq, have_out.channels, have_out.samples);

  // Open input device (M8)
  SDL_AudioSpec want_in, have_in;
  SDL_zero(want_in);
  want_in.freq = 44100;
  want_in.format = AUDIO_S16LSB;
  want_in.channels = 2;
  want_in.samples = audio_buffer_size > 0 ? audio_buffer_size : 1024;
  want_in.callback = NULL; // Use queue-based capture

  const char *m8_name = SDL_GetAudioDeviceName(m8_device_index, 1);
  audio_dev_in = SDL_OpenAudioDevice(m8_name, 1, &want_in, &have_in, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

  if (audio_dev_in == 0) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Failed to open M8 input device: %s", SDL_GetError());
    SDL_CloseAudioDevice(audio_dev_out);
    SDL_DestroyMutex(ring_mutex);
    SDL_free(audio_ring_buffer);
    return 0;
  }

  SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "Opened M8 input: %dHz, %d channels", have_in.freq, have_in.channels);

  // Start audio
  SDL_PauseAudioDevice(audio_dev_out, 0);
  SDL_PauseAudioDevice(audio_dev_in, 0);

  audio_paused = 0;
  audio_initialized = 1;

  return 1;
}

void audio_close(void) {
  if (!audio_initialized)
    return;

  SDL_Log("Closing audio devices");

  if (audio_dev_in)
    SDL_CloseAudioDevice(audio_dev_in);
  if (audio_dev_out)
    SDL_CloseAudioDevice(audio_dev_out);
  if (ring_mutex)
    SDL_DestroyMutex(ring_mutex);
  if (audio_ring_buffer)
    SDL_free(audio_ring_buffer);

  audio_dev_in = 0;
  audio_dev_out = 0;
  ring_mutex = NULL;
  audio_ring_buffer = NULL;
  ring_read_pos = ring_write_pos = ring_buffer_used = 0;

  SDL_QuitSubSystem(SDL_INIT_AUDIO);
  audio_initialized = 0;
}

// Call this from main loop to pump audio data
void audio_pump(void) { audio_capture_sdl2(); }

#else
// ============================================================================
// SDL3 Audio Implementation
// Uses AudioStream API
// ============================================================================

SDL_AudioStream *audio_stream_in, *audio_stream_out;

static unsigned int audio_paused = 0;
static unsigned int audio_initialized = 0;
static SDL_AudioSpec audio_spec_in = {SDL_AUDIO_S16LE, 2, 44100};

static void SDLCALL audio_cb_out(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
  // suppress compiler warnings
  (void)userdata;

  if (additional_amount <= 0) {
    return;
  }

  const int bytes_available = SDL_GetAudioStreamAvailable(audio_stream_in);
  if (bytes_available == -1) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO,
                 "Error getting available audio stream bytes: %s, destroying audio",
                 SDL_GetError());
    audio_close();
    return;
  }

  // Decide how much to feed this time.
  int to_write_goal = additional_amount;
  if (total_amount > 0) {
    const int prefill_cap = additional_amount * 2;
    if (to_write_goal < total_amount) {
      to_write_goal = SDL_min(total_amount, prefill_cap);
    }
  }

  int to_write = to_write_goal;
  Uint8 temp[4096];

  while (to_write > 0) {
    int still_avail = SDL_GetAudioStreamAvailable(audio_stream_in);
    if (still_avail <= 0) {
      break; // nothing more to pull now
    }

    int chunk = still_avail;
    if (chunk > (int)sizeof(temp)) chunk = (int)sizeof(temp);
    if (chunk > to_write) chunk = to_write;

    const int got = SDL_GetAudioStreamData(audio_stream_in, temp, chunk);
    if (got == -1) {
      SDL_LogError(SDL_LOG_CATEGORY_AUDIO,
                   "Error reading audio stream data: %s, destroying audio",
                   SDL_GetError());
      audio_close();
      return;
    }
    if (got == 0) {
      break; // no data currently available
    }

    if (!SDL_PutAudioStreamData(stream, temp, got)) {
      SDL_LogError(SDL_LOG_CATEGORY_AUDIO,
                   "Error putting audio stream data: %s, destroying audio",
                   SDL_GetError());
      audio_close();
      return;
    }

    to_write -= got;
  }
}

void audio_toggle(const char *output_device_name, unsigned int audio_buffer_size) {
  if (!audio_initialized) {
    audio_initialize(output_device_name, audio_buffer_size);
    return;
  }
  if (audio_paused) {
    SDL_ResumeAudioStreamDevice(audio_stream_out);
    SDL_ResumeAudioStreamDevice(audio_stream_in);
  } else {
    SDL_PauseAudioStreamDevice(audio_stream_in);
    SDL_PauseAudioStreamDevice(audio_stream_out);
  }
  audio_paused = !audio_paused;
  SDL_Log(audio_paused ? "Audio paused" : "Audio resumed");
}

int audio_initialize(const char *output_device_name, const unsigned int audio_buffer_size) {

  int num_devices_in, num_devices_out;
  SDL_AudioDeviceID m8_device_id = 0;
  SDL_AudioDeviceID output_device_id = 0;

  if (SDL_Init(SDL_INIT_AUDIO) == false) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "SDL Audio init failed, SDL Error: %s", SDL_GetError());
    return 0;
  }

  SDL_AudioDeviceID *devices_in = SDL_GetAudioRecordingDevices(&num_devices_in);
  if (!devices_in) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "No audio capture devices, SDL Error: %s", SDL_GetError());
    return 0;
  }

  SDL_AudioDeviceID *devices_out = SDL_GetAudioPlaybackDevices(&num_devices_out);
  if (!devices_out) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "No audio playback devices, SDL Error: %s",
                 SDL_GetError());
    return 0;
  }

  SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "Audio input devices:");
  for (int i = 0; i < num_devices_in; i++) {
    const SDL_AudioDeviceID instance_id = devices_in[i];
    const char *device_name = SDL_GetAudioDeviceName(instance_id);
    SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "%s", device_name);
    if (SDL_strstr(device_name, "M8") != NULL) {
      SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "M8 Audio Input device found: %s", device_name);
      m8_device_id = instance_id;
    }
  }

  if (output_device_name != NULL) {
    for (int i = 0; i < num_devices_out; i++) {
      const SDL_AudioDeviceID instance_id = devices_out[i];
      const char *device_name = SDL_GetAudioDeviceName(instance_id);
      SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "%s", device_name);
      if (SDL_strcasestr(device_name, output_device_name) != NULL) {
        SDL_Log("Requested output device found: %s", device_name);
        output_device_id = instance_id;
      }
    }
  }

  SDL_free(devices_in);
  SDL_free(devices_out);

  if (!output_device_id) {
    output_device_id = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
  }

  if (!m8_device_id) {
    // forget about it
    SDL_Log("Cannot find M8 audio input device");
    return 0;
  }

  char audio_buffer_size_str[256];
  SDL_snprintf(audio_buffer_size_str, sizeof(audio_buffer_size_str), "%d", audio_buffer_size);
  if (audio_buffer_size > 0) {
    SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO, "Setting requested audio device sample frames to %d",
                audio_buffer_size);
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, audio_buffer_size_str);
  }

  audio_stream_out = SDL_OpenAudioDeviceStream(output_device_id, NULL, audio_cb_out, NULL);

  SDL_AudioSpec audio_spec_out;
  int audio_out_buffer_size_real, audio_in_buffer_size_real = 0;

  SDL_GetAudioDeviceFormat(output_device_id, &audio_spec_out, &audio_out_buffer_size_real);

  if (!audio_stream_out) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Error opening audio output device: %s", SDL_GetError());
    return 0;
  }
  SDL_LogInfo(SDL_LOG_CATEGORY_AUDIO,
              "Opening audio output: rate %dhz, buffer size: %d frames", audio_spec_out.freq,
              audio_out_buffer_size_real);

  audio_stream_in = SDL_OpenAudioDeviceStream(m8_device_id, &audio_spec_in, NULL, NULL);
  if (!audio_stream_in) {
    SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Error opening audio input device: %s", SDL_GetError());
    SDL_DestroyAudioStream(audio_stream_out);
    return 0;
  }

  SDL_SetAudioStreamFormat(audio_stream_in, &audio_spec_in, &audio_spec_out);
  SDL_GetAudioDeviceFormat(m8_device_id, &audio_spec_in, &audio_in_buffer_size_real);
  SDL_LogDebug(SDL_LOG_CATEGORY_AUDIO, "Audiospec In: format %d, channels %d, rate %d, buffer size %d frames",
               audio_spec_in.format, audio_spec_in.channels, audio_spec_in.freq, audio_in_buffer_size_real);



  SDL_ResumeAudioStreamDevice(audio_stream_out);
  SDL_ResumeAudioStreamDevice(audio_stream_in);

  audio_paused = 0;
  audio_initialized = 1;

  return 1;
}

void audio_close(void) {
  if (!audio_initialized)
    return;
  SDL_Log("Closing audio devices");
  SDL_DestroyAudioStream(audio_stream_in);
  SDL_DestroyAudioStream(audio_stream_out);
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
  audio_initialized = 0;
}

#endif // USE_SDL2

#endif // USE_LIBUSB
