#pragma once

#include "../system/object/Self.h"
#include "scene_types.h"
#include <raylib.h>

#define TYPE BouncingBox

BEGIN_CLASS(0x2200);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetStructTransient("rect", Rect, {20.0f, 40.0f, 12.0f, 12.0f});
    Self_SetTransient("dx", float, 30.0f);
    Self_SetStructTransient("tint", Tint, {230, 40, 40, 255});
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    Rect rect = Self_GetDeref("rect", Rect);
    float dx = Self_GetDeref("dx", float);

    rect.x += dx * dt;
    if (rect.x + rect.w > 160.0f || rect.x < 0.0f) {
        dx = -dx;
        if (rect.x + rect.w > 160.0f) rect.x = 160.0f - rect.w;
        if (rect.x < 0.0f) rect.x = 0.0f;
    }
    Self_SetStructTransient("rect", Rect, rect);
    Self_SetTransient("dx", float, dx);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    Rect rect = Self_GetDeref("rect", Rect);
    Tint tint = Self_GetDeref("tint", Tint);
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){tint.r, tint.g, tint.b, tint.a});
MESSAGE_HANDLER_END()

CAN_RECEIVE_BEGIN()
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Create)
    SELF_CAN_RECEIVE_MID_EXTERN(Object, Destroy)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Update)
    SELF_CAN_RECEIVE_MID_EXTERN(GameObject, Render)
CAN_RECEIVE_END()

RECEIVE_MESSAGE_BEGIN()
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Create)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(Object, Destroy)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Update)
    SELF_RECEIVE_MESSAGE_ROUTE_EXTERN(GameObject, Render)
RECEIVE_MESSAGE_END()

CLASSDEF_INHERITS(GameObject)

#undef TYPE
