#pragma once

#include "../system/object/Self.h"
#include <raylib.h>
#include <math.h>

// ============================================================
// BouncingBox -- a rectangle that bounces horizontally
// ============================================================

#define TYPE BouncingBox

BEGIN_CLASS(0x2200);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetTransient("x", float, 20.0f);
    Self_SetTransient("y", float, 40.0f);
    Self_SetTransient("dx", float, 30.0f);
    Self_SetTransient("w", float, 12.0f);
    Self_SetTransient("h", float, 12.0f);
    Self_SetTransient("r", int, 230);
    Self_SetTransient("g", int, 40);
    Self_SetTransient("b", int, 40);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    float x = Self_GetDeref("x", float);
    float dx = Self_GetDeref("dx", float);
    float w = Self_GetDeref("w", float);

    x += dx * dt;
    if (x + w > 160.0f || x < 0.0f) {
        dx = -dx;
        if (x + w > 160.0f) x = 160.0f - w;
        if (x < 0.0f) x = 0.0f;
    }
    Self_SetTransient("x", float, x);
    Self_SetTransient("dx", float, dx);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    float x = Self_GetDeref("x", float);
    float y = Self_GetDeref("y", float);
    float w = Self_GetDeref("w", float);
    float h = Self_GetDeref("h", float);
    int r = Self_GetDeref("r", int);
    int g = Self_GetDeref("g", int);
    int b = Self_GetDeref("b", int);
    DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){r, g, b, 255});
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

// ============================================================
// SpinningCircle -- a circle that orbits a center point
// ============================================================

#define TYPE SpinningCircle

BEGIN_CLASS(0x2201);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetTransient("cx", float, 80.0f);
    Self_SetTransient("cy", float, 60.0f);
    Self_SetTransient("orbit", float, 25.0f);
    Self_SetTransient("angle", float, 0.0f);
    Self_SetTransient("speed", float, 2.0f);
    Self_SetTransient("size", float, 6.0f);
    Self_SetTransient("r", int, 40);
    Self_SetTransient("g", int, 80);
    Self_SetTransient("b", int, 230);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    float angle = Self_GetDeref("angle", float);
    float speed = Self_GetDeref("speed", float);
    angle += speed * dt;
    Self_SetTransient("angle", float, angle);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    float cx = Self_GetDeref("cx", float);
    float cy = Self_GetDeref("cy", float);
    float orbit = Self_GetDeref("orbit", float);
    float angle = Self_GetDeref("angle", float);
    float size = Self_GetDeref("size", float);
    int r = Self_GetDeref("r", int);
    int g = Self_GetDeref("g", int);
    int b = Self_GetDeref("b", int);

    float px = cx + orbit * cosf(angle);
    float py = cy + orbit * sinf(angle);
    DrawCircle((int)px, (int)py, size, (Color){r, g, b, 255});
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

// ============================================================
// Player -- arrow-key controlled square
// ============================================================

#define TYPE Player

BEGIN_CLASS(0x2202);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    Self_SetTransient("x", float, 72.0f);
    Self_SetTransient("y", float, 52.0f);
    Self_SetTransient("w", float, 8.0f);
    Self_SetTransient("h", float, 8.0f);
    Self_SetTransient("speed", float, 50.0f);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
    IGNORE_BASE();
    float *_dt = MH_Get(dt, float);
    float dt = _dt ? *_dt : 0.0f;

    float x = Self_GetDeref("x", float);
    float y = Self_GetDeref("y", float);
    float w = Self_GetDeref("w", float);
    float h = Self_GetDeref("h", float);
    float speed = Self_GetDeref("speed", float);

    if (IsKeyDown(KEY_RIGHT)) x += speed * dt;
    if (IsKeyDown(KEY_LEFT))  x -= speed * dt;
    if (IsKeyDown(KEY_DOWN))  y += speed * dt;
    if (IsKeyDown(KEY_UP))    y -= speed * dt;

    if (x < 0.0f) x = 0.0f;
    if (y < 0.0f) y = 0.0f;
    if (x + w > 160.0f) x = 160.0f - w;
    if (y + h > 120.0f) y = 120.0f - h;

    Self_SetTransient("x", float, x);
    Self_SetTransient("y", float, y);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    float x = Self_GetDeref("x", float);
    float y = Self_GetDeref("y", float);
    float w = Self_GetDeref("w", float);
    float h = Self_GetDeref("h", float);
    DrawRectangle((int)x, (int)y, (int)w, (int)h, (Color){255, 255, 255, 255});
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, (Color){40, 40, 40, 255});
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
