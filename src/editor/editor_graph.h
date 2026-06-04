#pragma once

#ifdef DEBUG

#include <raylib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../system/object/Self.h"

static void EditorTree_SelectObject(TempObjectReference obj);

#define EDITORGRAPH_MAX_NODES 128
#define EDITORGRAPH_MAX_EDGES 512
#define EDITORGRAPH_NODE_W 110
#define EDITORGRAPH_NODE_H 36
#define EDITORGRAPH_FONT 10
#define EDITORGRAPH_MARGIN 40

typedef struct {
    TempObjectReference obj;
    float x, y;
    float vx, vy;
} EditorGraphNode;

typedef struct {
    int from;
    int to;
    char key[32];
    int pair_idx;
    int pair_total;
} EditorGraphEdge;

typedef struct {
    int visible;
    TempObjectReference source;
    EditorGraphNode nodes[EDITORGRAPH_MAX_NODES];
    int node_count;
    EditorGraphEdge edges[EDITORGRAPH_MAX_EDGES];
    int edge_count;
    float pan_x, pan_y;
    float zoom;
    int panning;
    float drag_ox, drag_oy;
    float pan_ox, pan_oy;
    int settled;
} EditorGraphState;

static EditorGraphState _graph_state = {.zoom = 1.0f};

// ============================================================
// Graph building (BFS from source)
// ============================================================

static int _editorgraph_find_node(TempObjectReference obj) {
    for (int i = 0; i < _graph_state.node_count; i++) {
        if (_graph_state.nodes[i].obj == obj) return i;
    }
    return -1;
}

static int _editorgraph_add_node(TempObjectReference obj) {
    int idx = _editorgraph_find_node(obj);
    if (idx >= 0) return idx;
    if (_graph_state.node_count >= EDITORGRAPH_MAX_NODES) return -1;
    idx = _graph_state.node_count++;
    _graph_state.nodes[idx].obj = obj;
    _graph_state.nodes[idx].vx = 0;
    _graph_state.nodes[idx].vy = 0;
    _graph_state.nodes[idx].x = 0;
    _graph_state.nodes[idx].y = 0;
    return idx;
}

static void _editorgraph_add_edge(int from, int to, const char *key, uint32_t key_len) {
    if (_graph_state.edge_count >= EDITORGRAPH_MAX_EDGES) return;
    uint32_t copy = key_len < 31 ? key_len : 31;
    for (int i = 0; i < _graph_state.edge_count; i++) {
        if (_graph_state.edges[i].from == from && _graph_state.edges[i].to == to &&
            strncmp(_graph_state.edges[i].key, key, copy) == 0 &&
            _graph_state.edges[i].key[copy] == '\0') return;
    }
    EditorGraphEdge *e = &_graph_state.edges[_graph_state.edge_count++];
    e->from = from;
    e->to = to;
    memcpy(e->key, key, copy);
    e->key[copy] = '\0';
}

