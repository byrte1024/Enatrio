#ifdef DEBUG

#include <raylib.h>
#include <stdio.h>
#include "../system/object/Serialization.h"
#include "../system/filedialog.h"

#define EDITOR_HISTORY_SECONDS 20
#define EDITOR_HISTORY_MAX_SAMPLES (EDITOR_HISTORY_SECONDS * 240)

typedef struct {
    float samples[EDITOR_HISTORY_MAX_SAMPLES];
    int head;
    int count;
} EditorTimingRing;

typedef struct {
    EditorTimingRing update;
    EditorTimingRing render;
    EditorTimingRing editor;
    EditorTimingRing fps;
    double pending_editor_us;
    int paused;
    int dump_requested;
    int load_requested;
    char load_path[512];
    char dump_status[128];
    double dump_status_time;
} EditorOverlayState;

static EditorOverlayState _editor_state = {0};

static void _editor_ring_push(EditorTimingRing *ring, float value) {
    ring->samples[ring->head] = value;
    ring->head = (ring->head + 1) % EDITOR_HISTORY_MAX_SAMPLES;
    if (ring->count < EDITOR_HISTORY_MAX_SAMPLES) ring->count++;
}

static float _editor_ring_get(EditorTimingRing *ring, int age) {
    if (age >= ring->count) return 0.0f;
    int idx = (ring->head - 1 - age + EDITOR_HISTORY_MAX_SAMPLES) % EDITOR_HISTORY_MAX_SAMPLES;
    return ring->samples[idx];
}

void EditorOverlay_SetUpdateTime(double us) {
    _editor_ring_push(&_editor_state.update, (float)us);
}

void EditorOverlay_SetRenderTime(double us) {
    _editor_ring_push(&_editor_state.render, (float)us);
}

int EditorOverlay_IsPaused(void) {
    return _editor_state.paused;
}

int EditorOverlay_DumpRequested(void) {
    if (_editor_state.dump_requested) {
        _editor_state.dump_requested = 0;
        return 1;
    }
    return 0;
}

int EditorOverlay_LoadRequested(const char **out_path) {
    if (_editor_state.load_requested) {
        _editor_state.load_requested = 0;
        if (out_path) *out_path = _editor_state.load_path;
        return 1;
    }
    return 0;
}

void EditorOverlay_DumpScene(ExternalReference *scene_ref) {
    if (!scene_ref || !*scene_ref) {
        snprintf(_editor_state.dump_status, sizeof(_editor_state.dump_status),
                 "Dump failed: no scene");
        _editor_state.dump_status_time = GetTime();
        return;
    }

    AppPath_EnsureSubdir("Dumps");

    char filename[640];
    if (AppPath_BuildTimestamped(filename, sizeof(filename), "Dumps", "root", "cob") != 0) {
        snprintf(_editor_state.dump_status, sizeof(_editor_state.dump_status),
                 "Dump failed: no local path");
        _editor_state.dump_status_time = GetTime();
        return;
    }

    int result = Object_SaveToFile(filename, scene_ref, 1);
    if (result != 0) {
        snprintf(_editor_state.dump_status, sizeof(_editor_state.dump_status),
                 "Dump failed: %s", filename);
        _editor_state.dump_status_time = GetTime();
        return;
    }

    snprintf(_editor_state.dump_status, sizeof(_editor_state.dump_status),
             "Dumped -> %s", filename);
    _editor_state.dump_status_time = GetTime();
    LOG_INFO("Scene dump: %s", filename);
}

static void _editor_draw_graph(EditorTimingRing *ring, int x, int y,
                               int w, int h, Color color, const char *unit) {
    DrawRectangle(x, y, w, h, (Color){0, 0, 0, 140});
    DrawRectangleLines(x, y, w, h, (Color){80, 80, 80, 200});

    int sample_count = ring->count < w ? ring->count : w;
    if (sample_count < 2) return;

    float peak = 1.0f;
    for (int i = 0; i < sample_count; i++) {
        float v = _editor_ring_get(ring, i);
        if (v > peak) peak = v;
    }

    for (int i = 0; i < sample_count - 1; i++) {
        float v0 = _editor_ring_get(ring, sample_count - 1 - i);
        float v1 = _editor_ring_get(ring, sample_count - 2 - i);
        float y0 = y + h - 1 - (v0 / peak) * (h - 2);
        float y1 = y + h - 1 - (v1 / peak) * (h - 2);
        float x0 = (float)(x + i);
        float x1 = (float)(x + i + 1);
        DrawLineV((Vector2){x0, y0}, (Vector2){x1, y1}, color);
    }

    char buf[32];
    snprintf(buf, sizeof(buf), "%.0f %s", peak, unit);
    DrawText(buf, x + 2, y + 1, 10, (Color){200, 200, 200, 180});
}

