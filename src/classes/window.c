#include "window.h"
#include <raylib.h>

#define TYPE Window

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Create)
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

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Destroy)
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

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(Open)
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

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(Close)
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

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetTitle)
    MH_Require(Title);
    char *Title = (char *)MH_Get(Title, char);
    SetWindowTitle(Title);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetSize)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    SetWindowSize(Width, Height);
    Self_SetTransient("rw", int, Width);
    Self_SetTransient("rh", int, Height);
    LOG_INFO("Window resized to %dx%d", Width, Height);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetTargetFPS)
    MH_ExtractDeref(FPS, int);
    SetTargetFPS(FPS);
    LOG_INFO("Target FPS set to %d", FPS);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(ToggleFullscreen)
    ToggleFullscreen();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetVsync)
    MH_ExtractDeref(Enabled, int);
    if (Enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetInterpolation)
    MH_ExtractDeref(Mode, int);
    Self_SetValue("interp", int, Mode);
    int has_virtual = Self_GetDeref("has_virtual", int);
    if (has_virtual) {
        RenderTexture2D *vtex = Self_Get("vtex", RenderTexture2D);
        if (vtex) SetTextureFilter(vtex->texture, Mode);
    }
    LOG_INFO("Interpolation set to %d", Mode);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetAspectMode)
    MH_ExtractDeref(Mode, int);
    Self_SetValue("aspect", int, Mode);
    LOG_INFO("Aspect mode set to %s",
        Mode == WINDOW_ASPECT_STRETCH ? "STRETCH" : "BLACKBAR");
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(SetVirtualSize)
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

SELF_MESSAGE_HANDLER_BEGIN_SPLIT(GetInfo)
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

CLASSDEF_SPLIT_INHERITS(Object)

DECLARE_SINGLETON_IMPL(Window)

#undef TYPE
