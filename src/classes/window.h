#pragma once

#include "../system/object/Self.h"

#define WINDOW_ASPECT_BLACKBAR 0
#define WINDOW_ASPECT_STRETCH  1

#define WINDOW_INTERP_NEAREST  TEXTURE_FILTER_POINT
#define WINDOW_INTERP_BILINEAR TEXTURE_FILTER_BILINEAR

// ============================================================
// Class definition
// ============================================================

#define TYPE Window

BEGIN_CLASS(0x22FF);
INHERITS(Object);

DECLARE_SELF_MID(Open);
DECLARE_SELF_MID(Close);
DECLARE_SELF_MID(SetTitle);
DECLARE_SELF_MID(SetSize);
DECLARE_SELF_MID(SetTargetFPS);
DECLARE_SELF_MID(ToggleFullscreen);
DECLARE_SELF_MID(SetVsync);
DECLARE_SELF_MID(SetInterpolation);
DECLARE_SELF_MID(SetAspectMode);
DECLARE_SELF_MID(SetVirtualSize);
DECLARE_SELF_MID(GetInfo);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    RenderTexture2D empty_vtex = {0};
    _Object_StoreValue(Self_Values, "vtex", 4,
                       &empty_vtex, sizeof(RenderTexture2D),
                       CID_Window, SER_SKIP, 0);
    Self_SetTransient("vw", int, 0);
    Self_SetTransient("vh", int, 0);
    Self_SetTransient("rw", int, 0);
    Self_SetTransient("rh", int, 0);
    Self_SetValue("interp", int, WINDOW_INTERP_NEAREST);
    Self_SetValue("aspect", int, WINDOW_ASPECT_BLACKBAR);
    Self_SetTransient("has_virtual", int, 0);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
    int has_virtual = Self_GetDeref("has_virtual", int);
    if (has_virtual) {
        RenderTexture2D *vtex = Self_Get("vtex", RenderTexture2D);
        if (vtex) UnloadRenderTexture(*vtex);
    }
    if (IsWindowReady()) {
        CloseWindow();
        LOG_INFO("Window closed via SELF_Destroy");
    }
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Open)
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
    Self_SetTransient("rw", int, GetScreenWidth());
    Self_SetTransient("rh", int, GetScreenHeight());
    LOG_INFO("Window opened: %dx%d \"%s\"", Width, Height, Title);

    if (MH_Has(VirtualWidth) && MH_Has(VirtualHeight)) {
        int vw = MH_GetDeref(VirtualWidth, int);
        int vh = MH_GetDeref(VirtualHeight, int);
        if (vw > 0 && vh > 0) {
            RenderTexture2D vtex = LoadRenderTexture(vw, vh);
            SetTextureFilter(vtex.texture, Self_GetDeref("interp", int));
            _Object_StoreValue(Self_Values, "vtex", 4,
                               &vtex, sizeof(RenderTexture2D),
                               CID_Window, SER_SKIP, 0);
            Self_SetTransient("vw", int, vw);
            Self_SetTransient("vh", int, vh);
            Self_SetTransient("has_virtual", int, 1);
            LOG_INFO("Virtual screen: %dx%d", vw, vh);
        }
    }
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(Close)
    if (!IsWindowReady()) {
        payload->result = MESSAGE_RESULT_NOT_READY;
        return;
    }
    int has_virtual = Self_GetDeref("has_virtual", int);
    if (has_virtual) {
        RenderTexture2D *vtex = Self_Get("vtex", RenderTexture2D);
        if (vtex) UnloadRenderTexture(*vtex);
        Self_SetTransient("has_virtual", int, 0);
        Self_SetTransient("vw", int, 0);
        Self_SetTransient("vh", int, 0);
    }
    CloseWindow();
    LOG_INFO("Window closed");
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetTitle)
    MH_Require(Title);
    char *Title = (char *)MH_Get(Title, char);
    SetWindowTitle(Title);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetSize)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    SetWindowSize(Width, Height);
    Self_SetTransient("rw", int, Width);
    Self_SetTransient("rh", int, Height);
    LOG_INFO("Window resized to %dx%d", Width, Height);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetTargetFPS)
    MH_ExtractDeref(FPS, int);
    SetTargetFPS(FPS);
    LOG_INFO("Target FPS set to %d", FPS);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(ToggleFullscreen)
    ToggleFullscreen();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetVsync)
    MH_ExtractDeref(Enabled, int);
    if (Enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetInterpolation)
    MH_ExtractDeref(Mode, int);
    Self_SetValue("interp", int, Mode);
    int has_virtual = Self_GetDeref("has_virtual", int);
    if (has_virtual) {
        RenderTexture2D *vtex = Self_Get("vtex", RenderTexture2D);
        if (vtex) SetTextureFilter(vtex->texture, Mode);
    }
    LOG_INFO("Interpolation set to %d", Mode);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetAspectMode)
    MH_ExtractDeref(Mode, int);
    Self_SetValue("aspect", int, Mode);
    LOG_INFO("Aspect mode set to %s",
        Mode == WINDOW_ASPECT_STRETCH ? "STRETCH" : "BLACKBAR");
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(SetVirtualSize)
    MH_ExtractDeref(VirtualWidth, int);
    MH_ExtractDeref(VirtualHeight, int);
    if (VirtualWidth <= 0 || VirtualHeight <= 0) {
        payload->result = MESSAGE_RESULT_INVALID_PARAMS;
        return;
    }
    int has_virtual = Self_GetDeref("has_virtual", int);
    if (has_virtual) {
        RenderTexture2D *vtex = Self_Get("vtex", RenderTexture2D);
        if (vtex) UnloadRenderTexture(*vtex);
    }
    RenderTexture2D vtex = LoadRenderTexture(VirtualWidth, VirtualHeight);
    SetTextureFilter(vtex.texture, Self_GetDeref("interp", int));
    _Object_StoreValue(Self_Values, "vtex", 4,
                       &vtex, sizeof(RenderTexture2D),
                       CID_Window, SER_SKIP, 0);
    Self_SetValue("vw", int, VirtualWidth);
    Self_SetValue("vh", int, VirtualHeight);
    Self_SetValue("has_virtual", int, 1);
    LOG_INFO("Virtual screen set to %dx%d", VirtualWidth, VirtualHeight);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN(GetInfo)
    Self_SetTransient("rw", int, GetScreenWidth());
    Self_SetTransient("rh", int, GetScreenHeight());
    MH_SetValue(Width, int, GetScreenWidth());
    MH_SetValue(Height, int, GetScreenHeight());
    MH_SetValue(VirtualWidth, int, Self_GetDeref("vw", int));
    MH_SetValue(VirtualHeight, int, Self_GetDeref("vh", int));
    MH_SetValue(Fullscreen, int, IsWindowFullscreen());
    MH_SetValue(Focused, int, IsWindowFocused());
    MH_SetValue(FPS, int, GetFPS());
    MH_SetValue(Interpolation, int, Self_GetDeref("interp", int));
    MH_SetValue(AspectMode, int, Self_GetDeref("aspect", int));
    MH_SetValue(HasVirtual, int, Self_GetDeref("has_virtual", int));
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID(Open)
    SELF_CAN_RECEIVE_MID(Close)
    SELF_CAN_RECEIVE_MID(SetTitle)
    SELF_CAN_RECEIVE_MID(SetSize)
    SELF_CAN_RECEIVE_MID(SetTargetFPS)
    SELF_CAN_RECEIVE_MID(ToggleFullscreen)
    SELF_CAN_RECEIVE_MID(SetVsync)
    SELF_CAN_RECEIVE_MID(SetInterpolation)
    SELF_CAN_RECEIVE_MID(SetAspectMode)
    SELF_CAN_RECEIVE_MID(SetVirtualSize)
    SELF_CAN_RECEIVE_MID(GetInfo)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE(Open)
    SELF_RECEIVE_MESSAGE_ROUTE(Close)
    SELF_RECEIVE_MESSAGE_ROUTE(SetTitle)
    SELF_RECEIVE_MESSAGE_ROUTE(SetSize)
    SELF_RECEIVE_MESSAGE_ROUTE(SetTargetFPS)
    SELF_RECEIVE_MESSAGE_ROUTE(ToggleFullscreen)
    SELF_RECEIVE_MESSAGE_ROUTE(SetVsync)
    SELF_RECEIVE_MESSAGE_ROUTE(SetInterpolation)
    SELF_RECEIVE_MESSAGE_ROUTE(SetAspectMode)
    SELF_RECEIVE_MESSAGE_ROUTE(SetVirtualSize)
    SELF_RECEIVE_MESSAGE_ROUTE(GetInfo)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(Object)

