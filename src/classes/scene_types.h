#pragma once

#include <stdint.h>

typedef struct {
    float x, y;
    float w, h;
} Rect;

typedef struct {
    uint8_t r, g, b, a;
} Tint;

typedef struct {
    float cx, cy;
    float orbit;
    float angle;
    float speed;
    float size;
} OrbiterState;
