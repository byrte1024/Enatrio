#pragma once
#ifdef DEBUG

#include "../system/object/ObjectTypes.h"

void EditorGraph_Open(TempObjectReference obj);
void EditorGraph_Build(TempObjectReference source);
void EditorGraph_Update(void);
void EditorGraph_Draw(void);

#endif
