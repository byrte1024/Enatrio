#pragma once

#include "utils.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
    tests_run++; \
    LOG_INFO("[TEST] %s ...", name); \
} while (0)

#define PASS() do { \
    tests_passed++; \
    LOG_INFO("  PASS"); \
} while (0)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        LOG_ERROR("  FAIL: %s", #cond); \
        LOG_ERROR("    at %s:%d", __FILE__, __LINE__); \
        return; \
    } \
} while (0)
