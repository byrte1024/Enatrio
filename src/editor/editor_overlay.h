#pragma once
#ifdef DEBUG

#include "../system/object/ObjectTypes.h"

void EditorOverlay_SetUpdateTime(double us);
void EditorOverlay_SetRenderTime(double us);
int EditorOverlay_IsPaused(void);
int EditorOverlay_DumpRequested(void);
int EditorOverlay_LoadRequested(const char **out_path);
void EditorOverlay_DumpScene(ExternalReference *scene_ref);
void EditorOverlay_Draw(void);

#endif