static void EditorGraph_Build(TempObjectReference source) {
    _graph_state.node_count = 0;
    _graph_state.edge_count = 0;
    _graph_state.settled = 0;
    _graph_state.source = source;

    if (source == NULL || source->data == NULL) return;

    int queue[EDITORGRAPH_MAX_NODES];
    int depth[EDITORGRAPH_MAX_NODES];
    int q_head = 0, q_tail = 0;

    int src_idx = _editorgraph_add_node(source);
    if (src_idx < 0) return;
    queue[q_tail] = src_idx;
    depth[q_tail] = 0;
    q_tail++;

    while (q_head < q_tail) {
        int cur = queue[q_head];
        int cur_depth = depth[q_head];
        q_head++;
        TempObjectReference obj = _graph_state.nodes[cur].obj;
        if (obj == NULL || obj->data == NULL) continue;

        UnsafeHashMap *refs = obj->data->references;
        for (uint32_t i = 0; i < refs->bucket_count; i++) {
            UnsafeHashEntry *entry = &refs->buckets[i];
            if (entry->value < 0) continue;

            ObjectReference *ref_ptr =
                (ObjectReference *)UnsafeArray_GetFast(refs->values, (uint32_t)entry->value);
            if (ref_ptr == NULL || *ref_ptr == NULL) continue;
            TempObjectReference target = ObjectContainer_TempFrom(*ref_ptr);
            if (target->data == NULL) continue;

            int prev_count = _graph_state.node_count;
            int tidx = _editorgraph_add_node(target);
            if (tidx < 0) continue;

            uint32_t klen = _HASHKEY_LEN(entry->key_len);
            _editorgraph_add_edge(cur, tidx, (const char *)entry->key, klen);

            if (_graph_state.node_count > prev_count && q_tail < EDITORGRAPH_MAX_NODES) {
                queue[q_tail] = tidx;
                depth[q_tail] = cur_depth + 1;
                q_tail++;
            }
        }
    }

    // Initial placement: arrange by BFS depth in horizontal layers
    int max_depth = 0;
    for (int i = 0; i < q_tail; i++) {
        if (depth[i] > max_depth) max_depth = depth[i];
    }

    int layer_count[EDITORGRAPH_MAX_NODES];
    int layer_idx[EDITORGRAPH_MAX_NODES];
    memset(layer_count, 0, sizeof(layer_count));

    // Build a node->depth map (queue order matches node indices for new nodes)
    int node_depth[EDITORGRAPH_MAX_NODES];
    memset(node_depth, 0, sizeof(node_depth));
    for (int i = 0; i < q_tail; i++) {
        node_depth[queue[i]] = depth[i];
        layer_count[depth[i]]++;
    }

    memset(layer_idx, 0, sizeof(layer_idx));
    float row_spacing = 180.0f;
    float col_spacing = 160.0f;

    for (int i = 0; i < _graph_state.node_count; i++) {
        int d = node_depth[i];
        int count_in_layer = layer_count[d];
        int idx_in_layer = layer_idx[d]++;
        float layer_width = (count_in_layer - 1) * col_spacing;
        _graph_state.nodes[i].x = -layer_width * 0.5f + idx_in_layer * col_spacing;
        _graph_state.nodes[i].y = (d - max_depth * 0.5f) * row_spacing;
    }

    // Precompute edge pair indices for parallel edge rendering
    for (int i = 0; i < _graph_state.edge_count; i++) {
        EditorGraphEdge *e = &_graph_state.edges[i];
        int pmin = e->from < e->to ? e->from : e->to;
        int pmax = e->from < e->to ? e->to : e->from;
        int idx_in_pair = 0;
        int total = 0;
        for (int j = 0; j < _graph_state.edge_count; j++) {
            int jmin = _graph_state.edges[j].from < _graph_state.edges[j].to
                       ? _graph_state.edges[j].from : _graph_state.edges[j].to;
            int jmax = _graph_state.edges[j].from < _graph_state.edges[j].to
                       ? _graph_state.edges[j].to : _graph_state.edges[j].from;
            if (jmin == pmin && jmax == pmax) {
                if (j < i) idx_in_pair++;
                total++;
            }
        }
        e->pair_idx = idx_in_pair;
        e->pair_total = total;
    }
}

// ============================================================
// Force-directed layout
// ============================================================

static float _editorgraph_point_seg_dist2(float px, float py,
                                          float ax, float ay, float bx, float by,
                                          float *closest_x, float *closest_y) {
    float dx = bx - ax, dy = by - ay;
    float len2 = dx * dx + dy * dy;
    float t = 0.0f;
    if (len2 > 0.001f) {
        t = ((px - ax) * dx + (py - ay) * dy) / len2;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;
    }
    *closest_x = ax + t * dx;
    *closest_y = ay + t * dy;
    float ex = px - *closest_x, ey = py - *closest_y;
    return ex * ex + ey * ey;
}

