#ifndef INTESTING

#include <stdio.h>

#define VERBOSE

#include <raylib.h>

#include "system/utils.h"
#include "system/class/Class.h"

#include "classes/window.h"
#include "classes/exploder.h"

int main() {
  START_LOGGING("game", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Exploder_ClassDef());
  RegisterClass(Window_ClassDef());
  EndClassRegistrations();

  DISPATCH(CID_Exploder, MID_Exploder_ShimmiShimmiYea, {
    Payload_SetValue(msg, "Strength", float, 9.81f);
  }, {});

  DISPATCH(CID_Window, MID_Window_Open, {
    Payload_SetValue(msg, "Width", int, 640);
    Payload_SetValue(msg, "Height", int, 480);
  }, {});

  DISPATCH(CID_Window, MID_Window_SetTargetFPS, {
    Payload_SetValue(msg, "FPS", int, 60);
  }, {});

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Enatrio", 40, 40, 40, DARKGRAY);
    DrawFPS(10, 10);
    EndDrawing();
  }

  Dispatch(CID_Window, MID_Window_Close);

  return 0;
}

#endif