#pragma once

#include "../system/object/Self.h"
#include "scene_types.h"
#include <raylib.h>
#include <math.h>

#define TYPE SpinningCircle

BEGIN_CLASS(0x2201);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetStruct("orb", OrbiterState, {80.0f, 60.0f, 25.0f, 0.0f, 2.0f, 6.0f});
    Self_SetStruct("tint", Tint, {40, 80, 230, 255});
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    OrbiterState orb = Self_GetDeref("orb", OrbiterState);
    orb.angle += orb.speed * dt;
    Self_SetStruct("orb", OrbiterState, orb);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    OrbiterState orb = Self_GetDeref("orb", OrbiterState);
    Tint tint = Self_GetDeref("tint", Tint);

    float px = orb.cx + orb.orbit * cosf(orb.angle);
    float py = orb.cy + orb.orbit * sinf(orb.angle);
    DrawCircle((int)px, (int)py, orb.size, (Color){tint.r, tint.g, tint.b, tint.a});
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
