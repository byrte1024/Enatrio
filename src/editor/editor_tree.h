#pragma once
#ifdef DEBUG

#include "../system/object/ObjectTypes.h"

void EditorTree_SetRoot(TempObjectReference root);
TempObjectReference EditorTree_GetSelected(void);
void EditorTree_SelectObject(TempObjectReference obj);
void EditorTree_Update(void);
void EditorTree_Draw(void);
const char *_editortree_node_label(TempObjectReference obj, char *buf, int buf_size);

#endif
