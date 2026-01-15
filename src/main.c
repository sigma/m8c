// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT
// SDL3 entry point - callback-based app lifecycle

#ifndef USE_SDL2

#include "app.h"
#include "sdl_compat.h"
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

// Main callback loop
SDL_AppResult SDL_AppIterate(void *appstate) {
  return app_iterate((struct app_context *)appstate);
}

// Initialize the app
SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {
  SDL_SetAppMetadata("M8C", APP_VERSION, "fi.laamaa.m8c");

  // Process the application's main callback roughly at 120 Hz
  SDL_SetHint(SDL_HINT_MAIN_CALLBACK_RATE, "120");

  struct app_context *ctx = app_init(argc, argv);
  if (ctx == NULL) {
    return SDL_APP_FAILURE;
  }

  *appstate = ctx;
  return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
  (void)result;
  app_quit((struct app_context *)appstate);
}

#endif // USE_SDL2
