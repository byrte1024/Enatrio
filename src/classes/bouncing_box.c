#include "bouncing_box.h"
#include <raylib.h>

#define TYPE BouncingBox

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Create)
    CALL_BASE();
    Self_SetStruct("rect", Rect, {20.0f, 40.0f, 12.0f, 12.0f});
    Self_SetValue("dx", float, 30.0f);
    Self_SetStruct("tint", Tint, {230, 40, 40, 255});
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Update)
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
    Self_SetStruct("rect", Rect, rect);
    Self_SetValue("dx", float, dx);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Render)
    IGNORE_BASE();
    Rect rect = Self_GetDeref("rect", Rect);
    Tint tint = Self_GetDeref("tint", Tint);
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){tint.r, tint.g, tint.b, tint.a});
MESSAGE_HANDLER_END()

CLASSDEF_SPLIT_INHERITS(GameObject)

#undef TYPE
