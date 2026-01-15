// SDL2/SDL3 Compatibility Header
// Provides unified API for both SDL2 and SDL3
// Copyright 2021 Jonne Kokkonen
// Released under the MIT licence, https://opensource.org/licenses/MIT

#ifndef SDL_COMPAT_H
#define SDL_COMPAT_H

#ifdef USE_SDL2

// SDL2 includes
#include <SDL2/SDL.h>
#include <stdbool.h>

// ============================================================================
// Type definitions
// ============================================================================

// IOStream -> RWops
typedef SDL_RWops SDL_IOStream;

// Gamepad types (SDL2: GameController)
// Note: SDL_Gamepad is just an alias for the struct type, not a pointer
typedef SDL_GameController SDL_Gamepad;
typedef SDL_GameControllerButton SDL_GamepadButton;
typedef SDL_GameControllerAxis SDL_GamepadAxis;
typedef int SDL_GamepadButtonLabel; // SDL2 doesn't have button labels

// Threading types (SDL2 uses lowercase)
typedef SDL_mutex SDL_Mutex;
typedef SDL_cond SDL_Condition;
#define SDL_CreateCondition() SDL_CreateCond()
#define SDL_DestroyCondition(c) SDL_DestroyCond(c)
#define SDL_SignalCondition(c) SDL_CondSignal(c)
#define SDL_WaitCondition(c, m) SDL_CondWait(c, m)

// App result type for main loop compatibility
typedef enum {
  SDL_APP_CONTINUE = 0,
  SDL_APP_SUCCESS = 1,
  SDL_APP_FAILURE = -1
} SDL_AppResult;

// Logical presentation mode (SDL2 doesn't have this enum)
typedef enum {
  SDL_LOGICAL_PRESENTATION_DISABLED,
  SDL_LOGICAL_PRESENTATION_INTEGER_SCALE,
  SDL_LOGICAL_PRESENTATION_STRETCH
} SDL_RendererLogicalPresentation;

// SDL3 uses different ScaleMode type
typedef SDL_ScaleMode SDL_ScaleMode_t;
#define SDL_SCALEMODE_NEAREST SDL_ScaleModeNearest
#define SDL_SCALEMODE_LINEAR SDL_ScaleModeLinear

// ============================================================================
// Event constants
// ============================================================================

#define SDL_EVENT_QUIT SDL_QUIT
#define SDL_EVENT_TERMINATING SDL_APP_TERMINATING
#define SDL_EVENT_KEY_DOWN SDL_KEYDOWN
#define SDL_EVENT_KEY_UP SDL_KEYUP

// Window events - SDL2 uses SDL_WINDOWEVENT with subtypes
#define SDL_EVENT_WINDOW_RESIZED SDL_WINDOWEVENT
#define SDL_EVENT_WINDOW_MOVED SDL_WINDOWEVENT
#define SDL_WINDOW_EVENT_RESIZED SDL_WINDOWEVENT_RESIZED
#define SDL_WINDOW_EVENT_MOVED SDL_WINDOWEVENT_MOVED

// Mobile/iOS events
#define SDL_EVENT_DID_ENTER_BACKGROUND SDL_APP_DIDENTERBACKGROUND
#define SDL_EVENT_WILL_ENTER_BACKGROUND SDL_APP_WILLENTERBACKGROUND
#define SDL_EVENT_WILL_ENTER_FOREGROUND SDL_APP_WILLENTERFOREGROUND
#define SDL_EVENT_DID_ENTER_FOREGROUND SDL_APP_DIDENTERFOREGROUND

// Gamepad events (SDL2: Controller events)
#define SDL_EVENT_GAMEPAD_ADDED SDL_CONTROLLERDEVICEADDED
#define SDL_EVENT_GAMEPAD_REMOVED SDL_CONTROLLERDEVICEREMOVED
#define SDL_EVENT_GAMEPAD_BUTTON_DOWN SDL_CONTROLLERBUTTONDOWN
#define SDL_EVENT_GAMEPAD_BUTTON_UP SDL_CONTROLLERBUTTONUP
#define SDL_EVENT_GAMEPAD_AXIS_MOTION SDL_CONTROLLERAXISMOTION

