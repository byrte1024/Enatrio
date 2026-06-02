#ifndef INTESTING

#include <stdio.h>
#include <math.h>

#define VERBOSE

#include <raylib.h>

#include "system/utils.h"
#include "system/object/Self.h"

#include "classes/window.h"
#include "classes/exploder.h"
#include "classes/scene_demo.h"

int main() {
  START_LOGGING("game", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Object_ClassDef());
  RegisterClass(GameObject_ClassDef());
  RegisterClass(BouncingBox_ClassDef());
  RegisterClass(SpinningCircle_ClassDef());
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

  // Build scene graph: root -> box, circle1, circle2
  ExternalReference scene = GameObject_CreateRootRef(CID_GameObject);
  TempObjectReference root = ObjectContainer_TempFrom(scene);

  TempObjectReference box = GameObject_CreateChild(root, CID_BouncingBox);
  (void)box;

  TempObjectReference c1 = GameObject_CreateChild(root, CID_SpinningCircle);
  (void)c1;

  TempObjectReference c2 = GameObject_CreateChild(root, CID_SpinningCircle);
  _Object_StoreValue(c2->data->values, "orbit", 5,
                     &(float){15.0f}, sizeof(float), CID_SpinningCircle, SER_SKIP, 0);
  _Object_StoreValue(c2->data->values, "speed", 5,
                     &(float){-3.5f}, sizeof(float), CID_SpinningCircle, SER_SKIP, 0);
  _Object_StoreValue(c2->data->values, "size", 4,
                     &(float){4.0f}, sizeof(float), CID_SpinningCircle, SER_SKIP, 0);
  _Object_StoreValue(c2->data->values, "r", 1,
                     &(int){40}, sizeof(int), CID_SpinningCircle, SER_SKIP, 0);
  _Object_StoreValue(c2->data->values, "g", 1,
                     &(int){200}, sizeof(int), CID_SpinningCircle, SER_SKIP, 0);
  _Object_StoreValue(c2->data->values, "b", 1,
                     &(int){80}, sizeof(int), CID_SpinningCircle, SER_SKIP, 0);

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
      Payload_SetValue(msg, "dt", float, dt);
    }, {});

    Window_BeginFrame();
    ClearBackground(RAYWHITE);
    DrawText("Enatrio", 4, 4, 8, DARKGRAY);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Render, SPREAD_DOWN, {}, {});

    Window_EndFrame();
  }

  ObjectContainer_UnRef_External(&scene);

  SelfDispatch(win, MID_Window_SELF_Close);
  Window_DestroySingleton();

  return 0;
}

#endif
