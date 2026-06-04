#include "spinning_circle.h"
#include <raylib.h>
#include <math.h>

#define TYPE SpinningCircle

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Create)
    CALL_BASE();
    Self_SetStruct("orb", OrbiterState, {80.0f, 60.0f, 25.0f, 0.0f, 2.0f, 6.0f});
    Self_SetStruct("tint", Tint, {40, 80, 230, 255});
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    OrbiterState orb = Self_GetDeref("orb", OrbiterState);
    orb.angle += orb.speed * dt;
    Self_SetStruct("orb", OrbiterState, orb);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Render)
    IGNORE_BASE();
    OrbiterState orb = Self_GetDeref("orb", OrbiterState);
    Tint tint = Self_GetDeref("tint", Tint);

    float px = orb.cx + orb.orbit * cosf(orb.angle);
    float py = orb.cy + orb.orbit * sinf(orb.angle);
    DrawCircle((int)px, (int)py, orb.size, (Color){tint.r, tint.g, tint.b, tint.a});
MESSAGE_HANDLER_END()

CLASSDEF_SPLIT_INHERITS(GameObject)

#undef TYPE