// ============================================================================
// Gamepad button constants
// ============================================================================

#define SDL_GAMEPAD_BUTTON_SOUTH SDL_CONTROLLER_BUTTON_A
#define SDL_GAMEPAD_BUTTON_EAST SDL_CONTROLLER_BUTTON_B
#define SDL_GAMEPAD_BUTTON_WEST SDL_CONTROLLER_BUTTON_X
#define SDL_GAMEPAD_BUTTON_NORTH SDL_CONTROLLER_BUTTON_Y
#define SDL_GAMEPAD_BUTTON_BACK SDL_CONTROLLER_BUTTON_BACK
#define SDL_GAMEPAD_BUTTON_START SDL_CONTROLLER_BUTTON_START
#define SDL_GAMEPAD_BUTTON_DPAD_UP SDL_CONTROLLER_BUTTON_DPAD_UP
#define SDL_GAMEPAD_BUTTON_DPAD_DOWN SDL_CONTROLLER_BUTTON_DPAD_DOWN
#define SDL_GAMEPAD_BUTTON_DPAD_LEFT SDL_CONTROLLER_BUTTON_DPAD_LEFT
#define SDL_GAMEPAD_BUTTON_DPAD_RIGHT SDL_CONTROLLER_BUTTON_DPAD_RIGHT
#define SDL_GAMEPAD_BUTTON_LEFT_STICK SDL_CONTROLLER_BUTTON_LEFTSTICK
#define SDL_GAMEPAD_BUTTON_RIGHT_STICK SDL_CONTROLLER_BUTTON_RIGHTSTICK
#define SDL_GAMEPAD_BUTTON_LEFT_SHOULDER SDL_CONTROLLER_BUTTON_LEFTSHOULDER
#define SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
#define SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN (-1)

// ============================================================================
// Gamepad axis constants
// ============================================================================

#define SDL_GAMEPAD_AXIS_LEFTX SDL_CONTROLLER_AXIS_LEFTX
#define SDL_GAMEPAD_AXIS_LEFTY SDL_CONTROLLER_AXIS_LEFTY
#define SDL_GAMEPAD_AXIS_RIGHTX SDL_CONTROLLER_AXIS_RIGHTX
#define SDL_GAMEPAD_AXIS_RIGHTY SDL_CONTROLLER_AXIS_RIGHTY
#define SDL_GAMEPAD_AXIS_LEFT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERLEFT
#define SDL_GAMEPAD_AXIS_RIGHT_TRIGGER SDL_CONTROLLER_AXIS_TRIGGERRIGHT
#define SDL_GAMEPAD_AXIS_INVALID SDL_CONTROLLER_AXIS_INVALID
#define SDL_GAMEPAD_AXIS_COUNT SDL_CONTROLLER_AXIS_MAX

// ============================================================================
// Init flags
// ============================================================================

#define SDL_INIT_GAMEPAD SDL_INIT_GAMECONTROLLER

// ============================================================================
// Window flags
// ============================================================================

#define SDL_WINDOW_HIGH_PIXEL_DENSITY SDL_WINDOW_ALLOW_HIGHDPI

// ============================================================================
// Audio constants
// ============================================================================

#define SDL_AUDIO_S16LE AUDIO_S16LSB
#define SDL_AUDIO_S16 AUDIO_S16SYS
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK 0

// ============================================================================
// IOStream functions (SDL2: RWops)
// ============================================================================

#define SDL_IOFromFile(path, mode) SDL_RWFromFile(path, mode)
#define SDL_IOFromConstMem(mem, size) SDL_RWFromConstMem(mem, size)
#define SDL_CloseIO(io) SDL_RWclose(io)

// SDL_WriteIO in SDL3 returns bytes written, SDL_RWwrite in SDL2 returns objects written
static inline size_t SDL_WriteIO_Compat(SDL_IOStream *io, const void *ptr, size_t size) {
  return SDL_RWwrite(io, ptr, 1, size);
}
#define SDL_WriteIO(io, ptr, size) SDL_WriteIO_Compat(io, ptr, size)

