// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT
// SDL2 entry point - traditional main() with event loop

#ifdef USE_SDL2

#define APP_VERSION "v2.2.3"

#include "sdl_compat.h"
#include <stdlib.h>

#include "SDL2_inprint.h"
#include "backends/audio.h"
#include "backends/m8.h"
#include "common.h"
#include "config.h"
#include "gamepads.h"
#include "log_overlay.h"
#include "render.h"

// Forward declarations for event handling (defined in events.c)
SDL_AppResult handle_event(struct app_context *ctx, SDL_Event *event);

static void do_wait_for_device(struct app_context *ctx) {
  static Uint32 ticks_poll_device = 0;
  static int screensaver_initialized = 0;

  // Handle app suspension
  if (ctx->app_suspended) {
    return;
  }

  if (!screensaver_initialized) {
    screensaver_initialized = screensaver_init();
  }
  screensaver_draw();
  render_screen(&ctx->conf);

  // Poll for M8 device every second
  if (ctx->device_connected == 0 && SDL_GetTicks() - ticks_poll_device > 1000) {
    ticks_poll_device = SDL_GetTicks();
    if (m8_initialize(0, ctx->preferred_device)) {

      if (ctx->conf.audio_enabled) {
        if (!audio_initialize(ctx->conf.audio_device_name, ctx->conf.audio_buffer_size)) {
          SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Cannot initialize audio");
          ctx->conf.audio_enabled = 0;
        }
      }

      const int m8_enabled = m8_enable_display(1);
      // Device was found; enable display and proceed to the main loop
      if (m8_enabled == 1) {
        ctx->app_state = RUN;
        ctx->device_connected = 1;
        SDL_Delay(100); // Give the display time to initialize
        screensaver_destroy();
        screensaver_initialized = 0;
        m8_reset_display(); // Avoid display glitches.
      } else {
        SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Device not detected.");
        ctx->app_state = QUIT;
        screensaver_destroy();
        screensaver_initialized = 0;
#ifdef USE_RTMIDI
        show_error_message("Cannot initialize M8 remote display. Make sure you're running "
                           "firmware 6.0.0 or newer. Please close and restart the application to "
                           "try again.");
#endif
      }
    }
  }
}

static config_params_s initialize_config(int argc, char *argv[], char **preferred_device,
                                         char **config_filename) {
  for (int i = 1; i < argc; i++) {
    if (SDL_strcmp(argv[i], "--list") == 0) {
      exit(m8_list_devices());
    }
    if (SDL_strcmp(argv[i], "--dev") == 0 && i + 1 < argc) {
      *preferred_device = argv[i + 1];
      SDL_Log("Using preferred device: %s", *preferred_device);
      i++;
    } else if (SDL_strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
      *config_filename = argv[i + 1];
      SDL_Log("Using config file: %s", *config_filename);
      i++;
    }
  }

  config_params_s conf = config_initialize(*config_filename);

  if (TARGET_OS_IOS == 1) {
    // Predefined settings for iOS
    conf.init_fullscreen = 1;
  }
  config_read(&conf);

  return conf;
}

// Main iteration - process data and render
static SDL_AppResult app_iterate(struct app_context *ctx) {
  if (ctx == NULL) {
    return SDL_APP_FAILURE;
  }

  SDL_AppResult app_result = SDL_APP_CONTINUE;

  switch (ctx->app_state) {
  case INITIALIZE:
    break;

  case WAIT_FOR_DEVICE:
    do_wait_for_device(ctx);
    break;

  case RUN: {
    const int result = m8_process_data(&ctx->conf);
    if (result == DEVICE_DISCONNECTED) {
      ctx->device_connected = 0;
      ctx->app_state = WAIT_FOR_DEVICE;
      audio_close();
    } else if (result == DEVICE_FATAL_ERROR) {
      return SDL_APP_FAILURE;
    }
    render_screen(&ctx->conf);
    break;
  }

  case QUIT:
    app_result = SDL_APP_SUCCESS;
    break;
  }

  return app_result;
}

// Cleanup
static void app_quit(struct app_context *app) {
  if (app) {
    if (app->app_state == WAIT_FOR_DEVICE) {
      screensaver_destroy();
    }
    if (app->conf.audio_enabled) {
      audio_close();
    }
    gamepads_close();
    renderer_close();
    inline_font_close();
    if (app->device_connected) {
      m8_close();
    }
    SDL_free(app);

    SDL_Log("Shutting down.");
    SDL_Quit();
  }
}

int main(int argc, char *argv[]) {
  char *config_filename = NULL;

  // Initialize in-app log capture/overlay
  log_overlay_init();

#ifndef NDEBUG
  // Show debug messages in the application log
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_DEBUG);
  SDL_LogDebug(SDL_LOG_CATEGORY_TEST, "Running a Debug build");
#else
  SDL_SetLogPriorities(SDL_LOG_PRIORITY_INFO);
#endif

  struct app_context *ctx = SDL_calloc(1, sizeof(struct app_context));
  if (ctx == NULL) {
    SDL_LogCritical(SDL_LOG_CATEGORY_SYSTEM, "SDL_calloc failed: %s", SDL_GetError());
    return 1;
  }

  ctx->app_state = INITIALIZE;
  ctx->conf = initialize_config(argc, argv, &ctx->preferred_device, &config_filename);

  if (!renderer_initialize(&ctx->conf)) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize renderer.");
    SDL_free(ctx);
    return 1;
  }

  ctx->device_connected = m8_initialize(1, ctx->preferred_device);

  if (gamepads_initialize() < 0) {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Failed to initialize game controllers.");
    renderer_close();
    SDL_free(ctx);
    return 1;
  }

  if (ctx->device_connected && m8_enable_display(1)) {
    if (ctx->conf.audio_enabled) {
      audio_initialize(ctx->conf.audio_device_name, ctx->conf.audio_buffer_size);
    }
    ctx->app_state = RUN;
    render_screen(&ctx->conf);
  } else {
    SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "Device not detected.");
    ctx->device_connected = 0;
    ctx->app_state = WAIT_FOR_DEVICE;
  }

  // Main event loop
  SDL_Event event;
  SDL_AppResult result = SDL_APP_CONTINUE;
  Uint32 frame_start;
  const Uint32 target_frame_time = 1000 / 120; // ~120 fps target

  while (result == SDL_APP_CONTINUE) {
    frame_start = SDL_GetTicks();

    // Process all pending events
    while (SDL_PollEvent(&event)) {
      result = handle_event(ctx, &event);
      if (result != SDL_APP_CONTINUE) {
        break;
      }
    }

    if (result == SDL_APP_CONTINUE) {
      // Pump audio data
      audio_pump();

      // Main iteration
      result = app_iterate(ctx);
    }

    // Frame timing - target ~120fps like SDL3 callback
    Uint32 frame_time = SDL_GetTicks() - frame_start;
    if (frame_time < target_frame_time) {
      SDL_Delay(target_frame_time - frame_time);
    }
  }

  app_quit(ctx);
  return (result == SDL_APP_SUCCESS) ? 0 : 1;
}

#endif // USE_SDL2
