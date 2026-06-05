#ifdef DEBUG

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include "../system/object/Self.h"
#include "editor_tree.h"

#define EDITORINSP_PANEL_WIDTH 300
#define EDITORINSP_FONT_SIZE 10
#define EDITORINSP_LINE_HEIGHT 16
#define EDITORINSP_SECTION_GAP 8
#define EDITORINSP_PAD 6
#define EDITORINSP_MAX_REF_LINKS 64

typedef struct {
    int y;
    TempObjectReference target;
} EditorInspRefLink;

typedef struct {
    int visible;
    float scroll_y;
    EditorInspRefLink ref_links[EDITORINSP_MAX_REF_LINKS];
    int ref_link_count;
    int graph_requested;
    TempObjectReference graph_target;
} EditorInspectorState;

static EditorInspectorState _insp_state = {0};

static void _editorinsp_hex_line(char *out, int out_size,
                                 const uint8_t *data, uint32_t size, uint32_t offset) {
    int pos = 0;
    for (uint32_t i = offset; i < size && i < offset + 16 && pos + 3 < out_size; i++) {
        pos += snprintf(out + pos, out_size - pos, "%02X ", data[i]);
    }
    if (pos > 0) out[pos - 1] = '\0';
}

int EditorInspector_GraphRequested(TempObjectReference *out_target) {
    if (_insp_state.graph_requested) {
        _insp_state.graph_requested = 0;
        if (out_target) *out_target = _insp_state.graph_target;
        return 1;
    }
    return 0;
}

void EditorInspector_Update(void) {
    if (IsKeyPressed(KEY_F2)) {
        _insp_state.visible = !_insp_state.visible;
    }
    if (!_insp_state.visible) return;

    int panel_x = GetScreenWidth() - EDITORINSP_PANEL_WIDTH;
    Vector2 mouse = GetMousePosition();

    if (mouse.x >= panel_x) {
        float wheel = GetMouseWheelMove();
        _insp_state.scroll_y -= wheel * EDITORINSP_LINE_HEIGHT * 3;
        if (_insp_state.scroll_y < 0) _insp_state.scroll_y = 0;
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mouse.x >= panel_x) {
        for (int i = 0; i < _insp_state.ref_link_count; i++) {
            EditorInspRefLink *link = &_insp_state.ref_links[i];
            float link_y = (float)link->y - _insp_state.scroll_y;
            if (mouse.y >= link_y && mouse.y < link_y + EDITORINSP_LINE_HEIGHT) {
                if (link->target != NULL) {
                    EditorTree_SelectObject(link->target);
                }
                break;
            }
        }
    }
}