static int _editor_draw_button(int x, int y, int w, int h,
                               const char *label, Color bg, Color text_col) {
    Vector2 mouse = GetMousePosition();
    int hover = (mouse.x >= x && mouse.x <= x + w &&
                 mouse.y >= y && mouse.y <= y + h);
    uint8_t hr = bg.r > 225 ? 255 : bg.r + 30;
    uint8_t hg = bg.g > 225 ? 255 : bg.g + 30;
    uint8_t hb = bg.b > 225 ? 255 : bg.b + 30;
    Color bg_draw = hover ? (Color){hr, hg, hb, bg.a} : bg;
    DrawRectangle(x, y, w, h, bg_draw);
    DrawRectangleLines(x, y, w, h, text_col);
    DrawText(label, x + 4, y + 3, 10, text_col);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

void EditorOverlay_Draw(void) {
    double t_start = GetTime();

    _editor_ring_push(&_editor_state.editor, (float)_editor_state.pending_editor_us);

    int fps = GetFPS();
    _editor_ring_push(&_editor_state.fps, (float)fps);

    char buf[128];

    int graph_w = 200;
    int graph_h = 40;
    int pad = 4;
    int font_size = 14;
    int btn_h = 18;
    int text_x = pad;
    int num_graphs = 4;
    int num_lines = 4;
    int buttons_h = btn_h + pad;
    int base_y = GetScreenHeight() - pad
                 - (graph_h * num_graphs + pad * (num_graphs - 1))
                 - (font_size * num_lines + pad)
                 - buttons_h;

    // -- Buttons --
    {
        const char *pause_label = _editor_state.paused ? "Resume" : "Pause";
        Color pause_bg = _editor_state.paused
            ? (Color){40, 100, 40, 220} : (Color){100, 40, 40, 220};
        Color pause_text = _editor_state.paused
            ? (Color){120, 255, 120, 255} : (Color){255, 120, 120, 255};
        if (_editor_draw_button(text_x, base_y, 56, btn_h, pause_label,
                                pause_bg, pause_text)) {
            _editor_state.paused = !_editor_state.paused;
        }

        if (_editor_draw_button(text_x + 60, base_y, 56, btn_h, "Dump .cob",
                                (Color){40, 60, 100, 220},
                                (Color){140, 180, 255, 255})) {
            _editor_state.dump_requested = 1;
        }

        if (_editor_state.paused) {
            if (_editor_draw_button(text_x + 120, base_y, 56, btn_h, "Load .cob",
                                    (Color){100, 60, 40, 220},
                                    (Color){255, 180, 120, 255})) {
                const char *path = FileDialog_Open(
                    "Load Scene (.cob)", "Dumps", "*.cob", "ECOB files");
                if (path) {
                    snprintf(_editor_state.load_path, sizeof(_editor_state.load_path), "%s", path);
                    _editor_state.load_requested = 1;
                }
            }
        }
    }

    if (_editor_state.paused) {
        int paused_x = _editor_state.paused ? text_x + 180 : text_x + 122;
        DrawText("PAUSED", paused_x, base_y + 4, 10, (Color){255, 80, 80, 255});
    }

    // Dump status message (show for 5 seconds)
    if (_editor_state.dump_status[0] &&
        GetTime() - _editor_state.dump_status_time < 5.0) {
        DrawText(_editor_state.dump_status, text_x, base_y - 14,
                 10, (Color){140, 200, 255, 255});
    }

    int stats_y = base_y + buttons_h;

    snprintf(buf, sizeof(buf), "FPS: %d", fps);
    DrawText(buf, text_x, stats_y, font_size, (Color){255, 255, 100, 255});

    float cur_update = _editor_ring_get(&_editor_state.update, 0);
    snprintf(buf, sizeof(buf), "Update: %.1f us", cur_update);
    DrawText(buf, text_x, stats_y + font_size, font_size, GREEN);

    float cur_render = _editor_ring_get(&_editor_state.render, 0);
    snprintf(buf, sizeof(buf), "Render: %.1f us", cur_render);
    DrawText(buf, text_x, stats_y + font_size * 2, font_size, (Color){100, 200, 255, 255});

    float cur_editor = _editor_ring_get(&_editor_state.editor, 0);
    snprintf(buf, sizeof(buf), "Editor: %.1f us", cur_editor);
    DrawText(buf, text_x, stats_y + font_size * 3, font_size, (Color){255, 200, 80, 255});

    int graph_x = text_x;
    int graph_y = stats_y + font_size * num_lines + pad;
    int step = graph_h + pad;

    _editor_draw_graph(&_editor_state.fps, graph_x, graph_y,
                       graph_w, graph_h, (Color){255, 255, 100, 255}, "fps");
    _editor_draw_graph(&_editor_state.update, graph_x, graph_y + step,
                       graph_w, graph_h, GREEN, "us");
    _editor_draw_graph(&_editor_state.render, graph_x, graph_y + step * 2,
                       graph_w, graph_h, (Color){100, 200, 255, 255}, "us");
    _editor_draw_graph(&_editor_state.editor, graph_x, graph_y + step * 3,
                       graph_w, graph_h, (Color){255, 200, 80, 255}, "us");

    _editor_state.pending_editor_us = (GetTime() - t_start) * 1e6;
}

#endif
