#pragma once

#include <lvgl.h>
#include <stdint.h>

#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

#define LVGL_BACKGROUND lv_color_black()
#define LVGL_FOREGROUND lv_color_white()

struct status_state {
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    uint8_t wpm[10];
#else
    uint16_t apm[10];
    uint8_t wpm[10];
#endif
};

void fill_background(lv_obj_t *canvas);
void init_rect_dsc(lv_draw_rect_dsc_t *rect_dsc, lv_color_t bg_color);
void init_line_dsc(lv_draw_line_dsc_t *line_dsc, lv_color_t color, uint8_t width);
void init_label_dsc(lv_draw_label_dsc_t *label_dsc, lv_color_t color, const lv_font_t *font,
                    lv_text_align_t align);
void canvas_draw_line(lv_obj_t *canvas, const lv_point_t *points, uint32_t point_cnt,
                      lv_draw_line_dsc_t *line_dsc);
void canvas_draw_img(lv_obj_t *canvas, lv_coord_t x, lv_coord_t y, const void *src,
                     const lv_draw_img_dsc_t *img_dsc);
