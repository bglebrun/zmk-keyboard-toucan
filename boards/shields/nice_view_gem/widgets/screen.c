#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/display.h>

#include "screen.h"
#include "wpm.h"

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], const struct status_state *state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    fill_background(canvas);

    draw_wpm_status(canvas, state);
}

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)

/**
 * Central path: use ZMK's built-in WPM subsystem
 **/

#include <zmk/events/wpm_state_changed.h>
#include <zmk/wpm.h>

static void set_wpm_status(struct zmk_widget_screen *widget, struct wpm_status_state state) {
    for (int i = 0; i < 9; i++) {
        widget->state.wpm[i] = widget->state.wpm[i + 1];
    }
    widget->state.wpm[9] = state.wpm;

    draw_top(widget->obj, widget->cbuf, &widget->state);
}

static void wpm_status_update_cb(struct wpm_status_state state) {
    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_wpm_status(widget, state); }
}

static struct wpm_status_state wpm_status_get_state(const zmk_event_t *eh) {
    return (struct wpm_status_state){.wpm = zmk_wpm_get_state()};
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_wpm_status, struct wpm_status_state, wpm_status_update_cb,
                            wpm_status_get_state)

ZMK_SUBSCRIPTION(widget_wpm_status, zmk_wpm_state_changed);

static void screen_init_listeners(void) {
    widget_wpm_status_init();
}

#else /* Split peripheral path */

/**
 * Peripheral path: local APM tracking via position events
 **/

#include <zmk/events/position_state_changed.h>

#define APM_UPDATE_INTERVAL_SECONDS 1
#define APM_RESET_INTERVAL_SECONDS 5

static uint32_t key_press_count = 0;
static uint8_t apm_update_counter = 0;

static int peripheral_apm_listener(const zmk_event_t *eh) {
    const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);
    if (ev && ev->state) {
        key_press_count++;
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(peripheral_apm, peripheral_apm_listener);
ZMK_SUBSCRIPTION(peripheral_apm, zmk_position_state_changed);

static void apm_update_work_handler(struct k_work *work) {
    apm_update_counter++;

    uint16_t apm = 0;
    uint8_t wpm = 0;
    if (apm_update_counter > 0) {
        apm = (uint16_t)((key_press_count * 60.0) /
                         (apm_update_counter * APM_UPDATE_INTERVAL_SECONDS));
        wpm = apm / 5;
    }

    if (apm_update_counter >= APM_RESET_INTERVAL_SECONDS) {
        apm_update_counter = 0;
        key_press_count = 0;
    }

    struct zmk_widget_screen *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) {
        for (int i = 0; i < 9; i++) {
            widget->state.apm[i] = widget->state.apm[i + 1];
            widget->state.wpm[i] = widget->state.wpm[i + 1];
        }
        widget->state.apm[9] = apm;
        widget->state.wpm[9] = wpm;

        draw_top(widget->obj, widget->cbuf, &widget->state);
    }
}

static K_WORK_DEFINE(apm_update_work, apm_update_work_handler);

static void apm_timer_expiry(struct k_timer *timer) {
    k_work_submit_to_queue(zmk_display_work_q(), &apm_update_work);
}

static K_TIMER_DEFINE(apm_timer, apm_timer_expiry, NULL);

static void screen_init_listeners(void) {
    k_timer_start(&apm_timer, K_SECONDS(APM_UPDATE_INTERVAL_SECONDS),
                  K_SECONDS(APM_UPDATE_INTERVAL_SECONDS));
}

#endif /* CONFIG_ZMK_SPLIT */

/**
 * Initialization
 **/

int zmk_widget_screen_init(struct zmk_widget_screen *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, SCREEN_WIDTH, SCREEN_HEIGHT);

    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_canvas_set_buffer(top, widget->cbuf, SCREEN_WIDTH, SCREEN_HEIGHT, LV_IMG_CF_TRUE_COLOR);

    sys_slist_append(&widgets, &widget->node);
    screen_init_listeners();

    return 0;
}

lv_obj_t *zmk_widget_screen_obj(struct zmk_widget_screen *widget) { return widget->obj; }