static void _editorgraph_simulate(void) {
    if (_graph_state.node_count < 2 || _graph_state.settled) return;

    float k_rep = 30000.0f;
    float k_att = 0.004f;
    float rest_len = 180.0f;
    float k_center = 0.0008f;
    float k_edge_rep = 8000.0f;
    float edge_rep_thresh = 120.0f;
    float damping = 0.82f;
    float dt = 1.0f;
    float max_velocity = 0.3f;

    for (int i = 0; i < _graph_state.node_count; i++) {
        float fx = 0, fy = 0;
        EditorGraphNode *a = &_graph_state.nodes[i];

        // Node-node repulsion
        for (int j = 0; j < _graph_state.node_count; j++) {
            if (i == j) continue;
            EditorGraphNode *b = &_graph_state.nodes[j];
            float dx = a->x - b->x;
            float dy = a->y - b->y;
            float dist2 = dx * dx + dy * dy;
            if (dist2 < 1.0f) dist2 = 1.0f;
            float f = k_rep / dist2;
            float dist = sqrtf(dist2);
            fx += f * dx / dist;
            fy += f * dy / dist;
        }

        // Edge attraction
        for (int e = 0; e < _graph_state.edge_count; e++) {
            int other = -1;
            if (_graph_state.edges[e].from == i) other = _graph_state.edges[e].to;
            else if (_graph_state.edges[e].to == i) other = _graph_state.edges[e].from;
            if (other < 0) continue;

            EditorGraphNode *b = &_graph_state.nodes[other];
            float dx = b->x - a->x;
            float dy = b->y - a->y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < 0.1f) dist = 0.1f;
            float f = k_att * (dist - rest_len);
            fx += f * dx / dist;
            fy += f * dy / dist;
        }

        // Edge-node repulsion: push node away from edges it's not part of
        for (int e = 0; e < _graph_state.edge_count; e++) {
            if (_graph_state.edges[e].from == i || _graph_state.edges[e].to == i) continue;
            EditorGraphNode *ea = &_graph_state.nodes[_graph_state.edges[e].from];
            EditorGraphNode *eb = &_graph_state.nodes[_graph_state.edges[e].to];
            float cpx, cpy;
            float d2 = _editorgraph_point_seg_dist2(a->x, a->y, ea->x, ea->y,
                                                     eb->x, eb->y, &cpx, &cpy);
            float thresh2 = edge_rep_thresh * edge_rep_thresh;
            if (d2 < thresh2 && d2 > 0.1f) {
                float dist = sqrtf(d2);
                float f = k_edge_rep / d2;
                fx += f * (a->x - cpx) / dist;
                fy += f * (a->y - cpy) / dist;
            }
        }

        // Center gravity
        fx -= k_center * a->x;
        fy -= k_center * a->y;

        a->vx = (a->vx + fx * dt) * damping;
        a->vy = (a->vy + fy * dt) * damping;
    }

    float total_v = 0;
    for (int i = 0; i < _graph_state.node_count; i++) {
        EditorGraphNode *n = &_graph_state.nodes[i];
        float speed = sqrtf(n->vx * n->vx + n->vy * n->vy);
        total_v += speed;
        n->x += n->vx;
        n->y += n->vy;
    }

    if (total_v / _graph_state.node_count < max_velocity) {
        _graph_state.settled = 1;
    }
}

// ============================================================
// Drawing helpers
// ============================================================

static void _editorgraph_arrow(float x1, float y1, float x2, float y2,
                               float zoom, Color color) {
    DrawLineV((Vector2){x1, y1}, (Vector2){x2, y2}, color);

    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f) return;
    dx /= len;
    dy /= len;

    float sz = 8.0f * zoom;
    if (sz < 4.0f) sz = 4.0f;
    float px = x2 - dx * sz;
    float py = y2 - dy * sz;
    float half = sz * 0.4f;
    Vector2 tip = {x2, y2};
    Vector2 left = {px - dy * half, py + dx * half};
    Vector2 right = {px + dy * half, py - dx * half};

    // Raylib requires CCW winding; compute cross product to pick correct order
    float cross = (left.x - tip.x) * (right.y - tip.y) - (left.y - tip.y) * (right.x - tip.x);
    if (cross > 0)
        DrawTriangle(tip, left, right, color);
    else
        DrawTriangle(tip, right, left, color);
}

// Uses _editortree_node_label from editor_tree.h (included before this file)

static void _editorgraph_edge_clip(float cx, float cy, float tx, float ty,
                                   float hw, float hh, float *ox, float *oy) {
    float dx = tx - cx;
    float dy = ty - cy;
    if (fabsf(dx) < 0.001f && fabsf(dy) < 0.001f) { *ox = cx; *oy = cy; return; }

    float sx = (fabsf(dx) > 0.001f) ? hw / fabsf(dx) : 1e6f;
    float sy = (fabsf(dy) > 0.001f) ? hh / fabsf(dy) : 1e6f;
    float s = sx < sy ? sx : sy;
    *ox = cx + dx * s;
    *oy = cy + dy * s;
}

// ============================================================
// Public API
// ============================================================

