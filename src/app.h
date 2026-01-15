// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT
// Common application logic shared between SDL2 and SDL3 entry points

#ifndef APP_H
#define APP_H

#include "sdl_compat.h"
#include "common.h"
#include "config.h"

#define APP_VERSION "v2.2.3"

// Parse command line arguments and initialize configuration
config_params_s app_parse_args(int argc, char *argv[], char **preferred_device,
                               char **config_filename);

// Initialize the application (renderer, gamepads, M8 device)
// Returns the initialized app context, or NULL on failure
struct app_context *app_init(int argc, char *argv[]);

// Main iteration - process data and render
// Returns SDL_APP_CONTINUE, SDL_APP_SUCCESS, or SDL_APP_FAILURE
SDL_AppResult app_iterate(struct app_context *ctx);

// Cleanup and shutdown
void app_quit(struct app_context *app);

#endif // APP_H