void EditorInspector_Draw(TempObjectReference selected) {
    if (!_insp_state.visible) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    int panel_x = sw - EDITORINSP_PANEL_WIDTH;

    DrawRectangle(panel_x, 0, EDITORINSP_PANEL_WIDTH, sh, (Color){20, 20, 20, 210});
    DrawLine(panel_x, 0, panel_x, sh, (Color){60, 60, 60, 255});

    DrawText("Inspector [F2]", panel_x + EDITORINSP_PAD, 4,
             EDITORINSP_FONT_SIZE, (Color){180, 180, 180, 255});

    if (selected == NULL || selected->data == NULL) {
        DrawText("No object selected", panel_x + EDITORINSP_PAD, 24,
                 EDITORINSP_FONT_SIZE, (Color){120, 120, 120, 255});
        return;
    }

    _insp_state.ref_link_count = 0;

    int content_x = panel_x + EDITORINSP_PAD;
    int content_w = EDITORINSP_PANEL_WIDTH - EDITORINSP_PAD * 2;
    int y = 24;

    BeginScissorMode(panel_x, 20, EDITORINSP_PANEL_WIDTH, sh - 20);

    int draw_y = y - (int)_insp_state.scroll_y;

    // -- Header --
    const char *classname = CLASSID_TOSTRING(selected->cid);

    int is_gameobject = 0;
    {
        ClassID walk = selected->cid;
        while (!CLASSID_ISUNTYPED(walk)) {
            if (walk == CID_GameObject) { is_gameobject = 1; break; }
            walk = ClassDefinitions[walk].parent_cid;
        }
    }

    const char *name = is_gameobject ? GameObject_GetName(selected) : NULL;
    char header[128];
    if (name && name[0] != '\0') {
        snprintf(header, sizeof(header), "%s (%s)", name, classname);
    } else {
        snprintf(header, sizeof(header), "(%s)", classname);
    }
    DrawText(header, content_x, draw_y, EDITORINSP_FONT_SIZE, (Color){255, 255, 100, 255});
    draw_y += EDITORINSP_LINE_HEIGHT;

    if (!is_gameobject) {
        DrawText("* not a GameObject", content_x, draw_y,
                 EDITORINSP_FONT_SIZE, (Color){255, 100, 100, 255});
        draw_y += EDITORINSP_LINE_HEIGHT;
    }

    {
        char info[96];
        snprintf(info, sizeof(info), "CID: 0x%04X  int_refs: %d  ext_refs: %d",
                 selected->cid, selected->internal_refs, selected->external_refs);
        DrawText(info, content_x, draw_y, EDITORINSP_FONT_SIZE, (Color){140, 140, 140, 255});
        draw_y += EDITORINSP_LINE_HEIGHT;

        // Show inheritance chain
        char chain[192] = {0};
        int pos = 0;
        ClassID walk = selected->cid;
        while (!CLASSID_ISUNTYPED(walk) && CLASSID_ISREGISTERED(walk)) {
            if (pos > 0) pos += snprintf(chain + pos, sizeof(chain) - pos, " -> ");
            pos += snprintf(chain + pos, sizeof(chain) - pos, "%s", CLASSID_TOSTRING(walk));
            walk = ClassDefinitions[walk].parent_cid;
        }
        DrawText(chain, content_x, draw_y, EDITORINSP_FONT_SIZE, (Color){110, 110, 110, 255});
        draw_y += EDITORINSP_LINE_HEIGHT;
    }

    // -- Graph button --
    {
        int btn_x = content_x;
        int btn_y_abs = draw_y + 2;
        int btn_w = 50;
        int btn_h = 16;
        DrawRectangle(btn_x, btn_y_abs, btn_w, btn_h, (Color){60, 80, 120, 220});
        DrawRectangleLines(btn_x, btn_y_abs, btn_w, btn_h, (Color){100, 140, 200, 255});
        DrawText("Graph", btn_x + 8, btn_y_abs + 3, EDITORINSP_FONT_SIZE, (Color){200, 220, 255, 255});

        Vector2 mouse = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) &&
            mouse.x >= btn_x && mouse.x <= btn_x + btn_w &&
            mouse.y >= btn_y_abs && mouse.y <= btn_y_abs + btn_h) {
            _insp_state.graph_requested = 1;
            _insp_state.graph_target = selected;
        }
        draw_y += btn_h + 6;
    }

    draw_y += EDITORINSP_SECTION_GAP;

    // -- Values --
    DrawText("Values", content_x, draw_y, EDITORINSP_FONT_SIZE, (Color){100, 200, 255, 255});
    draw_y += EDITORINSP_LINE_HEIGHT;
    DrawLine(content_x, draw_y - 2, content_x + content_w, draw_y - 2, (Color){50, 50, 50, 255});

    {
        UnsafeVariedHashMap *vals = selected->data->values;
        for (uint32_t i = 0; i < vals->bucket_count; i++) {
            UnsafeVariedHashEntry *e = &vals->buckets[i];
            if (e->value < 0) continue;

            UnsafeVariedHashEntryInfo *info =
                (UnsafeVariedHashEntryInfo *)UnsafeArray_GetFast(vals->entries, (uint32_t)e->value);
            uint8_t *raw = (uint8_t *)UnsafeArray_GetFast(vals->data, info->offset);
            uint32_t total_size = info->size;

            uint32_t klen = _HASHKEY_LEN(e->key_len);
            char key_buf[128] = {0};
            uint32_t copy_len = klen < sizeof(key_buf) - 1 ? klen : sizeof(key_buf) - 1;
            memcpy(key_buf, e->key, copy_len);

            uint32_t data_size = total_size > sizeof(ObjectValueHeader)
                                 ? total_size - sizeof(ObjectValueHeader) : 0;
            ObjectValueHeader *hdr = (ObjectValueHeader *)raw;
            uint8_t *data = raw + sizeof(ObjectValueHeader);

            char label[192];
            snprintf(label, sizeof(label), "%s  (%u bytes, ser:%d)",
                     key_buf, data_size, hdr->ser_id);
            DrawText(label, content_x + 4, draw_y, EDITORINSP_FONT_SIZE,
                     (Color){200, 200, 200, 255});
            draw_y += EDITORINSP_LINE_HEIGHT;

            for (uint32_t off = 0; off < data_size; off += 16) {
                char hex[64];
                _editorinsp_hex_line(hex, sizeof(hex), data, data_size, off);
                DrawText(hex, content_x + 12, draw_y, EDITORINSP_FONT_SIZE,
                         (Color){140, 180, 140, 255});
                draw_y += EDITORINSP_LINE_HEIGHT;
            }
        }
    }

    draw_y += EDITORINSP_SECTION_GAP;

    // -- References --
    DrawText("References", content_x, draw_y, EDITORINSP_FONT_SIZE, (Color){255, 180, 80, 255});
    draw_y += EDITORINSP_LINE_HEIGHT;
    DrawLine(content_x, draw_y - 2, content_x + content_w, draw_y - 2, (Color){50, 50, 50, 255});

    {
        UnsafeHashMap *refs = selected->data->references;
        for (uint32_t i = 0; i < refs->bucket_count; i++) {
            UnsafeHashEntry *e = &refs->buckets[i];
            if (e->value < 0) continue;

            uint32_t klen = _HASHKEY_LEN(e->key_len);
            char key_buf[128] = {0};
            uint32_t copy_len = klen < sizeof(key_buf) - 1 ? klen : sizeof(key_buf) - 1;
            memcpy(key_buf, e->key, copy_len);

            ObjectReference *ref_ptr =
                (ObjectReference *)UnsafeArray_GetFast(refs->values, (uint32_t)e->value);
            TempObjectReference target = (ref_ptr && *ref_ptr) ? ObjectContainer_TempFrom(*ref_ptr) : NULL;

            char label[192];
            if (target != NULL && target->data != NULL) {
                const char *tname = GameObject_GetName(target);
                const char *tcls = CLASSID_TOSTRING(target->cid);
                if (tname && tname[0] != '\0') {
                    snprintf(label, sizeof(label), "%s -> %s (%s)", key_buf, tname, tcls);
                } else {
                    snprintf(label, sizeof(label), "%s -> (%s)", key_buf, tcls);
                }
            } else if (target != NULL) {
                snprintf(label, sizeof(label), "%s -> [empty %s]",
                         key_buf, CLASSID_TOSTRING(target->cid));
            } else {
                snprintf(label, sizeof(label), "%s -> NULL", key_buf);
            }

            Color ref_color = target ? (Color){255, 200, 100, 255} : (Color){120, 120, 120, 255};
            DrawText(label, content_x + 4, draw_y, EDITORINSP_FONT_SIZE, ref_color);

            if (target != NULL && _insp_state.ref_link_count < EDITORINSP_MAX_REF_LINKS) {
                _insp_state.ref_links[_insp_state.ref_link_count++] =
                    (EditorInspRefLink){draw_y + (int)_insp_state.scroll_y, target};
            }

            draw_y += EDITORINSP_LINE_HEIGHT;
        }
    }

    EndScissorMode();

    float max_scroll = (float)(draw_y + (int)_insp_state.scroll_y - sh + 20);
    if (max_scroll < 0) max_scroll = 0;
    if (_insp_state.scroll_y > max_scroll) _insp_state.scroll_y = max_scroll;
}

#endif