#undef TYPE

DECLARE_SINGLETON(Window)

// ============================================================
// Frame helpers
// ============================================================

static void Window_BeginFrame(void) {
    BeginDrawing();
    ClearBackground(BLACK);
    if (_Window_singleton == NULL) return;
    TempObjectReference w = GET_SINGLETON(Window);
    if (!w || !w->data) return;
    int *vflag = (int *)_Object_GetValueData(w->data->values, "has_virtual", 11);
    if (vflag && *vflag) {
        RenderTexture2D *vtex = (RenderTexture2D *)_Object_GetValueData(w->data->values, "vtex", 4);
        if (vtex) BeginTextureMode(*vtex);
    }
}

static void Window_EndFrame(void) {
    if (_Window_singleton == NULL) { EndDrawing(); return; }
    TempObjectReference w = GET_SINGLETON(Window);
    if (!w || !w->data) { EndDrawing(); return; }

    // Keep rw/rh in sync with actual screen size (window is resizable)
    int _cur_rw = GetScreenWidth();
    int _cur_rh = GetScreenHeight();
    _Object_StoreValue(w->data->values, "rw", 2, &_cur_rw, sizeof(int), CID_Window, SER_SKIP, 0);
    _Object_StoreValue(w->data->values, "rh", 2, &_cur_rh, sizeof(int), CID_Window, SER_SKIP, 0);

    int *vflag = (int *)_Object_GetValueData(w->data->values, "has_virtual", 11);
    if (vflag && *vflag) {
        EndTextureMode();

        RenderTexture2D *vtex = (RenderTexture2D *)_Object_GetValueData(w->data->values, "vtex", 4);
        int *vw = (int *)_Object_GetValueData(w->data->values, "vw", 2);
        int *vh = (int *)_Object_GetValueData(w->data->values, "vh", 2);
        int *interp = (int *)_Object_GetValueData(w->data->values, "interp", 6);
        int *aspect = (int *)_Object_GetValueData(w->data->values, "aspect", 6);

        if (!vtex || !vw || !vh || !interp || !aspect) { EndDrawing(); return; }

        SetTextureFilter(vtex->texture, *interp);

        Rectangle src = { 0, 0, (float)*vw, -(float)*vh };
        Rectangle dst;

        if (*aspect == WINDOW_ASPECT_STRETCH) {
            dst = (Rectangle){ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
        } else {
            float sw = (float)GetScreenWidth();
            float sh = (float)GetScreenHeight();
            float scale = (sw / *vw < sh / *vh) ? sw / *vw : sh / *vh;
            float dw = *vw * scale;
            float dh = *vh * scale;
            dst = (Rectangle){ (sw - dw) * 0.5f, (sh - dh) * 0.5f, dw, dh };
        }

        DrawTexturePro(vtex->texture, src, dst, (Vector2){0, 0}, 0.0f, WHITE);
    }
    EndDrawing();
}
