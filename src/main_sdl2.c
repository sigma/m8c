// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT
// SDL2 entry point - traditional main() with event loop

#ifdef USE_SDL2

#include "app.h"
#include "backends/audio.h"
#include "sdl_compat.h"

// Forward declaration for event handling (defined in events.c)
SDL_AppResult handle_event(struct app_context *ctx, SDL_Event *event);

int main(int argc, char *argv[]) {
  struct app_context *ctx = app_init(argc, argv);
  if (ctx == NULL) {
    return 1;
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
      // Pump audio data (SDL2 only - routes capture to playback)
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