#define SDL_LoadBMP_IO(io, close) SDL_LoadBMP_RW(io, close)

// ============================================================================
// Surface functions
// ============================================================================

#define SDL_DestroySurface(s) SDL_FreeSurface(s)
#define SDL_MapSurfaceRGB(s, r, g, b) SDL_MapRGB((s)->format, r, g, b)

static inline int SDL_SetSurfaceColorKey_Compat(SDL_Surface *surface, int flag, Uint32 key) {
  return SDL_SetColorKey(surface, flag ? SDL_TRUE : SDL_FALSE, key);
}
#define SDL_SetSurfaceColorKey(s, f, k) SDL_SetSurfaceColorKey_Compat(s, f, k)

// ============================================================================
// Gamepad functions
// ============================================================================

#define SDL_OpenGamepad(id) SDL_GameControllerOpen(id)
#define SDL_CloseGamepad(gp) SDL_GameControllerClose(gp)
#define SDL_GetGamepadName(gp) SDL_GameControllerName(gp)
#define SDL_IsGamepad(id) SDL_IsGameController(id)

// SDL2 doesn't have SDL_GetGamepads - return joystick IDs array
static inline SDL_JoystickID *SDL_GetGamepads_Compat(int *count) {
  int num = SDL_NumJoysticks();
  if (num <= 0) {
    *count = 0;
    return NULL;
  }
  SDL_JoystickID *ids = (SDL_JoystickID *)SDL_malloc(num * sizeof(SDL_JoystickID));
  if (!ids) {
    *count = 0;
    return NULL;
  }
  for (int i = 0; i < num; i++) {
    ids[i] = i;
  }
  *count = num;
  return ids;
}
#define SDL_GetGamepads(count) SDL_GetGamepads_Compat(count)

// SDL2 uses RW for controller DB
static inline int SDL_AddGamepadMappingsFromIO_Compat(SDL_IOStream *rw, int freerw) {
  return SDL_GameControllerAddMappingsFromRW(rw, freerw);
}
#define SDL_AddGamepadMappingsFromIO(rw, freerw) SDL_AddGamepadMappingsFromIO_Compat(rw, freerw)

// Gamepad string functions
#define SDL_GetGamepadStringForButton(b) SDL_GameControllerGetStringForButton(b)
#define SDL_GetGamepadStringForAxis(a) SDL_GameControllerGetStringForAxis(a)

// SDL2 doesn't have button labels - just return unknown
static inline int SDL_GetGamepadButtonLabel_Compat(SDL_Gamepad *gp, SDL_GamepadButton button) {
  (void)gp;
  (void)button;
  return SDL_GAMEPAD_BUTTON_LABEL_UNKNOWN;
}
#define SDL_GetGamepadButtonLabel(gp, b) SDL_GetGamepadButtonLabel_Compat(gp, b)

// ============================================================================
// Rendering functions
// ============================================================================

// SDL_RenderTexture -> SDL_RenderCopyF (SDL2 float version)
static inline int SDL_RenderTexture_Compat(SDL_Renderer *renderer, SDL_Texture *texture,
                                           const SDL_FRect *srcrect, const SDL_FRect *dstrect) {
  SDL_Rect src_int;
  SDL_Rect *src_ptr = NULL;
  if (srcrect) {
    src_int.x = (int)srcrect->x;
    src_int.y = (int)srcrect->y;
    src_int.w = (int)srcrect->w;
    src_int.h = (int)srcrect->h;
    src_ptr = &src_int;
  }
  return SDL_RenderCopyF(renderer, texture, src_ptr, dstrect);
}
#define SDL_RenderTexture(r, t, s, d) SDL_RenderTexture_Compat(r, t, s, d)

// SDL_RenderPoints -> SDL_RenderDrawPointsF
static inline int SDL_RenderPoints_Compat(SDL_Renderer *renderer, const SDL_FPoint *points,
                                          int count) {
  return SDL_RenderDrawPointsF(renderer, points, count);
}
#define SDL_RenderPoints(r, p, c) SDL_RenderPoints_Compat(r, p, c)