static void EditorGraph_Open(TempObjectReference obj) {
    _graph_state.visible = 1;
    _graph_state.pan_x = 0;
    _graph_state.pan_y = 0;
    _graph_state.zoom = 1.0f;
    EditorGraph_Build(obj);
}

static void EditorGraph_Update(void) {
    if (IsKeyPressed(KEY_F3)) {
        _graph_state.visible = !_graph_state.visible;
    }
    if (!_graph_state.visible) return;

    _editorgraph_simulate();

    Vector2 mouse = GetMousePosition();

    // Zoom toward mouse position
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        float old_zoom = _graph_state.zoom;
        _graph_state.zoom *= (wheel > 0) ? 1.1f : 0.9f;
        if (_graph_state.zoom < 0.15f) _graph_state.zoom = 0.15f;
        if (_graph_state.zoom > 5.0f) _graph_state.zoom = 5.0f;

        float scale_change = _graph_state.zoom / old_zoom;
        float scx = (float)GetScreenWidth() * 0.5f;
        float scy = (float)GetScreenHeight() * 0.5f;
        _graph_state.pan_x = (mouse.x - scx) + (_graph_state.pan_x - (mouse.x - scx)) * scale_change;
        _graph_state.pan_y = (mouse.y - scy) + (_graph_state.pan_y - (mouse.y - scy)) * scale_change;
    }

    // Pan with right-click drag
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        _graph_state.panning = 1;
        _graph_state.drag_ox = mouse.x;
        _graph_state.drag_oy = mouse.y;
        _graph_state.pan_ox = _graph_state.pan_x;
        _graph_state.pan_oy = _graph_state.pan_y;
    }
    if (_graph_state.panning) {
        if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
            _graph_state.pan_x = _graph_state.pan_ox + (mouse.x - _graph_state.drag_ox);
            _graph_state.pan_y = _graph_state.pan_oy + (mouse.y - _graph_state.drag_oy);
        } else {
            _graph_state.panning = 0;
        }
    }

    // Left-click: select node + rebuild graph from it
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        float z = _graph_state.zoom;
        float cx = (float)GetScreenWidth() * 0.5f + _graph_state.pan_x;
        float cy = (float)GetScreenHeight() * 0.5f + _graph_state.pan_y;
        float nw = EDITORGRAPH_NODE_W * z;
        float nh = EDITORGRAPH_NODE_H * z;

        for (int i = 0; i < _graph_state.node_count; i++) {
            EditorGraphNode *n = &_graph_state.nodes[i];
            float nx = cx + n->x * z - nw * 0.5f;
            float ny = cy + n->y * z - nh * 0.5f;
            if (mouse.x >= nx && mouse.x <= nx + nw &&
                mouse.y >= ny && mouse.y <= ny + nh) {
                EditorTree_SelectObject(n->obj);
                EditorGraph_Build(n->obj);
                break;
            }
        }
    }
}

