#include "player.h"
#include <raylib.h>

#define TYPE Player

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Create)
    CALL_BASE();
    Self_SetStruct("rect", Rect, {72.0f, 52.0f, 8.0f, 8.0f});
    Self_SetValue("speed", float, 50.0f);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    Rect rect = Self_GetDeref("rect", Rect);
    float speed = Self_GetDeref("speed", float);

    if (IsKeyDown(KEY_RIGHT)) rect.x += speed * dt;
    if (IsKeyDown(KEY_LEFT))  rect.x -= speed * dt;
    if (IsKeyDown(KEY_DOWN))  rect.y += speed * dt;
    if (IsKeyDown(KEY_UP))    rect.y -= speed * dt;

    if (rect.x < 0.0f) rect.x = 0.0f;
    if (rect.y < 0.0f) rect.y = 0.0f;
    if (rect.x + rect.w > 160.0f) rect.x = 160.0f - rect.w;
    if (rect.y + rect.h > 120.0f) rect.y = 120.0f - rect.h;

    Self_SetStruct("rect", Rect, rect);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN_SPLIT(GameObject, Render)
    IGNORE_BASE();
    Rect rect = Self_GetDeref("rect", Rect);
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){255, 255, 255, 255});
    DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){40, 40, 40, 255});
MESSAGE_HANDLER_END()

CLASSDEF_SPLIT_INHERITS(GameObject)

#undef TYPE