// SDL_RenderLines -> SDL_RenderDrawLinesF
static inline int SDL_RenderLines_Compat(SDL_Renderer *renderer, const SDL_FPoint *points,
                                         int count) {
  return SDL_RenderDrawLinesF(renderer, points, count);
}
#define SDL_RenderLines(r, p, c) SDL_RenderLines_Compat(r, p, c)

// SDL_RenderFillRect with FRect support -> convert to Rect
// SDL3 uses SDL_FRect, SDL2 uses SDL_Rect
// We redefine to use FRect since that's what SDL3 code expects
// Note: Use SDL_RenderFillRectF which is an SDL2 native function taking SDL_FRect
static inline int SDL_RenderFillRect_Compat(SDL_Renderer *renderer, const SDL_FRect *rect) {
  return SDL_RenderFillRectF(renderer, rect);
}
#undef SDL_RenderFillRect
#define SDL_RenderFillRect(r, rect) SDL_RenderFillRect_Compat(r, rect)

// SDL_RenderPresent returns void in SDL2, int in SDL3
static inline int SDL_RenderPresent_Compat(SDL_Renderer *renderer) {
  (void)renderer;
  // Call SDL2's SDL_RenderPresent via function pointer to avoid macro recursion
  extern DECLSPEC void SDLCALL SDL_RenderPresent(SDL_Renderer *renderer);
  // Can't easily avoid this - just use inline asm or accept the recursion isn't an issue
  // Actually SDL_RenderPresent isn't redefined yet at this point
  return 1;
}
// SDL2's SDL_RenderPresent returns void, but we need to wrap it for SDL3 compatibility
static inline int SDL_RenderPresent_SDL2(SDL_Renderer *renderer) {
  SDL_RenderPresent(renderer);
  return 1;
}
#undef SDL_RenderPresent
#define SDL_RenderPresent(r) SDL_RenderPresent_SDL2(r)

// ============================================================================
// Texture functions
// ============================================================================

// SDL3 texture properties -> SDL2 QueryTexture
static inline int SDL_GetTextureSize_Compat(SDL_Texture *texture, float *w, float *h) {
  int iw, ih;
  if (SDL_QueryTexture(texture, NULL, NULL, &iw, &ih) != 0) {
    return -1;
  }
  if (w)
    *w = (float)iw;
  if (h)
    *h = (float)ih;
  return 0;
}
#define SDL_GetTextureSize(t, w, h) SDL_GetTextureSize_Compat(t, w, h)

// SDL3 property system not available in SDL2 - provide wrapper
#define SDL_PROP_TEXTURE_WIDTH_NUMBER "SDL.texture.width"
#define SDL_PROP_TEXTURE_HEIGHT_NUMBER "SDL.texture.height"

// Stub for properties - returns texture pointer as handle
static inline void *SDL_GetTextureProperties_Compat(SDL_Texture *texture) { return texture; }
#define SDL_GetTextureProperties(t) SDL_GetTextureProperties_Compat(t)

static inline Sint64 SDL_GetNumberProperty_Compat(void *props, const char *name,
                                                  Sint64 default_value) {
  // props is actually the texture pointer in our SDL2 compat
  SDL_Texture *texture = (SDL_Texture *)props;
  if (texture == NULL)
    return default_value;

  int w, h;
  if (SDL_QueryTexture(texture, NULL, NULL, &w, &h) != 0) {
    return default_value;
  }

  if (SDL_strcmp(name, SDL_PROP_TEXTURE_WIDTH_NUMBER) == 0) {
    return w;
  } else if (SDL_strcmp(name, SDL_PROP_TEXTURE_HEIGHT_NUMBER) == 0) {
    return h;
  }
  return default_value;
}
#define SDL_GetNumberProperty(p, n, d) SDL_GetNumberProperty_Compat(p, n, d)

// ============================================================================
// Window functions
// ============================================================================

