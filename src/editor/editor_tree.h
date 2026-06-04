#pragma once

#ifdef DEBUG

#include <raylib.h>
#include <stdio.h>
#include "../system/object/Self.h"

#define EDITORTREE_PANEL_WIDTH 250
#define EDITORTREE_ITEM_HEIGHT 20
#define EDITORTREE_INDENT 16
#define EDITORTREE_MAX_COLLAPSED 256
#define EDITORTREE_MAX_VISIBLE 512
#define EDITORTREE_FONT_SIZE 10
#define EDITORTREE_BTN_SIZE 12

typedef struct {
    TempObjectReference obj;
    int depth;
} EditorTreeEntry;

typedef struct {
    int visible;
    float scroll_y;
    TempObjectReference root;
    TempObjectReference selected;
    TempObjectReference collapsed[EDITORTREE_MAX_COLLAPSED];
    int collapsed_count;
    EditorTreeEntry cached_entries[EDITORTREE_MAX_VISIBLE];
    int cached_count;
    int cache_dirty;
} EditorTreeState;

static EditorTreeState _tree_state = {.cache_dirty = 1};

static void EditorTree_SetRoot(TempObjectReference root) {
    _tree_state.root = root;
}

static TempObjectReference EditorTree_GetSelected(void) {
    return _tree_state.selected;
}

static int _editortree_is_collapsed(TempObjectReference obj) {
    for (int i = 0; i < _tree_state.collapsed_count; i++) {
        if (_tree_state.collapsed[i] == obj) return 1;
    }
    return 0;
}

static void _editortree_toggle_collapsed(TempObjectReference obj) {
    for (int i = 0; i < _tree_state.collapsed_count; i++) {
        if (_tree_state.collapsed[i] == obj) {
            _tree_state.collapsed[i] = _tree_state.collapsed[--_tree_state.collapsed_count];
            _tree_state.cache_dirty = 1;
            return;
        }
    }
    if (_tree_state.collapsed_count < EDITORTREE_MAX_COLLAPSED) {
        _tree_state.collapsed[_tree_state.collapsed_count++] = obj;
        _tree_state.cache_dirty = 1;
    }
}

static int _editortree_flatten(TempObjectReference node, int depth,
                               EditorTreeEntry *out, int max_entries, int count) {
    if (node == NULL || node->data == NULL || count >= max_entries) return count;
    out[count++] = (EditorTreeEntry){node, depth};

    if (_editortree_is_collapsed(node)) return count;

    void *cc_ptr = _Object_GetValueData(node->data->values, "child_count", 11);
    int child_count = cc_ptr ? *(int *)cc_ptr : 0;

    for (int i = 0; i < child_count && count < max_entries; i++) {
        char kbuf[_GO_CHILD_KEY_MAX];
        uint32_t klen = _go_child_key(kbuf, i);
        TempObjectReference child = Object_GetRef(node, kbuf, klen);
        if (child != NULL) {
            count = _editortree_flatten(child, depth + 1, out, max_entries, count);
        }
    }
    return count;
}

static void _editortree_refresh_cache(void) {
    if (!_tree_state.cache_dirty) return;
    _tree_state.cached_count = _editortree_flatten(
        _tree_state.root, 0, _tree_state.cached_entries,
        EDITORTREE_MAX_VISIBLE, 0);
    _tree_state.cache_dirty = 0;
}

static const char *_editortree_node_label(TempObjectReference obj, char *buf, int buf_size) {
    const char *name = GameObject_GetName(obj);
    const char *classname = CLASSID_TOSTRING(obj->cid);

    if (name && name[0] != '\0') {
        snprintf(buf, buf_size, "%s", name);
    } else {
        snprintf(buf, buf_size, "(%s)", classname);
    }
    return buf;
}

static void EditorTree_SelectObject(TempObjectReference obj) {
    _tree_state.selected = obj;
}