static void EditorGraph_Draw(void) {
    if (!_graph_state.visible) return;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, (Color){10, 10, 15, 220});

    {
        char hud[96];
        snprintf(hud, sizeof(hud),
                 "Reference Graph [F3]  (scroll=zoom, right-drag=pan, click=refocus)  x%.1f",
                 _graph_state.zoom);
        DrawText(hud, 8, 4, EDITORGRAPH_FONT, (Color){160, 160, 160, 255});
    }

    if (_graph_state.node_count == 0) {
        DrawText("No graph data", sw / 2 - 40, sh / 2, EDITORGRAPH_FONT,
                 (Color){120, 120, 120, 255});
        return;
    }

    float z = _graph_state.zoom;
    float cx = (float)sw * 0.5f + _graph_state.pan_x;
    float cy = (float)sh * 0.5f + _graph_state.pan_y;
    float nw = EDITORGRAPH_NODE_W * z;
    float nh = EDITORGRAPH_NODE_H * z;
    float hw = nw * 0.5f;
    float hh = nh * 0.5f;
    int font_main = (int)(EDITORGRAPH_FONT * z);
    int font_small = (int)((EDITORGRAPH_FONT - 2) * z);
    if (font_main < 4) font_main = 4;
    if (font_small < 3) font_small = 3;

    // Edges -- offset both the line and label for parallel/bidirectional edges
    for (int i = 0; i < _graph_state.edge_count; i++) {
        EditorGraphEdge *e = &_graph_state.edges[i];
        if (e->from < 0 || e->from >= _graph_state.node_count) continue;
        if (e->to < 0 || e->to >= _graph_state.node_count) continue;

        EditorGraphNode *a = &_graph_state.nodes[e->from];
        EditorGraphNode *b = &_graph_state.nodes[e->to];

        float ax = cx + a->x * z, ay = cy + a->y * z;
        float bx = cx + b->x * z, by = cy + b->y * z;

        int pair_min = e->from < e->to ? e->from : e->to;
        int pair_max = e->from < e->to ? e->to : e->from;
        int pair_idx = e->pair_idx;
        int pair_total = e->pair_total;

        EditorGraphNode *lo = &_graph_state.nodes[pair_min];
        EditorGraphNode *hi = &_graph_state.nodes[pair_max];
        float edx = (cx + hi->x * z) - (cx + lo->x * z);
        float edy = (cy + hi->y * z) - (cy + lo->y * z);
        float elen = sqrtf(edx * edx + edy * edy);
        float perp_x = 0, perp_y = 0;
        if (elen > 0.1f) { perp_x = -edy / elen; perp_y = edx / elen; }

        float spacing = 12.0f * z;
        float offset = (pair_idx - (pair_total - 1) * 0.5f) * spacing;

        float a_off_x = ax + perp_x * offset;
        float a_off_y = ay + perp_y * offset;
        float b_off_x = bx + perp_x * offset;
        float b_off_y = by + perp_y * offset;

        float ox1, oy1, ox2, oy2;
        _editorgraph_edge_clip(a_off_x, a_off_y, b_off_x, b_off_y, hw, hh, &ox1, &oy1);
        _editorgraph_edge_clip(b_off_x, b_off_y, a_off_x, a_off_y, hw, hh, &ox2, &oy2);

        _editorgraph_arrow(ox1, oy1, ox2, oy2, z, (Color){100, 140, 180, 200});

        // Label at midpoint of the offset line
        float lx = (ox1 + ox2) * 0.5f + perp_x * 6.0f * z;
        float ly = (oy1 + oy2) * 0.5f + perp_y * 6.0f * z;
        DrawText(e->key, (int)lx, (int)ly - font_small / 2,
                 font_small, (Color){90, 120, 150, 180});
    }

    // Nodes
    for (int i = 0; i < _graph_state.node_count; i++) {
        EditorGraphNode *n = &_graph_state.nodes[i];
        float npx = cx + n->x * z - hw;
        float npy = cy + n->y * z - hh;

        int is_source = (n->obj == _graph_state.source);
        Color bg = is_source ? (Color){50, 60, 90, 230} : (Color){35, 35, 40, 230};
        Color border = is_source ? (Color){100, 140, 200, 255} : (Color){70, 70, 80, 255};

        DrawRectangle((int)npx, (int)npy, (int)nw, (int)nh, bg);
        DrawRectangleLines((int)npx, (int)npy, (int)nw, (int)nh, border);

        char label[64];
        _editortree_node_label(n->obj, label, sizeof(label));
        DrawText(label, (int)npx + (int)(4 * z), (int)npy + (int)(4 * z),
                 font_main, (Color){220, 220, 220, 255});

        const char *cls = CLASSID_TOSTRING(n->obj->cid);
        DrawText(cls, (int)npx + (int)(4 * z), (int)npy + (int)(4 * z) + font_main + (int)(2 * z),
                 font_small, (Color){120, 120, 140, 255});

        if (n->obj->external_refs > 0) {
            char ext[16];
            snprintf(ext, sizeof(ext), "E:%d", n->obj->external_refs);
            int ex = (int)(npx + nw) - (int)(24 * z);
            int ey = (int)npy + (int)(2 * z);
            DrawRectangle(ex - (int)(2 * z), ey - 1, (int)(26 * z), (int)(12 * z),
                          (Color){180, 80, 40, 200});
            DrawText(ext, ex, ey, font_small, (Color){255, 220, 180, 255});
        }

        if (n->obj->internal_refs > 0) {
            char iref[16];
            snprintf(iref, sizeof(iref), "I:%d", n->obj->internal_refs);
            int ix = (int)(npx + nw) - (int)(24 * z);
            int iy = (int)(npy + nh) - (int)(12 * z);
            DrawText(iref, ix, iy, font_small, (Color){140, 140, 160, 200});
        }
    }
}

#endif