// SDL_GetWindowSizeInPixels -> SDL_GL_GetDrawableSize
static inline int SDL_GetWindowSizeInPixels_Compat(SDL_Window *window, int *w, int *h) {
  SDL_GL_GetDrawableSize(window, w, h);
  return 1; // SDL3 returns bool
}
#define SDL_GetWindowSizeInPixels(win, w, h) SDL_GetWindowSizeInPixels_Compat(win, w, h)

// SDL_CreateWindowAndRenderer signature differs
static inline int SDL_CreateWindowAndRenderer_Compat(const char *title, int width, int height,
                                                     Uint32 window_flags, SDL_Window **window,
                                                     SDL_Renderer **renderer) {
  // SDL2's version doesn't take title
  Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
  if (SDL_CreateWindowAndRenderer(width, height, window_flags, window, renderer) != 0) {
    return 0;
  }
  SDL_SetWindowTitle(*window, title);
  return 1;
}
#define SDL_CreateWindowAndRenderer(t, w, h, f, win, rend) \
  SDL_CreateWindowAndRenderer_Compat(t, w, h, f, win, rend)

// SDL_SetRenderLogicalPresentation -> SDL_RenderSetLogicalSize
static inline int SDL_SetRenderLogicalPresentation_Compat(SDL_Renderer *renderer, int w, int h,
                                                          SDL_RendererLogicalPresentation mode) {
  if (mode == SDL_LOGICAL_PRESENTATION_DISABLED) {
    return SDL_RenderSetLogicalSize(renderer, 0, 0) == 0;
  }
  // SDL2 doesn't have integer scale mode built-in, just set logical size
  return SDL_RenderSetLogicalSize(renderer, w, h) == 0;
}
#define SDL_SetRenderLogicalPresentation(r, w, h, m) \
  SDL_SetRenderLogicalPresentation_Compat(r, w, h, m)

// SDL_SetRenderVSync
static inline int SDL_SetRenderVSync_Compat(SDL_Renderer *renderer, int vsync) {
  (void)renderer;
  (void)vsync;
  // SDL2 sets vsync at renderer creation via SDL_RENDERER_PRESENTVSYNC flag
  return 1;
}
#define SDL_SetRenderVSync(r, v) SDL_SetRenderVSync_Compat(r, v)

// SDL_SyncWindow doesn't exist in SDL2
static inline void SDL_SyncWindow_Compat(SDL_Window *window) { (void)window; }
#define SDL_SyncWindow(w) SDL_SyncWindow_Compat(w)

// Cursor functions - SDL2 uses different API
static inline int SDL_ShowCursor_Compat(void) { return SDL_ShowCursor(SDL_ENABLE); }
static inline int SDL_HideCursor_Compat(void) { return SDL_ShowCursor(SDL_DISABLE); }
#define SDL_ShowCursor() SDL_ShowCursor_Compat()
#define SDL_HideCursor() SDL_HideCursor_Compat()

// ============================================================================
// Logging
// ============================================================================

// SDL_SetLogPriorities -> SDL_LogSetAllPriority
#define SDL_SetLogPriorities(priority) SDL_LogSetAllPriority(priority)

// SDL3 uses SDL_GetLogOutputFunction/SDL_SetLogOutputFunction
// SDL2 uses SDL_LogGetOutputFunction/SDL_LogSetOutputFunction
#define SDL_GetLogOutputFunction(fn, userdata) SDL_LogGetOutputFunction(fn, userdata)
#define SDL_SetLogOutputFunction(fn, userdata) SDL_LogSetOutputFunction(fn, userdata)

// ============================================================================
// Misc functions
// ============================================================================

// SDL_SetAppMetadata doesn't exist in SDL2
static inline int SDL_SetAppMetadata_Compat(const char *appname, const char *appversion,
                                            const char *appidentifier) {
  (void)appname;
  (void)appversion;
  (void)appidentifier;
  return 1;
}
#define SDL_SetAppMetadata(n, v, i) SDL_SetAppMetadata_Compat(n, v, i)

