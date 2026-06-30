#ifndef DISPLAY_LAYER_H
#define DISPLAY_LAYER_H

#include <time.h>
#include <stdio.h>

// 1. If we are testing, provide the mock interface to anyone including this file
#ifdef UNIT_TESTING
typedef struct lv_obj {
    char text_buffer[128];
    int text_align;
} lv_obj_t;

typedef struct {
    lv_obj_t *home_screen;
    lv_obj_t *clock_label;
    lv_obj_t *date_label;
    lv_obj_t *heartbeat_label;
    lv_obj_t *o2_label;
    lv_obj_t *steps_label;
    lv_obj_t *activity_label;
    lv_obj_t *notification_label;
} objects_t;

// Tell display_layer.c that 'objects' and mutations exist over in test_ui_layers.c
extern objects_t objects;
void lv_label_set_text(lv_obj_t *obj, const char *text);

#define lv_label_set_text_fmt(obj, fmt, ...) \
    do { \
        if (obj) snprintf((obj)->text_buffer, sizeof((obj)->text_buffer), fmt, __VA_ARGS__); \
    } while(0)
#endif

// 2. Your standard production function prototypes
void ui_update_clock_and_date(const struct tm *time_info);
void ui_update_health_metrics(int bpm, int spo2, int steps);
void ui_update_activity_state(int bpm);
void ui_update_notifications(const struct tm *time_info);

#endif