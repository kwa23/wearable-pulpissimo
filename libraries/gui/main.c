/**
 * @file    main.c
 * @brief   NusaCore RISC-V Wearable — GUI Framework Entry Point
 *
 * Build targets:
 * make PLATFORM=linux    SDL2 window for development on Ubuntu/CachyOS
 * make PLATFORM=riscv    pulp-runtime on NusaCore board
 */

/* ── Standard library includes MUST be at the top, outside any function ── */
#include <stdio.h>
#include <stdint.h>

#ifdef PLATFORM_LINUX
#  include <unistd.h>   /* usleep()  */
#  include <time.h>
#  include <stdlib.h>
#endif

/* ── LVGL & project headers ─────────────────────────────────────────────── */
#include "lvgl.h"
#include "display_driver.h"
#include "input_driver.h"
#include "ui.h"             /* EEZ Studio generated master header */
#include "display_layer.h"

/* =========================================================================
 * RISC-V tick counter
 * Incremented manually in the main loop (replace with hardware timer ISR).
 * On Linux, usleep() provides the timing and we just call lv_tick_inc().
 * ========================================================================= */
#ifndef PLATFORM_LINUX
volatile uint32_t g_tick_ms = 0;
#endif

/* =========================================================================
 * Forward declarations
 * ========================================================================= */
static void on_gesture(const gesture_event_t *event);

/* =========================================================================
 * Main
 * ========================================================================= */
int main(void)
{
    printf("\n");
    printf("================================================\n");
    printf("  NusaCore GUI Framework\n");
    printf("  ICDeC RISC-V Wearable Project\n");
#ifdef PLATFORM_LINUX
    printf("  Platform : Linux / SDL2 (development)\n");
#else
    printf("  Platform : RISC-V bare-metal (NusaCore)\n");
#endif
    printf("================================================\n\n");

    /* 1. Initialise LVGL */
    lv_init();
    printf("[MAIN] LVGL initialised\n");

    /* 2. Display (SDL2 window on Linux, OLED on RISC-V) */
    lv_display_t *display = display_driver_init();
    if (display == NULL) {
        fprintf(stderr, "[MAIN] FATAL: display_driver_init() failed\n");
        return 1;
    }

    /* 3. Input (SDL mouse on Linux, touch controller on RISC-V) */
    input_driver_init(display);

    /* 4. Register application gesture handler */
    input_driver_set_gesture_cb(on_gesture);

    /* 5. Build the EEZ Studio UI layout */
    ui_init();
    printf("[MAIN] EEZ Studio UI layout ready — entering event loop\n\n");

#ifdef PLATFORM_LINUX
    uint32_t sim_ticks = 0;
    int current_steps = 1420; // Starting baseline for step counts
#endif

    /* 6. Event loop */
    while (1) {
#ifdef PLATFORM_LINUX
lv_tick_inc(5);
        ui_tick();               /* EEZ runtime tick for processing animations/states */

        // =====================================================================
        // Host OS Clock Update (Linux Preview Path)
        // =====================================================================
        time_t raw_time = time(NULL);
        struct tm *time_info = localtime(&raw_time);

        // Call your display layer to handle clock & date rendering
        ui_update_clock_and_date(time_info);

        // =====================================================================
        // Simulated Sensor Variance (Changes roughly every 1.5 seconds)
        // =====================================================================
        sim_ticks++;
        if (sim_ticks % 300 == 0) {
            // 1. Generate random math for fake sensors
            int simulated_bpm = 65 + (rand() % 21);
            int simulated_spo2 = 97 + (rand() % 3);
            current_steps += (rand() % 5); 

            // 2. Pass the raw data into your clean UI layer functions
            ui_update_health_metrics(simulated_bpm, simulated_spo2, current_steps);
            ui_update_activity_state(simulated_bpm);
            ui_update_notifications(time_info);
            
            printf("[SIMULATOR] UI Updated -> HR: %d bpm | SpO2: %d%% | Steps: %d\n", 
                   simulated_bpm, simulated_spo2, current_steps);
        }

        lv_timer_handler();
        usleep(5000);            /* 5 ms sleep keeps CPU usage low      */
#else
        lv_tick_inc(1);
        g_tick_ms++;
        ui_tick();               /* EEZ runtime tick for processing animations/states */

        // =====================================================================
        // Real Hardware Sensor Update Path (RISC-V Bare-metal)
        // =====================================================================
        /* if (rtc_data_ready()) {
            // Read hardware I2C logs here later...
        }
        */

        lv_timer_handler();
        /* TODO: replace busy-loop with 1 ms hardware timer ISR         */
#endif
    }

    return 0;
}

/* =========================================================================
 * Gesture handler
 * Called by input_driver when a gesture is recognised.
 * ========================================================================= */
static int g_current_tile = 0;
#define TILE_COUNT 6

static void on_gesture(const gesture_event_t *event)
{
    // Safety check: ensure the EEZ layout objects are fully initialized first
    if (objects.home_screen == NULL) return;

    /* Retrieve the tileview dynamically. 
     * Adjust the index '0' if your tileview is not the first child 
     * of the home_screen in EEZ Studio. */
    lv_obj_t *my_tileview = lv_obj_get_child(objects.home_screen, 0);
    if (my_tileview == NULL) return; // Extra safety if child is missing

    switch (event->type) {
        case GESTURE_SWIPE_LEFT:
            if (g_current_tile < TILE_COUNT - 1) {
                g_current_tile++;
                lv_obj_set_tile_id(my_tileview, (uint32_t)g_current_tile, 0, LV_ANIM_ON);
                printf("[APP] Swiped Left -> Moving manually to Tile %d\n", g_current_tile);
            }
            break;

        case GESTURE_SWIPE_RIGHT:
            if (g_current_tile > 0) {
                g_current_tile--;
                lv_obj_set_tile_id(my_tileview, (uint32_t)g_current_tile, 0, LV_ANIM_ON);
                printf("[APP] Swiped Right -> Moving manually to Tile %d\n", g_current_tile);
            }
            break;

        case GESTURE_CLICK:
            printf("[APP] Click at (%d, %d)\n", event->end_x, event->end_y);
            break;

        // --- Added missing gestures from Snippet 1 ---
        case GESTURE_DOUBLE_CLICK:
            printf("[APP] Double-click — toggle backlight\n");
            // Insert backlight toggle logic here, e.g.:
            // lv_disp_set_brightness(lv_disp_get_default(), new_value);
            break;

        case GESTURE_LONG_PRESS:
            printf("[APP] Long press — open settings\n");
            // Insert settings navigation logic here, e.g.:
            // lv_obj_add_flag(settings_panel, LV_OBJ_FLAG_HIDDEN);
            break;
        // ---------------------------------------------

        default:
            break;
    }
}   

/* =========================================================================
 * pulp-runtime hook (RISC-V only)
 * ========================================================================= */
void pe_start(void)
{
    /* Reserved for future multi-core use on NusaCore */
}