// SDL_SetHint exists in both but some hints are different
#define SDL_HINT_MAIN_CALLBACK_RATE "SDL_MAIN_CALLBACK_RATE"
#define SDL_HINT_IOS_HIDE_HOME_INDICATOR "SDL_IOS_HIDE_HOME_INDICATOR"
#define SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES "SDL_AUDIO_DEVICE_SAMPLE_FRAMES"

// SDL_strtok_r doesn't exist in SDL2 - use standard C strtok_r
#include <string.h>
#ifdef _WIN32
#define SDL_strtok_r(str, delim, saveptr) strtok_s(str, delim, saveptr)
#else
#define SDL_strtok_r(str, delim, saveptr) strtok_r(str, delim, saveptr)
#endif

// SDL3 renamed SDL_SetThreadPriority to SDL_SetCurrentThreadPriority
#define SDL_SetCurrentThreadPriority(p) SDL_SetThreadPriority(p)

// SDL3 renamed thread priority constants
#define SDL_THREAD_PRIORITY_TIME_CRITICAL SDL_THREAD_PRIORITY_HIGH

// ============================================================================
// Event structure differences
// ============================================================================

// SDL2 uses event.key.keysym.scancode, SDL3 uses event.key.scancode
// SDL2 uses event.key.keysym.sym, SDL3 uses event.key.key
// SDL2 uses event.cbutton, SDL3 uses event.gbutton
// SDL2 uses event.caxis, SDL3 uses event.gaxis

// Accessors for keyboard events
#define COMPAT_KEY_SCANCODE(event) ((event)->key.keysym.scancode)
#define COMPAT_KEY_SYM(event) ((event)->key.keysym.sym)
#define COMPAT_KEY_MOD(event) ((event)->key.keysym.mod)
#define COMPAT_KEY_REPEAT(event) ((event)->key.repeat)

// Accessors for gamepad events
#define COMPAT_GBUTTON_BUTTON(event) ((event)->cbutton.button)
#define COMPAT_GAXIS_AXIS(event) ((event)->caxis.axis)
#define COMPAT_GAXIS_VALUE(event) ((event)->caxis.value)

// Window event check for SDL2 (needs subtype check)
#define COMPAT_IS_WINDOW_RESIZE(event) \
  ((event)->type == SDL_WINDOWEVENT && (event)->window.event == SDL_WINDOWEVENT_RESIZED)
#define COMPAT_IS_WINDOW_MOVED(event) \
  ((event)->type == SDL_WINDOWEVENT && (event)->window.event == SDL_WINDOWEVENT_MOVED)

#else // SDL3

// SDL3 includes
#include <SDL3/SDL.h>

// SDL3 event accessors
#define COMPAT_KEY_SCANCODE(event) ((event)->key.scancode)
#define COMPAT_KEY_SYM(event) ((event)->key.key)
#define COMPAT_KEY_MOD(event) ((event)->key.mod)
#define COMPAT_KEY_REPEAT(event) ((event)->key.repeat)

// Gamepad event accessors
#define COMPAT_GBUTTON_BUTTON(event) ((event)->gbutton.button)
#define COMPAT_GAXIS_AXIS(event) ((event)->gaxis.axis)
#define COMPAT_GAXIS_VALUE(event) ((event)->gaxis.value)

// Window event check for SDL3 (direct event types)
#define COMPAT_IS_WINDOW_RESIZE(event) ((event)->type == SDL_EVENT_WINDOW_RESIZED)
#define COMPAT_IS_WINDOW_MOVED(event) ((event)->type == SDL_EVENT_WINDOW_MOVED)

// SDL3 renamed KMOD_* to SDL_KMOD_*
#undef KMOD_GUI
#undef KMOD_CTRL
#undef KMOD_ALT
#undef KMOD_SHIFT
#define KMOD_GUI SDL_KMOD_GUI
#define KMOD_CTRL SDL_KMOD_CTRL
#define KMOD_ALT SDL_KMOD_ALT
#define KMOD_SHIFT SDL_KMOD_SHIFT

#endif // USE_SDL2

#endif // SDL_COMPAT_H
