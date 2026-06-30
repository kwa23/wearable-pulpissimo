/**
 * @file    test_ui_layers.c
 * @brief   Unit tests for NusaCore UI display layers and state transitions.
 *
 * Compile & run on any Linux host (No SDL2 window required):
 * cd app/tests
 * gcc test_ui_layers.c -o test_ui_layers
 * ./test_ui_layers
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "display_layer.h"

// =========================================================================
// 1. LVGL & EEZ Minimal Structural Mocks (Prevents bringing in massive graphics deps)
// =========================================================================


typedef enum {
    ACTIVITY_STILL,
    ACTIVITY_WALKING,
    ACTIVITY_RUNNING
} activity_state_t;

objects_t objects;

// Mock implementations of the LVGL label mutation APIs
void lv_label_set_text(lv_obj_t *obj, const char *text) {
    if (obj) snprintf(obj->text_buffer, sizeof(obj->text_buffer), "%s", text);
}

// =========================================================================
// 2. Test Framework Metrics
// =========================================================================
static int g_pass = 0, g_fail = 0;

#define ASSERT(cond, msg)                                                   \
    do {                                                                     \
        if (cond) { printf("  PASS: %s\n", msg); g_pass++;                  \
        } else    { printf("  FAIL: %s  (line %d)\n", msg, __LINE__);       \
                    g_fail++; }                                               \
    } while (0)

// Helper to initialize or clear memory pools between isolated test runs
static void init_mock_ui(void) {
    static lv_obj_t mock_clock, mock_date, mock_heart, mock_o2, mock_steps, mock_act, mock_notif;
    
    memset(&mock_clock, 0, sizeof(lv_obj_t));
    memset(&mock_date,  0, sizeof(lv_obj_t));
    memset(&mock_heart, 0, sizeof(lv_obj_t));
    memset(&mock_o2,    0, sizeof(lv_obj_t));
    memset(&mock_steps, 0, sizeof(lv_obj_t));
    memset(&mock_act,   0, sizeof(lv_obj_t));
    memset(&mock_notif, 0, sizeof(lv_obj_t));

    objects.clock_label        = &mock_clock;
    objects.date_label        = &mock_date;
    objects.heartbeat_label    = &mock_heart;
    objects.o2_label           = &mock_o2;
    objects.steps_label        = &mock_steps;
    objects.activity_label     = &mock_act;
    objects.notification_label = &mock_notif;
}

// =========================================================================
// 3. Test Cases for Display Layers
// =========================================================================

static void test_clock_and_date_layer(void) {
    printf("\n[test_clock_and_date_layer]\n");
    init_mock_ui();

    struct tm fake_time;
    fake_time.tm_hour = 9;   fake_time.tm_min = 5;  fake_time.tm_sec = 2;
    fake_time.tm_mday = 15;  fake_time.tm_mon = 9;  fake_time.tm_year = 126; 

    ui_update_clock_and_date(&fake_time);
    
    ASSERT(strcmp(objects.clock_label->text_buffer, "09:05:02") == 0, "Clock matches mock time");
    
    // DEBUG PRINT 1:
    printf("  [DEBUG] objects.date_label pointer address: %p\n", (void*)objects.date_label);
    
    ASSERT(strcmp(objects.date_label->text_buffer, "15/10/2026") == 0, "Date matches mock time conversion");
}

static void test_health_metrics_layer(void) {
    printf("\n[test_health_metrics_layer]\n");
    init_mock_ui();

    // DEBUG PRINT 2:
    printf("  [DEBUG] Entering health metrics. Labels: HR=%p, O2=%p, Steps=%p\n", 
           (void*)objects.heartbeat_label, (void*)objects.o2_label, (void*)objects.steps_label);

    ui_update_health_metrics(74, 98, 1250);

    ASSERT(strcmp(objects.heartbeat_label->text_buffer, "74 bpm") == 0, "BPM updates accurately");
    ASSERT(strcmp(objects.o2_label->text_buffer, "SpO2: 98%") == 0, "SpO2 percentage notation matches layout requirement");
    ASSERT(strcmp(objects.steps_label->text_buffer, "1250 steps") == 0, "Steps count text formatting updates accurately");
}

static void test_activity_state_boundaries(void) {
    printf("\n[test_activity_state_boundaries]\n");

    // Test Case A: Running Threshold
    init_mock_ui();
    ui_update_activity_state(85);
    ASSERT(strcmp(objects.activity_label->text_buffer, "State: RUNNING") == 0, "BPM > 80 outputs RUNNING");

    // Test Case B: Walking Boundary
    init_mock_ui();
    ui_update_activity_state(75);
    ASSERT(strcmp(objects.activity_label->text_buffer, "State: WALKING") == 0, "72 < BPM <= 80 outputs WALKING");

    // Test Case C: Still Boundary
    init_mock_ui();
    ui_update_activity_state(60);
    ASSERT(strcmp(objects.activity_label->text_buffer, "State:  STILL ") == 0, "BPM <= 72 outputs STILL (with structural alignment spaces)");
}

static void test_notification_layer_trigger_logic(void) {
    printf("\n[test_notification_layer_trigger_logic]\n");
    struct tm fake_time;

    // Test Case A: Inside the 5-second alarm window (e.g., 22 seconds)
    init_mock_ui();
    fake_time.tm_sec = 22; // 22 % 20 = 2 (which is < 5)
    ui_update_notifications(&fake_time);
    ASSERT(strcmp(objects.notification_label->text_buffer, "ALARM!\nWake Up!") == 0, "Alarm shows inside the triggered window evaluation");

    // Test Case B: Outside the alarm window (e.g., 27 seconds)
    init_mock_ui();
    fake_time.tm_sec = 27; // 27 % 20 = 7 (which is >= 5)
    ui_update_notifications(&fake_time);
    ASSERT(strcmp(objects.notification_label->text_buffer, "No New\nNotifications") == 0, "Quiet notification clear text updates normally outside the window");
}

// =========================================================================
// 4. Main Driver
// =========================================================================
int main(void) {
    printf("================================================\n");
    printf("  NusaCore UI Layer Architecture — Unit Tests   \n");
    printf("================================================\n");

    test_clock_and_date_layer();
    test_health_metrics_layer();
    test_activity_state_boundaries();
    test_notification_layer_trigger_logic();

    printf("\n================================================\n");
    printf("  %d passed, %d failed\n", g_pass, g_fail);
    printf("================================================\n");

    return g_fail == 0 ? 0 : 1;
}