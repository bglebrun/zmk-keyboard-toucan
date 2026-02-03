#include <math.h>
#include <zephyr/kernel.h>
#include "wpm.h"
#include "../assets/custom_fonts.h"
#include "util.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

LV_IMG_DECLARE(grid);

// Chart dimensions (1.5x scale from original)
#define GRID_WIDTH 100
#define GRID_HEIGHT 49
#define GRAPH_HEIGHT 48          // graph fills grid vertically (was 32, x1.5)
#define GRAPH_POINT_SPACING 11.1f // (was 7.4, x1.5)

// Total chart height: grid + gap + labels
#define CHART_HEIGHT 77          // 49 (grid) + 4 (gap) + 12 (label1) + 12 (label2)

// Center on screen
#define CHART_START_X ((SCREEN_WIDTH - GRID_WIDTH) / 2)
#define CHART_START_Y ((SCREEN_HEIGHT - CHART_HEIGHT) / 2)

// Derived positions
#define GRID_X (CHART_START_X)
#define GRID_Y (CHART_START_Y)
#define GRAPH_BASELINE_Y (CHART_START_Y + GRID_HEIGHT) // bottom of grid
#define LABEL_Y (GRAPH_BASELINE_Y + 4)
#define LABEL2_Y (LABEL_Y + 12)

static void draw_needle(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 1);

    int centerX = SCREEN_WIDTH / 2;
    int centerY = CHART_START_Y + 45; // (was +30, x1.5)

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    int value = state->apm[9];
    float max = 0;
    for (int i = 0; i < 10; i++) {
        if (state->apm[i] > max) {
            max = state->apm[i];
        }
    }
#else
    int value = state->wpm[9];
    float max = 0;
    for (int i = 0; i < 10; i++) {
        if (state->wpm[i] > max) {
            max = state->wpm[i];
        }
    }
#endif

    if (max == 0) {
        max = 100;
    }
    if (value < 0) {
        value = 0;
    }
    if (value > max) {
        value = max;
    }

    float radius = 37; // (was 25, x1.5)
    float angleDeg = 180.0f - ((float)value / max) * 180.0f;
    float angleRad = angleDeg * (M_PI / 180.0f);

    int needleEndX = centerX + (int)(radius * cosf(angleRad));
    int needleEndY = centerY - (int)(radius * sinf(angleRad));

    lv_point_t points[2] = {
        {centerX, centerY},
        {needleEndX, needleEndY}
    };

    canvas_draw_line(canvas, points, 2, &line_dsc);
}

static void draw_grid(lv_obj_t *canvas) {
    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);

    canvas_draw_img(canvas, GRID_X, GRID_Y, &grid, &img_dsc);
}

static void draw_graph(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_line_dsc_t line_dsc;
    init_line_dsc(&line_dsc, LVGL_FOREGROUND, 2);
    lv_point_t points[10];

    int max = 0;
    int min;

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    min = 65535;
    for (int i = 0; i < 10; i++) {
        if (state->apm[i] > max) {
            max = state->apm[i];
        }
        if (state->apm[i] < min) {
            min = state->apm[i];
        }
    }
#else
    min = 256;
    for (int i = 0; i < 10; i++) {
        if (state->wpm[i] > max) {
            max = state->wpm[i];
        }
        if (state->wpm[i] < min) {
            min = state->wpm[i];
        }
    }
#endif

    int range = max - min;
    if (range == 0) {
        range = 1;
    }

    for (int i = 0; i < 10; i++) {
        points[i].x = CHART_START_X + (int)(i * GRAPH_POINT_SPACING);
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        points[i].y = GRAPH_BASELINE_Y - ((state->apm[i] - min) * GRAPH_HEIGHT / range);
#else
        points[i].y = GRAPH_BASELINE_Y - ((state->wpm[i] - min) * GRAPH_HEIGHT / range);
#endif
    }

    canvas_draw_line(canvas, points, 10, &line_dsc);
}

static void draw_label(lv_obj_t *canvas, const struct status_state *state) {
    lv_draw_label_dsc_t label_left_dsc;
    init_label_dsc(&label_left_dsc, LVGL_FOREGROUND, &pixel_operator_mono, LV_TEXT_ALIGN_LEFT);

    lv_draw_label_dsc_t label_right_dsc;
    init_label_dsc(&label_right_dsc, LVGL_FOREGROUND, &pixel_operator_mono, LV_TEXT_ALIGN_RIGHT);

#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    lv_canvas_draw_text(canvas, CHART_START_X, LABEL_Y, 25, &label_left_dsc, "APM");

    char apm_text[6] = {};
    snprintf(apm_text, sizeof(apm_text), "%d", state->apm[9]);
    lv_canvas_draw_text(canvas, CHART_START_X + 26, LABEL_Y, 42, &label_right_dsc, apm_text);

    lv_canvas_draw_text(canvas, CHART_START_X, LABEL2_Y, 25, &label_left_dsc, "WPM");

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", state->wpm[9]);
    lv_canvas_draw_text(canvas, CHART_START_X + 26, LABEL2_Y, 42, &label_right_dsc, wpm_text);
#else
    lv_canvas_draw_text(canvas, CHART_START_X, LABEL_Y, 25, &label_left_dsc, "WPM");

    char wpm_text[6] = {};
    snprintf(wpm_text, sizeof(wpm_text), "%d", state->wpm[9]);
    lv_canvas_draw_text(canvas, CHART_START_X + 26, LABEL_Y, 42, &label_right_dsc, wpm_text);
#endif
}

void draw_wpm_status(lv_obj_t *canvas, const struct status_state *state) {
    draw_needle(canvas, state);
    draw_grid(canvas);
    draw_graph(canvas, state);
    draw_label(canvas, state);
}
