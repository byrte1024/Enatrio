#pragma once

#include "../system/class/Class.h"

// ============================================================
// Virtual screen state
// ============================================================

#define WINDOW_ASPECT_BLACKBAR 0
#define WINDOW_ASPECT_STRETCH  1

#define WINDOW_INTERP_NEAREST  TEXTURE_FILTER_POINT
#define WINDOW_INTERP_BILINEAR TEXTURE_FILTER_BILINEAR

static RenderTexture2D _window_vtex = {0};
static int _window_vw = 0;
static int _window_vh = 0;
static int _window_interp = WINDOW_INTERP_NEAREST;
static int _window_aspect = WINDOW_ASPECT_BLACKBAR;
static bool _window_has_virtual = false;

// ============================================================
// Frame helpers -- called directly in the game loop, not via messages.
// Window_BeginFrame: clears real screen, begins drawing to virtual screen.
// Window_EndFrame: presents virtual screen scaled to real screen.
// ============================================================

static void Window_BeginFrame(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    if (_window_has_virtual) {
        BeginTextureMode(_window_vtex);
    }
}

static void Window_EndFrame(void) {
    if (_window_has_virtual) {
        EndTextureMode();

        SetTextureFilter(_window_vtex.texture, _window_interp);

        // RenderTexture is Y-flipped in raylib
        Rectangle src = { 0, 0, (float)_window_vw, -(float)_window_vh };
        Rectangle dst;

        if (_window_aspect == WINDOW_ASPECT_STRETCH) {
            dst = (Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
        } else {
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            float scale = (sw / _window_vw < sh / _window_vh)
                        ? sw / _window_vw
                        : sh / _window_vh;
            float dw = _window_vw * scale;
            float dh = _window_vh * scale;
            dst = (Rectangle){ (sw - dw) * 0.5f, (sh - dh) * 0.5f, dw, dh };
        }

        DrawTexturePro(_window_vtex.texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
    EndDrawing();
}

// ============================================================
// Class definition
// ============================================================

#define TYPE Window

BEGIN_CLASS(0x22FF);

DECLARE_MID(Open);
DECLARE_MID(Close);
DECLARE_MID(SetTitle);
DECLARE_MID(SetSize);
DECLARE_MID(SetTargetFPS);
DECLARE_MID(ToggleFullscreen);
DECLARE_MID(SetVsync);
DECLARE_MID(SetInterpolation);
DECLARE_MID(SetAspectMode);
DECLARE_MID(SetVirtualSize);
DECLARE_MID(GetInfo);

// Params: Width (int), Height (int), Title (char*)
// Optional: VirtualWidth (int), VirtualHeight (int)
MESSAGE_HANDLER_BEGIN(Open)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    char *Title = (char *)MH_Get(Title, char);
    if (!Title) Title = "Enatrio";

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(Width, Height, Title);
    if (!IsWindowReady()) {
        LOG_ERROR("Failed to open window %dx%d", Width, Height);
        payload->result = MESSAGE_RESULT_INTERNAL_ERROR;
        return;
    }
    LOG_INFO("Window opened: %dx%d \"%s\"", Width, Height, Title);

    if (MH_Has(VirtualWidth) && MH_Has(VirtualHeight)) {
        int vw = MH_GetDeref(VirtualWidth, int);
        int vh = MH_GetDeref(VirtualHeight, int);
        if (vw > 0 && vh > 0) {
            _window_vtex = LoadRenderTexture(vw, vh);
            _window_vw = vw;
            _window_vh = vh;
            _window_has_virtual = true;
            SetTextureFilter(_window_vtex.texture, _window_interp);
            LOG_INFO("Virtual screen: %dx%d", vw, vh);
        }
    }
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(Close)
    if (!IsWindowReady()) {
        payload->result = MESSAGE_RESULT_NOT_READY;
        return;
    }
    if (_window_has_virtual) {
        UnloadRenderTexture(_window_vtex);
        _window_has_virtual = false;
        _window_vw = 0;
        _window_vh = 0;
    }
    CloseWindow();
    LOG_INFO("Window closed");
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(SetTitle)
    MH_Require(Title);
    char *Title = (char *)MH_Get(Title, char);
    SetWindowTitle(Title);
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(SetSize)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    SetWindowSize(Width, Height);
    LOG_INFO("Window resized to %dx%d", Width, Height);
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(SetTargetFPS)
    MH_ExtractDeref(FPS, int);
    SetTargetFPS(FPS);
    LOG_INFO("Target FPS set to %d", FPS);
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(ToggleFullscreen)
    ToggleFullscreen();
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(SetVsync)
    MH_ExtractDeref(Enabled, int);
    if (Enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
MESSAGE_HANDLER_END()

// Params: Mode (int) -- WINDOW_INTERP_NEAREST or WINDOW_INTERP_BILINEAR
MESSAGE_HANDLER_BEGIN(SetInterpolation)
    MH_ExtractDeref(Mode, int);
    _window_interp = Mode;
    if (_window_has_virtual) {
        SetTextureFilter(_window_vtex.texture, _window_interp);
    }
    LOG_INFO("Interpolation set to %d", Mode);
MESSAGE_HANDLER_END()

// Params: Mode (int) -- WINDOW_ASPECT_BLACKBAR or WINDOW_ASPECT_STRETCH
MESSAGE_HANDLER_BEGIN(SetAspectMode)
    MH_ExtractDeref(Mode, int);
    _window_aspect = Mode;
    LOG_INFO("Aspect mode set to %s", Mode == WINDOW_ASPECT_STRETCH ? "STRETCH" : "BLACKBAR");
MESSAGE_HANDLER_END()

// Params: VirtualWidth (int), VirtualHeight (int)
MESSAGE_HANDLER_BEGIN(SetVirtualSize)
    MH_ExtractDeref(VirtualWidth, int);
    MH_ExtractDeref(VirtualHeight, int);
    if (VirtualWidth <= 0 || VirtualHeight <= 0) {
        payload->result = MESSAGE_RESULT_INVALID_PARAMS;
        return;
    }
    if (_window_has_virtual) {
        UnloadRenderTexture(_window_vtex);
    }
    _window_vtex = LoadRenderTexture(VirtualWidth, VirtualHeight);
    _window_vw = VirtualWidth;
    _window_vh = VirtualHeight;
    _window_has_virtual = true;
    SetTextureFilter(_window_vtex.texture, _window_interp);
    LOG_INFO("Virtual screen set to %dx%d", VirtualWidth, VirtualHeight);
MESSAGE_HANDLER_END()

// Out: Width, Height, VirtualWidth, VirtualHeight, Fullscreen, Focused, FPS,
//      Interpolation, AspectMode, HasVirtual (all int)
MESSAGE_HANDLER_BEGIN(GetInfo)
    MH_SetValue(Width, int, GetScreenWidth());
    MH_SetValue(Height, int, GetScreenHeight());
    MH_SetValue(VirtualWidth, int, _window_vw);
    MH_SetValue(VirtualHeight, int, _window_vh);
    MH_SetValue(Fullscreen, int, IsWindowFullscreen());
    MH_SetValue(Focused, int, IsWindowFocused());
    MH_SetValue(FPS, int, GetFPS());
    MH_SetValue(Interpolation, int, _window_interp);
    MH_SetValue(AspectMode, int, _window_aspect);
    MH_SetValue(HasVirtual, int, _window_has_virtual ? 1 : 0);
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Open)
    CAN_RECEIVE_MID(Close)
    CAN_RECEIVE_MID(SetTitle)
    CAN_RECEIVE_MID(SetSize)
    CAN_RECEIVE_MID(SetTargetFPS)
    CAN_RECEIVE_MID(ToggleFullscreen)
    CAN_RECEIVE_MID(SetVsync)
    CAN_RECEIVE_MID(SetInterpolation)
    CAN_RECEIVE_MID(SetAspectMode)
    CAN_RECEIVE_MID(SetVirtualSize)
    CAN_RECEIVE_MID(GetInfo)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    RECEIVE_MESSAGE_ROUTE(Open)
    RECEIVE_MESSAGE_ROUTE(Close)
    RECEIVE_MESSAGE_ROUTE(SetTitle)
    RECEIVE_MESSAGE_ROUTE(SetSize)
    RECEIVE_MESSAGE_ROUTE(SetTargetFPS)
    RECEIVE_MESSAGE_ROUTE(ToggleFullscreen)
    RECEIVE_MESSAGE_ROUTE(SetVsync)
    RECEIVE_MESSAGE_ROUTE(SetInterpolation)
    RECEIVE_MESSAGE_ROUTE(SetAspectMode)
    RECEIVE_MESSAGE_ROUTE(SetVirtualSize)
    RECEIVE_MESSAGE_ROUTE(GetInfo)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE
