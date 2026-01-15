#include "events.h"
#include "backends/m8.h"
#include "common.h"
#include "gamepads.h"
#include "input.h"
#include "render.h"
#include "settings.h"
#include "sdl_compat.h"

// Event handler - shared between SDL2 and SDL3
static SDL_AppResult process_event(struct app_context *ctx, SDL_Event *event) {
  SDL_AppResult ret_val = SDL_APP_CONTINUE;

  switch (event->type) {

  // --- System events ---
  case SDL_EVENT_QUIT:
  case SDL_EVENT_TERMINATING:
    ret_val = SDL_APP_SUCCESS;
    break;

#ifdef USE_SDL2
  // SDL2: Window events have subtypes
  case SDL_WINDOWEVENT:
    if (event->window.event == SDL_WINDOWEVENT_RESIZED ||
        event->window.event == SDL_WINDOWEVENT_MOVED) {
      renderer_fix_texture_scaling_after_window_resize(&ctx->conf);
    }
    break;
#else
  // SDL3: Direct event types
  case SDL_EVENT_WINDOW_RESIZED:
  case SDL_EVENT_WINDOW_MOVED:
    renderer_fix_texture_scaling_after_window_resize(&ctx->conf);
    break;
#endif

  // --- iOS specific events ---
  case SDL_EVENT_DID_ENTER_BACKGROUND:
    SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Received SDL_EVENT_DID_ENTER_BACKGROUND");
    ctx->app_suspended = 1;
    if (ctx->device_connected)
      m8_pause_processing();
    break;
  case SDL_EVENT_WILL_ENTER_BACKGROUND:
    break;
  case SDL_EVENT_WILL_ENTER_FOREGROUND:
    SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Received SDL_EVENT_WILL_ENTER_FOREGROUND");
    break;
  case SDL_EVENT_DID_ENTER_FOREGROUND:
    SDL_LogDebug(SDL_LOG_CATEGORY_SYSTEM, "Received SDL_EVENT_DID_ENTER_FOREGROUND");
    ctx->app_suspended = 0;
    if (ctx->device_connected) {
      renderer_clear_screen();
      m8_resume_processing();
    }
    break;

  // --- Input events ---
  case SDL_EVENT_GAMEPAD_ADDED:
  case SDL_EVENT_GAMEPAD_REMOVED:
    gamepads_initialize();
    break;

  case SDL_EVENT_KEY_DOWN:
    // Toggle settings with Command/Win+comma
    if (COMPAT_KEY_SYM(event) == SDLK_COMMA && COMPAT_KEY_REPEAT(event) == 0 &&
        (COMPAT_KEY_MOD(event) & KMOD_GUI)) {
      settings_toggle_open();
      return ret_val;
    }
    // Toggle settings with config defined key
    if (COMPAT_KEY_SCANCODE(event) == ctx->conf.key_toggle_settings &&
        COMPAT_KEY_REPEAT(event) == 0) {
      settings_toggle_open();
      return ret_val;
    }
    if (settings_is_open()) {
      settings_handle_event(ctx, event);
      return ret_val;
    }
    input_handle_key_down_event(ctx, event);
    break;

  case SDL_EVENT_KEY_UP:
    if (settings_is_open()) {
      settings_handle_event(ctx, event);
      return ret_val;
    }
    input_handle_key_up_event(ctx, event);
    break;

  case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    if (settings_is_open()) {
      settings_handle_event(ctx, event);
      return ret_val;
    }

    if (COMPAT_GBUTTON_BUTTON(event) == SDL_GAMEPAD_BUTTON_BACK) {
      if (ctx->app_state == WAIT_FOR_DEVICE && !settings_is_open()) {
        settings_toggle_open();
      }
    }

    input_handle_gamepad_button(ctx, COMPAT_GBUTTON_BUTTON(event), true);
    break;

  case SDL_EVENT_GAMEPAD_BUTTON_UP:
    if (settings_is_open()) {
      settings_handle_event(ctx, event);
      return ret_val;
    }
    input_handle_gamepad_button(ctx, COMPAT_GBUTTON_BUTTON(event), false);
    break;

  case SDL_EVENT_GAMEPAD_AXIS_MOTION:
    if (settings_is_open()) {
      settings_handle_event(ctx, event);
      return ret_val;
    }
    input_handle_gamepad_axis(ctx, COMPAT_GAXIS_AXIS(event), COMPAT_GAXIS_VALUE(event));
    break;

  default:
    break;
  }
  return ret_val;
}

#ifdef USE_SDL2
// SDL2: Exported function for main_sdl2.c to call
SDL_AppResult handle_event(struct app_context *ctx, SDL_Event *event) {
  return process_event(ctx, event);
}
#else
// SDL3: Callback-based event handler
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
  struct app_context *ctx = appstate;
  return process_event(ctx, event);
}
#endif
