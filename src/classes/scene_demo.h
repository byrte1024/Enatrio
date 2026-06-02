#pragma once

#include "../system/object/Self.h"
#include <raylib.h>
#include <math.h>

// ============================================================
// Shared structs for packed Self values
// ============================================================

typedef struct {
    float x, y;
    float w, h;
} Rect;

typedef struct {
    uint8_t r, g, b, a;
} Tint;

// ============================================================
// BouncingBox -- a rectangle that bounces horizontally
// ============================================================

#define TYPE BouncingBox

BEGIN_CLASS(0x2200);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    { Rect _r = {20.0f, 40.0f, 12.0f, 12.0f}; Self_Set("rect", &_r, sizeof(Rect)); }
    Self_SetTransient("dx", float, 30.0f);
    { Tint _t = {230, 40, 40, 255}; Self_Set("tint", &_t, sizeof(Tint)); }
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
    Self_Set("rect", &rect, sizeof(Rect));
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

// ============================================================
// SpinningCircle -- a circle that orbits a center point
// ============================================================

typedef struct {
    float cx, cy;
    float orbit;
    float angle;
    float speed;
    float size;
} OrbiterState;

#define TYPE SpinningCircle

BEGIN_CLASS(0x2201);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    { OrbiterState _o = {80.0f, 60.0f, 25.0f, 0.0f, 2.0f, 6.0f}; Self_Set("orb", &_o, sizeof(OrbiterState)); }
    { Tint _t = {40, 80, 230, 255}; Self_Set("tint", &_t, sizeof(Tint)); }
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
    Self_Set("orb", &orb, sizeof(OrbiterState));
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

// ============================================================
// Player -- arrow-key controlled square
// ============================================================

#define TYPE Player

BEGIN_CLASS(0x2202);
INHERITS(GameObject);

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Create)
    CALL_BASE();
    { Rect _r = {72.0f, 52.0f, 8.0f, 8.0f}; Self_Set("rect", &_r, sizeof(Rect)); }
    Self_SetTransient("speed", float, 50.0f);
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(Object, Destroy)
    CALL_BASE();
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Update)
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

    Self_Set("rect", &rect, sizeof(Rect));
MESSAGE_HANDLER_END()

SELF_MESSAGE_HANDLER_BEGIN_EXTERN(GameObject, Render)
    IGNORE_BASE();
    Rect rect = Self_GetDeref("rect", Rect);
    DrawRectangle((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){255, 255, 255, 255});
    DrawRectangleLines((int)rect.x, (int)rect.y, (int)rect.w, (int)rect.h,
        (Color){40, 40, 40, 255});
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
