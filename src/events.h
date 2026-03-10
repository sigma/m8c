// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT

#ifndef EVENTS_H_
#define EVENTS_H_

#include "sdl_compat.h"
#include "common.h"

#ifdef USE_SDL2
// SDL2: Function to handle events from main loop
SDL_AppResult handle_event(struct app_context *ctx, SDL_Event *event);
#endif

#endif
