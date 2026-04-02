#include "cts/UnsafeArray.h"
#ifndef INTESTING

#include <stdio.h>

#define VERBOSE

#include <raylib.h>

#include "utils.h"
#include "cts/class.h"

int main() {
  START_LOGGING("game.log", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();

  EndClassRegistrations();

  return 0;
}

#endif