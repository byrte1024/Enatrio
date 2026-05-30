#pragma once

#include "../system/class/Class.h"

#define TYPE Window

BEGIN_CLASS(0x22FF);

DECLARE_MID(Open);
DECLARE_MID(Close);
DECLARE_MID(SetTitle);
DECLARE_MID(SetSize);
DECLARE_MID(SetTargetFPS);
DECLARE_MID(ToggleFullscreen);
DECLARE_MID(SetVsync);
DECLARE_MID(GetInfo);

// Params: Width (int), Height (int), Title (char*)
MESSAGE_HANDLER_BEGIN(Open)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    char *Title = (char *)MH_Get(Title, char);
    if (!Title) Title = "Enatrio";

    InitWindow(Width, Height, Title);
    if (!IsWindowReady()) {
        LOG_ERROR("Failed to open window %dx%d", Width, Height);
        payload->result = MESSAGE_RESULT_INTERNAL_ERROR;
        return;
    }
    LOG_INFO("Window opened: %dx%d \"%s\"", Width, Height, Title);
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(Close)
    if (!IsWindowReady()) {
        payload->result = MESSAGE_RESULT_NOT_READY;
        return;
    }
    CloseWindow();
    LOG_INFO("Window closed");
MESSAGE_HANDLER_END()

// Params: Title (char*)
MESSAGE_HANDLER_BEGIN(SetTitle)
    MH_Require(Title);
    char *Title = (char *)MH_Get(Title, char);
    SetWindowTitle(Title);
MESSAGE_HANDLER_END()

// Params: Width (int), Height (int)
MESSAGE_HANDLER_BEGIN(SetSize)
    MH_ExtractDeref(Width, int);
    MH_ExtractDeref(Height, int);
    SetWindowSize(Width, Height);
    LOG_INFO("Window resized to %dx%d", Width, Height);
MESSAGE_HANDLER_END()

// Params: FPS (int)
MESSAGE_HANDLER_BEGIN(SetTargetFPS)
    MH_ExtractDeref(FPS, int);
    SetTargetFPS(FPS);
    LOG_INFO("Target FPS set to %d", FPS);
MESSAGE_HANDLER_END()

MESSAGE_HANDLER_BEGIN(ToggleFullscreen)
    ToggleFullscreen();
MESSAGE_HANDLER_END()

// Params: Enabled (int) -- 1 to enable, 0 to disable
MESSAGE_HANDLER_BEGIN(SetVsync)
    MH_ExtractDeref(Enabled, int);
    if (Enabled) {
        SetWindowState(FLAG_VSYNC_HINT);
    } else {
        ClearWindowState(FLAG_VSYNC_HINT);
    }
MESSAGE_HANDLER_END()

// Out: Width (int), Height (int), Fullscreen (int), Focused (int), FPS (int)
MESSAGE_HANDLER_BEGIN(GetInfo)
    MH_SetValue(Width, int, GetScreenWidth());
    MH_SetValue(Height, int, GetScreenHeight());
    MH_SetValue(Fullscreen, int, IsWindowFullscreen());
    MH_SetValue(Focused, int, IsWindowFocused());
    MH_SetValue(FPS, int, GetFPS());
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    CAN_RECEIVE_MID(Open)
    CAN_RECEIVE_MID(Close)
    CAN_RECEIVE_MID(SetTitle)
    CAN_RECEIVE_MID(SetSize)
    CAN_RECEIVE_MID(SetTargetFPS)
    CAN_RECEIVE_MID(ToggleFullscreen)
    CAN_RECEIVE_MID(SetVsync)
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
    RECEIVE_MESSAGE_ROUTE(GetInfo)
RECEIVE_MESSAGE_END()

CLASSDEF()

#undef TYPE
