#ifndef INTESTING

#include <stdio.h>

#define VERBOSE

#include <raylib.h>

#include "system/utils.h"
#include "system/object/Self.h"

#include "classes/window.h"
#include "classes/exploder.h"

int main() {
  START_LOGGING("game", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Object_ClassDef());
  RegisterClass(Exploder_ClassDef());
  RegisterClass(Window_ClassDef());
  EndClassRegistrations();

  Window_CreateSingleton();
  TempObjectReference win = GET_SINGLETON(Window);

  SELF_DISPATCH(win, MID_Window_SELF_Open, {
    Payload_SetValue(msg, "Width", int, 640);
    Payload_SetValue(msg, "Height", int, 480);
    Payload_SetValue(msg, "VirtualWidth", int, 160);
    Payload_SetValue(msg, "VirtualHeight", int, 120);
  }, {});

  SELF_DISPATCH(win, MID_Window_SELF_SetTargetFPS, {
    Payload_SetValue(msg, "FPS", int, 120);
  }, {});

  while (!WindowShouldClose()) {
    Window_BeginFrame();

    ClearBackground(RAYWHITE);
    DrawText("Enatrio", 4, 4, 8, DARKGRAY);
    DrawRectangle(60, 40, 40, 40, RED);
    DrawCircle(80, 80, 15, BLUE);

    Window_EndFrame();
  }

  SelfDispatch(win, MID_Window_SELF_Close);
  Window_DestroySingleton();

  return 0;
}

#endif
