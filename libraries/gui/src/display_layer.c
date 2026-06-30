#include "display_layer.h"
// Include your EEZ Studio ui.h and LVGL here in production

#ifndef UNIT_TESTING
#  include "ui.h"
#  include "lvgl.h"
#endif

void ui_update_clock_and_date(const struct tm *time_info) {
    if (!time_info) return;

    if (objects.clock_label) {
        lv_label_set_text_fmt(objects.clock_label, "%02d:%02d:%02d",
                              time_info->tm_hour, time_info->tm_min, time_info->tm_sec);
    }
    if (objects.date_label) { 
        lv_label_set_text_fmt(objects.date_label, "%02d/%02d/%04d",
                              time_info->tm_mday, time_info->tm_mon + 1, time_info->tm_year + 1900);
    }
}

void ui_update_health_metrics(int bpm, int spo2, int steps) {
    if (objects.heartbeat_label) {
        lv_label_set_text_fmt(objects.heartbeat_label, "%d bpm", bpm);
    }
    if (objects.o2_label) {
        lv_label_set_text_fmt(objects.o2_label, "SpO2: %d%%", spo2);
    }
    if (objects.steps_label) {
        lv_label_set_text_fmt(objects.steps_label, "%d steps", steps);
    }
}

void ui_update_activity_state(int bpm) {
    if (!objects.activity_label) return;

    if (bpm > 80) {
        lv_label_set_text(objects.activity_label, "State: RUNNING");
    } else if (bpm > 72) {
        lv_label_set_text(objects.activity_label, "State: WALKING");
    } else {
        lv_label_set_text(objects.activity_label, "State:  STILL ");
    }
}

void ui_update_notifications(const struct tm *time_info) {
    if (!time_info || !objects.notification_label) return;

    if (time_info->tm_sec % 20 < 5) { 
        lv_label_set_text(objects.notification_label, "ALARM!\nWake Up!");
    } else {
        lv_label_set_text(objects.notification_label, "No New\nNotifications");
    }
}