#pragma once
#ifdef DEBUG

#include "../system/object/ObjectTypes.h"

int EditorInspector_GraphRequested(TempObjectReference *out_target);
void EditorInspector_Update(void);
void EditorInspector_Draw(TempObjectReference selected);

#endif
