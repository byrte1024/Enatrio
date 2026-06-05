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

DECLARE_SELF_MID(Open, 0x01);
DECLARE_SELF_MID(Close, 0x02);
DECLARE_SELF_MID(SetTitle, 0x03);
DECLARE_SELF_MID(SetSize, 0x04);
DECLARE_SELF_MID(SetTargetFPS, 0x05);
DECLARE_SELF_MID(ToggleFullscreen, 0x06);
DECLARE_SELF_MID(SetVsync, 0x07);
DECLARE_SELF_MID(SetInterpolation, 0x08);
DECLARE_SELF_MID(SetAspectMode, 0x09);
DECLARE_SELF_MID(SetVirtualSize, 0x0A);
DECLARE_SELF_MID(GetInfo, 0x0B);

SELF_MESSAGE_HANDLER_DECL_EXTERN(Object, Create)
SELF_MESSAGE_HANDLER_DECL_EXTERN(Object, Destroy)
SELF_MESSAGE_HANDLER_DECL(Open)
SELF_MESSAGE_HANDLER_DECL(Close)
SELF_MESSAGE_HANDLER_DECL(SetTitle)
SELF_MESSAGE_HANDLER_DECL(SetSize)
SELF_MESSAGE_HANDLER_DECL(SetTargetFPS)
SELF_MESSAGE_HANDLER_DECL(ToggleFullscreen)
SELF_MESSAGE_HANDLER_DECL(SetVsync)
SELF_MESSAGE_HANDLER_DECL(SetInterpolation)
SELF_MESSAGE_HANDLER_DECL(SetAspectMode)
SELF_MESSAGE_HANDLER_DECL(SetVirtualSize)
SELF_MESSAGE_HANDLER_DECL(GetInfo)

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

CLASSDEF_DECL_INHERITS(Object)

#undef TYPE

DECLARE_SINGLETON_DECL(Window)

// ============================================================
// Frame helpers (static inline -- stay in header for inlining)
// ============================================================

static void Window_BeginFrame(void) {
    if (_Window_singleton == NULL) return;
    TempObjectReference w = GET_SINGLETON(Window);
    if (!w || !w->data) return;
    int *vflag = (int *)_Object_GetValueData(w->data->values, "has_virtual", 11);
    if (vflag && *vflag) {
        RenderTexture2D *vtex = (RenderTexture2D *)_Object_GetValueData(w->data->values, "vtex", 4);
        if (vtex) BeginTextureMode(*vtex);
    }
}

static void Window_BlitVirtualScreen(void) {
    if (_Window_singleton == NULL) return;
    TempObjectReference w = GET_SINGLETON(Window);
    if (!w || !w->data) return;

    int _cur_rw = GetScreenWidth();
    int _cur_rh = GetScreenHeight();
    _Object_StoreValue(w->data->values, "rw", 2, &_cur_rw, sizeof(int), CID_Window, SER_SKIP, 0);
    _Object_StoreValue(w->data->values, "rh", 2, &_cur_rh, sizeof(int), CID_Window, SER_SKIP, 0);

    int *vflag = (int *)_Object_GetValueData(w->data->values, "has_virtual", 11);
    if (!vflag || !*vflag) return;

    RenderTexture2D *vtex = (RenderTexture2D *)_Object_GetValueData(w->data->values, "vtex", 4);
    int *vw = (int *)_Object_GetValueData(w->data->values, "vw", 2);
    int *vh = (int *)_Object_GetValueData(w->data->values, "vh", 2);
    int *interp = (int *)_Object_GetValueData(w->data->values, "interp", 6);
    int *aspect = (int *)_Object_GetValueData(w->data->values, "aspect", 6);

    if (!vtex || !vw || !vh || !interp || !aspect) return;

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

static void Window_EndFrame(void) {
    if (_Window_singleton == NULL) return;
    TempObjectReference w = GET_SINGLETON(Window);
    if (!w || !w->data) return;

    int *vflag = (int *)_Object_GetValueData(w->data->values, "has_virtual", 11);
    if (vflag && *vflag) EndTextureMode();

    Window_BlitVirtualScreen();
}