static void EditorTree_Update(void) {
    if (IsKeyPressed(KEY_F1)) {
        _tree_state.visible = !_tree_state.visible;
    }
    if (!_tree_state.visible) return;

    _tree_state.cache_dirty = 1;
    _editortree_refresh_cache();

    Vector2 mouse = GetMousePosition();
    if (mouse.x < EDITORTREE_PANEL_WIDTH) {
        float wheel = GetMouseWheelMove();
        _tree_state.scroll_y -= wheel * EDITORTREE_ITEM_HEIGHT * 3;
        if (_tree_state.scroll_y < 0) _tree_state.scroll_y = 0;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x < EDITORTREE_PANEL_WIDTH) {
        int count = _tree_state.cached_count;

        int handled = 0;
        for (int i = 0; i < count; i++) {
            float item_y = 24.0f + i * EDITORTREE_ITEM_HEIGHT - _tree_state.scroll_y;
            if (item_y < 20 || item_y > GetScreenHeight()) continue;

            float btn_x = (float)(4 + _tree_state.cached_entries[i].depth * EDITORTREE_INDENT);
            float btn_y = item_y + (EDITORTREE_ITEM_HEIGHT - EDITORTREE_BTN_SIZE) * 0.5f;

            if (mouse.x >= btn_x && mouse.x <= btn_x + EDITORTREE_BTN_SIZE &&
                mouse.y >= btn_y && mouse.y <= btn_y + EDITORTREE_BTN_SIZE) {
                _editortree_toggle_collapsed(_tree_state.cached_entries[i].obj);
                handled = 1;
                break;
            }

            if (mouse.y >= item_y && mouse.y < item_y + EDITORTREE_ITEM_HEIGHT &&
                mouse.x < EDITORTREE_PANEL_WIDTH) {
                _tree_state.selected = _tree_state.cached_entries[i].obj;
                handled = 1;
                break;
            }
        }
        (void)handled;
    }
}

static void EditorTree_Draw(void) {
    if (!_tree_state.visible || _tree_state.root == NULL) return;

    int panel_h = GetScreenHeight();
    DrawRectangle(0, 0, EDITORTREE_PANEL_WIDTH, panel_h, (Color){20, 20, 20, 210});
    DrawLine(EDITORTREE_PANEL_WIDTH, 0, EDITORTREE_PANEL_WIDTH, panel_h,
             (Color){60, 60, 60, 255});

    DrawText("Scene Tree [F1]", 4, 4, EDITORTREE_FONT_SIZE, (Color){180, 180, 180, 255});

    _editortree_refresh_cache();
    int count = _tree_state.cached_count;

    float max_scroll = (float)(count * EDITORTREE_ITEM_HEIGHT) - (panel_h - 40);
    if (max_scroll < 0) max_scroll = 0;
    if (_tree_state.scroll_y > max_scroll) _tree_state.scroll_y = max_scroll;

    BeginScissorMode(0, 20, EDITORTREE_PANEL_WIDTH, panel_h - 20);

    for (int i = 0; i < count; i++) {
        float item_y = 24.0f + i * EDITORTREE_ITEM_HEIGHT - _tree_state.scroll_y;
        if (item_y + EDITORTREE_ITEM_HEIGHT < 20 || item_y > panel_h) continue;

        TempObjectReference obj = _tree_state.cached_entries[i].obj;
        int depth = _tree_state.cached_entries[i].depth;
        int is_selected = (obj == _tree_state.selected);

        if (is_selected) {
            DrawRectangle(0, (int)item_y, EDITORTREE_PANEL_WIDTH,
                          EDITORTREE_ITEM_HEIGHT, (Color){60, 80, 120, 180});
        }

        int x_offset = 4 + depth * EDITORTREE_INDENT;
        float btn_y = item_y + (EDITORTREE_ITEM_HEIGHT - EDITORTREE_BTN_SIZE) * 0.5f;

        void *cc_ptr = _Object_GetValueData(obj->data->values, "child_count", 11);
        int child_count = cc_ptr ? *(int *)cc_ptr : 0;

        if (child_count > 0) {
            int collapsed = _editortree_is_collapsed(obj);
            Color btn_color = {120, 120, 120, 255};
            DrawRectangleLines(x_offset, (int)btn_y, EDITORTREE_BTN_SIZE,
                               EDITORTREE_BTN_SIZE, btn_color);
            const char *sym = collapsed ? "+" : "-";
            DrawText(sym, x_offset + 3, (int)btn_y + 1, EDITORTREE_FONT_SIZE, btn_color);
        }

        int text_x = x_offset + EDITORTREE_BTN_SIZE + 4;
        char label[128];
        _editortree_node_label(obj, label, sizeof(label));

        Color text_color = {210, 210, 210, 255};
        if (is_selected) text_color = (Color){255, 255, 255, 255};
        else if (depth == 0) text_color = (Color){255, 255, 100, 255};
        DrawText(label, text_x, (int)item_y + 4, EDITORTREE_FONT_SIZE, text_color);
    }

    EndScissorMode();
}

#endif
