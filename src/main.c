#if !defined(INTESTING) && !defined(INBENCH)

#define VERBOSE

#include <raylib.h>

#include "system/utils.h"
#include "system/object/Self.h"

#include "classes/window.h"
#include "classes/bouncing_box.h"
#include "classes/spinning_circle.h"
#include "classes/player.h"

#ifdef DEBUG
#include "editor/editor_overlay.h"
#include "editor/editor_tree.h"
#include "editor/editor_inspector.h"
#include "editor/editor_graph.h"
#endif

int main() {
  START_LOGGING("game", LOG_INFO);
  LOG_BUILD_INFO();

  BeginClassRegistrations();
  RegisterClass(Object_ClassDef());
  RegisterClass(GameObject_ClassDef());
  RegisterClass(BouncingBox_ClassDef());
  RegisterClass(SpinningCircle_ClassDef());
  RegisterClass(Player_ClassDef());
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

  ExternalReference scene = GameObject_CreateRootRef(CID_GameObject);
  TempObjectReference root = ObjectContainer_TempFrom(scene);
  GameObject_SetName(root, "Root");

  TempObjectReference box1 = GameObject_CreateChild(root, CID_BouncingBox);
  GameObject_SetName(box1, "RedBox");

  TempObjectReference box2 = GameObject_CreateChild(root, CID_BouncingBox);
  GameObject_SetName(box2, "GreenBox");
  {
      Rect rect = {60.0f, 70.0f, 10.0f, 10.0f};
      Tint tint = {40, 200, 40, 255};
      _Object_StoreValue(box2->data->values, "rect", 4,
                         &rect, sizeof(Rect), CID_BouncingBox, SER_SKIP, 0);
      _Object_StoreValue(box2->data->values, "tint", 4,
                         &tint, sizeof(Tint), CID_BouncingBox, SER_SKIP, 0);
  }

  TempObjectReference player = GameObject_CreateChild(root, CID_Player);
  GameObject_SetName(player, "Player");

  TempObjectReference orbiters = GameObject_CreateChild(root, CID_GameObject);
  GameObject_SetName(orbiters, "Orbiters");

  TempObjectReference c1 = GameObject_CreateChild(orbiters, CID_SpinningCircle);
  GameObject_SetName(c1, "BlueOrbit");

  TempObjectReference c2 = GameObject_CreateChild(orbiters, CID_SpinningCircle);
  GameObject_SetName(c2, "GreenOrbit");
  {
      OrbiterState orb = {80.0f, 60.0f, 15.0f, 0.0f, -3.5f, 4.0f};
      Tint tint = {40, 200, 80, 255};
      _Object_StoreValue(c2->data->values, "orb", 3,
                         &orb, sizeof(OrbiterState), CID_SpinningCircle, SER_SKIP, 0);
      _Object_StoreValue(c2->data->values, "tint", 4,
                         &tint, sizeof(Tint), CID_SpinningCircle, SER_SKIP, 0);
  }

#ifdef DEBUG
  EditorTree_SetRoot(root);
#endif

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

#ifdef DEBUG
    EditorTree_Update();
    EditorInspector_Update();
    EditorGraph_Update();

    {
        TempObjectReference graph_target = NULL;
        if (EditorInspector_GraphRequested(&graph_target)) {
            EditorGraph_Open(graph_target);
        }
    }
#endif

    double t0 = GetTime();
    double t1 = t0;
    double t2 = t0;

#ifdef DEBUG
    int game_paused = EditorOverlay_IsPaused();
    if (!game_paused) {
#endif
    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Update, SPREAD_DOWN, {
      Payload_SetValue(msg, "dt", float, dt);
    }, {});
    t1 = GetTime();

    BeginDrawing();
    ClearBackground(BLACK);

    Window_BeginFrame();
    ClearBackground(RAYWHITE);
    DrawText("Enatrio", 4, 4, 8, DARKGRAY);

    GAMEOBJECT_DISPATCH(root, MID_GameObject_SELF_Render, SPREAD_DOWN, {}, {});

    Window_EndFrame();
    t2 = GetTime();
#ifdef DEBUG
    } else {
        BeginDrawing();
        ClearBackground(BLACK);
        Window_BlitVirtualScreen();
    }
#endif

#ifdef DEBUG
    EditorOverlay_SetUpdateTime((t1 - t0) * 1e6);
    EditorOverlay_SetRenderTime((t2 - t1) * 1e6);
    EditorOverlay_Draw();
    if (EditorOverlay_DumpRequested()) {
        EditorOverlay_DumpScene(&scene);
    }
    EditorTree_Draw();
    EditorInspector_Draw(EditorTree_GetSelected());
    EditorGraph_Draw();
#else
    (void)t0; (void)t1; (void)t2;
#endif

    EndDrawing();
  }

  ObjectContainer_UnRef_External(&scene);

  SelfDispatch(win, MID_Window_SELF_Close);
  Window_DestroySingleton();

  return 0;
}

#endif
