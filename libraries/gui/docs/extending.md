# NusaCore GUI Framework — Extension Guide

How to add new **input devices**, **output devices**, and **EEZ Studio UI screens**
to the framework without changing the rest of the codebase.

---

## Table of contents

1. [Framework architecture recap](#1-framework-architecture-recap)
2. [Adding a new output device (display)](#2-adding-a-new-output-device-display)
   - 2a. Different OLED controller
   - 2b. TFT / colour display over SPI
3. [Adding a new input device](#3-adding-a-new-input-device)
   - 3a. Different touchscreen controller (same gesture engine)
   - 3b. Physical push button (new LVGL indev type)
4. [Adding a new UI screen layout via EEZ Studio](#4-adding-a-new-ui-screen-layer-via-eez-studio)
   - 4a. Creating and exporting screens
   - 4b. Binding manual gestures to your layout
5. [How sensor tasks push data to the UI](#5-how-sensor-tasks-push-data-to-the-ui)

---

## 1. Framework architecture recap

```
main.c
  ├── display_driver_init()   ← ONE display, registered once
  ├── input_driver_init()     ← ONE indev, registered once
  ├── input_driver_set_gesture_cb(on_gesture)
  └── ui_create()             ← Loac generated layout from EEZ Studio
        ├── Tile 0: Clock
        ├── Tile 1: Health
        └── Tile 2: Fitness     ← Controlled via ui_tick() loop events
```

**Rule of thumb:**
- New *hardware output*             → edit `src/display_driver.c` only
- New *hardware input*              → edit `src/input_driver.c` only (gesture engine stays)
- New *UI layout modifications*     → edit in **EEZ Studio**, click **Export**, and adjust step animations inside `main.c` 

---

## 2. Adding a new output device (display)

All display code lives in `src/display_driver.c`.
The rest of the codebase only calls `display_driver_init()` and is abstract to the hardware layer.

### How `display_driver.c` works

```
display_driver_init()
  │
  ├─ [Linux]   lv_sdl_window_create(W, H)          ← no flush_cb needed
  │
  └─ [RISC-V]  lv_display_create(W, H)
                lv_display_set_flush_cb(disp, oled_flush_cb)
                lv_display_set_buffers(...)
                       │
                       ▼
               oled_flush_cb(disp, area, px_map)
                 │  Convert pixel format if needed
                 │  Send pixels to hardware
                 └  lv_display_flush_ready(disp)   ← MUST call this
```

### 2a. Swapping to a different OLED controller (e.g. SH1106 instead of SSD1306)

SH1106 uses a *page-based* write (no auto-increment across pages).
Only `oled_flush_cb` needs to change.

**In `src/display_driver.c`, RISC-V section:**

```c
static void oled_flush_cb(lv_display_t *disp,
                           const lv_area_t *area,
                           uint8_t *px_map)
{
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);

    // Convert LVGL pixel buffer → 1-bit (0 = off, 1 = on)
    // px_map is 16-bit RGB565 (see lv_conf.h LV_COLOR_DEPTH 16)
    uint16_t *src = (uint16_t *)px_map;

    for (int32_t row = area->y1; row <= area->y2; row++) {
        int page = row / 8;
        int bit  = row % 8;

        // SH1106: set page + column address before each row
        bsp_i2c_cmd(0xB0 | page);               // set page
        bsp_i2c_cmd(0x00 | (area->x1 & 0x0F));  // col low nibble
        bsp_i2c_cmd(0x10 | (area->x1 >> 4));    // col high nibble

        uint8_t byte = 0;
        for (int32_t col = area->x1; col <= area->x2; col++) {
            int idx = (row - area->y1) * w + (col - area->x1);
            if (src[idx]) byte |= (1 << bit);
        }
        bsp_i2c_data(&byte, 1);
    }

    lv_display_flush_ready(disp);   // ← always required
}
```

### 2b. Adding a TFT colour display (e.g. ST7789, ILI9341) over SPI

TFT displays use 16-bit RGB565 natively — which is exactly what LVGL
uses at `LV_COLOR_DEPTH 16`. No pixel conversion needed.

**Step 1 — Update `lv_conf.h`** if switching the RISC-V build to 16-bit:
```c
#define LV_COLOR_DEPTH  16
```

**Step 2 — Edit `src/display_driver.h`**:
```c
// Change the resolution constants to match the TFT
#define DISPLAY_HOR_RES  240    // e.g. 240×240 round TFT
#define DISPLAY_VER_RES  240
```

**Step 3 — Edit `src/display_driver.c`** (RISC-V section):
```c
/* TODO: include TFT module header */
#include "tft.h"

static uint16_t g_draw_buf[DISPLAY_HOR_RES * 20];  // 20-line band

static void tft_flush_cb(lv_display_t *disp,
                          const lv_area_t *area,
                          uint8_t *px_map)
{
    // px_map is already RGB565 — send directly to TFT via SPI
    tft_set_window(area->x1, area->y1, area->x2, area->y2);
    tft_write_pixels((uint16_t *)px_map,
                     lv_area_get_width(area) * lv_area_get_height(area));

    lv_display_flush_ready(disp);
}

lv_display_t *display_driver_init(void)
{
    tft_init();   // TODO: replace stub

    lv_display_t *disp = lv_display_create(DISPLAY_HOR_RES, DISPLAY_VER_RES);
    lv_display_set_flush_cb(disp, tft_flush_cb);
    lv_display_set_buffers(disp, g_draw_buf, NULL,
                            sizeof(g_draw_buf),
                            LV_DISPLAY_RENDER_MODE_PARTIAL);
    return disp;
}
```

**That is all** — `main.c`, `input_driver.c`, `gesture.c` stay unchanged.

---

## 3. Adding a new input device

All input code lives in `src/input_driver.c`.
The gesture engine (`src/gesture.c`) is untouched regardless of the
input hardware.

### 3a. Swapping to a different touchscreen controller

Only the two-line stub inside `touch_read_cb` changes.

**Find this block in `src/input_driver.c`** (RISC-V section):
```c
static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    bool    pressed = false;
    int16_t x = 0, y = 0;

    /* TODO: Replace with your module */
    // pressed = touch_read(&pt);
    // x = pt.x;   y = pt.y;

    feed_and_report(pressed, x, y, data);
}
```

**Replace the TODO** with your controller's read call:
```c
// Example: CST816S via ICDeC touch module
#include "touch.h"

static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    touch_point_t pt;
    bool pressed = touch_read(&pt);   // returns true if finger detected
    feed_and_report(pressed, pt.x, pt.y, data);
}
```

The `feed_and_report()` helper automatically:
1. Runs the gesture state machine
2. Fires `g_gesture_cb` if a gesture completed
3. Reports raw position to LVGL for widget hit-testing

### 3b. Adding a physical push button

Buttons use `LV_INDEV_TYPE_KEYPAD` instead of `LV_INDEV_TYPE_POINTER`.
They map to LVGL's focus/navigation system rather than to x/y coordinates.

**Step 1 — Add a second indev in `input_driver_init()`:**
```c
// In input_driver_init(), after registering the touch indev:

static void button_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    /*
     * TODO: read GPIO pin state, e.g.:
     *   bool pressed = (gpio_read(BUTTON_PIN) == 0);  // active-low
     */
    bool pressed = false;  // stub

    // Map hardware button → LVGL key code
    data->key   = LV_KEY_ENTER;   // or LV_KEY_NEXT, LV_KEY_PREV, etc.
    data->state = pressed ? LV_INDEV_STATE_PRESSED
                          : LV_INDEV_STATE_RELEASED;
}

// Register:
lv_indev_t *btn_indev = lv_indev_create();
lv_indev_set_type(btn_indev, LV_INDEV_TYPE_KEYPAD);
lv_indev_set_read_cb(btn_indev, button_read_cb);
lv_indev_set_display(btn_indev, display);
```

**Step 2 — Make widgets respond to key events** by adding a group:
```c
// In ui_create() in main.c:
lv_group_t *grp = lv_group_create();
lv_group_set_default(grp);
lv_indev_set_group(btn_indev, grp);

// Add focusable objects:
lv_group_add_obj(grp, my_button_widget);
```

**No changes** to `gesture.c`, `display_driver.c`, or `lv_conf.h`.

---

## 4. Adding a new UI screen layer via EEZ Studio

Adding a screen means adding one tile/panel to the tileview.

### 4a. Step-by-step template

**Step 1 — Increment the tile count constant:**
```c
// main.c
#define TILE_COUNT 4   // was 3, now 4
```

**Step 2 — Add widget handle variables** (so other functions can update them):
```c
// At the top of main.c, alongside the existing handles:
static lv_obj_t *g_lbl_my_value = NULL;
```

**Step 3 — Add the tile** inside `ui_create()`, after the last existing tile.
The column index must increase by 1, and the direction flags must be consistent:

```c
// The NEW last tile gets LV_DIR_LEFT only (nothing to the right)
lv_obj_t *t3 = lv_tileview_add_tile(g_tileview, 3, 0, LV_DIR_LEFT);
lv_obj_set_style_bg_color(t3, lv_color_black(), 0);

// Add widgets
g_lbl_my_value = lv_label_create(t3);
lv_label_set_text(g_lbl_my_value, "MyValue: --");
lv_obj_set_style_text_color(g_lbl_my_value, lv_color_white(), 0);
lv_obj_align(g_lbl_my_value, LV_ALIGN_CENTER, 0, 0);

// The PREVIOUS last tile must now also accept right-swipe
// Change tile 2's direction from LV_DIR_LEFT to:
//   LV_DIR_LEFT | LV_DIR_RIGHT
// (set when you call lv_tileview_add_tile for tile 2)
```

**Step 4 — Add an update function** that sensor tasks can call:
```c
// main.c — add this function (also declare it in a shared header if needed)
void ui_update_my_screen(int value)
{
    if (!g_lbl_my_value) return;
    lv_label_set_text_fmt(g_lbl_my_value, "MyValue: %d", value);
}
```

**Step 5 — Call the update function** from the sensor task (or a timer):
```c
// From any file, after including the declaration:
ui_update_my_screen(42);
```

That is all. LVGL will redraw only what changed on the next timer cycle.

---

### 4b. Complete example: Clock & Date screen

This is tile 0 in the existing code.  Here it is written as a full
self-contained example you can copy as a starting point.

```c
// ── Widget handles ────────────────────────────────────────────────────
static lv_obj_t *g_lbl_time = NULL;
static lv_obj_t *g_lbl_date = NULL;

// ── Build the tile (called once in ui_create) ─────────────────────────
static void screen_clock_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    g_lbl_time = lv_label_create(parent);
    lv_label_set_text(g_lbl_time, "00:00:00");
    lv_obj_set_style_text_font(g_lbl_time, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(g_lbl_time, lv_color_white(), 0);
    lv_obj_align(g_lbl_time, LV_ALIGN_CENTER, 0, -10);

    g_lbl_date = lv_label_create(parent);
    lv_label_set_text(g_lbl_date, "01/01/2025");
    lv_obj_set_style_text_font(g_lbl_date, &lv_font_montserrat_8, 0);
    lv_obj_set_style_text_color(g_lbl_date, lv_color_white(), 0);
    lv_obj_align(g_lbl_date, LV_ALIGN_CENTER, 0, 8);
}

// ── Update function (call from RTC task every second) ─────────────────
void ui_update_clock(uint8_t h, uint8_t m, uint8_t s,
                     uint8_t day, uint8_t month, uint16_t year)
{
    if (!g_lbl_time || !g_lbl_date) return;
    lv_label_set_text_fmt(g_lbl_time, "%02d:%02d:%02d", h, m, s);
    lv_label_set_text_fmt(g_lbl_date, "%02d/%02d/%04d", day, month, year);
}

// ── In ui_create() ────────────────────────────────────────────────────
// lv_obj_t *t0 = lv_tileview_add_tile(g_tileview, 0, 0, LV_DIR_RIGHT);
// screen_clock_create(t0);
```

---

### 4c. Complete example: Heart rate display

```c
// ── Widget handles ────────────────────────────────────────────────────
static lv_obj_t *g_arc_hr   = NULL;
static lv_obj_t *g_lbl_hr   = NULL;
static lv_obj_t *g_lbl_spo2 = NULL;

// ── Build the tile ────────────────────────────────────────────────────
static void screen_health_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_black(), 0);

    // Arc widget as a visual BPM meter (0–200 BPM range)
    g_arc_hr = lv_arc_create(parent);
    lv_arc_set_range(g_arc_hr, 0, 200);
    lv_arc_set_value(g_arc_hr, 0);
    lv_obj_set_size(g_arc_hr, 50, 50);
    lv_obj_align(g_arc_hr, LV_ALIGN_CENTER, -30, 0);
    lv_arc_set_bg_angles(g_arc_hr, 135, 45);   // 270° sweep

    // BPM numeric label inside the arc
    g_lbl_hr = lv_label_create(parent);
    lv_label_set_text(g_lbl_hr, "--");
    lv_obj_set_style_text_color(g_lbl_hr, lv_color_white(), 0);
    lv_obj_align(g_lbl_hr, LV_ALIGN_CENTER, -30, 0);

    // SpO2 label
    g_lbl_spo2 = lv_label_create(parent);
    lv_label_set_text(g_lbl_spo2, "SpO2\n--%");
    lv_obj_set_style_text_color(g_lbl_spo2, lv_color_white(), 0);
    lv_obj_align(g_lbl_spo2, LV_ALIGN_CENTER, 28, 0);
}

// ── Update function (call from HR sensor task) ────────────────────────
void ui_update_health(uint16_t bpm, uint8_t spo2)
{
    if (!g_lbl_hr || !g_lbl_spo2) return;

    lv_arc_set_value(g_arc_hr, (int32_t)bpm);
    lv_label_set_text_fmt(g_lbl_hr,   "%d", bpm);
    lv_label_set_text_fmt(g_lbl_spo2, "SpO2\n%d%%", spo2);
}
```

---

## 5. How sensor tasks push data to the UI

LVGL is **not thread-safe** by default on bare-metal single-core
(which is the current NusaCore target).  Since everything runs in one
execution context (the `while(1)` loop), update functions are safe to
call anywhere in the same loop.

```
while(1) {
    lv_tick_inc(1);
    g_tick_ms++;

    // Sensor reads happen here (or via polling in task functions)
    if (rtc_tick()) {
        uint8_t h, m, s, d, mo; uint16_t yr;
        rtc_read(&h, &m, &s, &d, &mo, &yr);
        ui_update_clock(h, m, s, d, mo, yr);    // ← safe here
    }

    if (hr_data_ready()) {
        uint16_t bpm; uint8_t spo2;
        hr_read(&bpm, &spo2);
        ui_update_health(bpm, spo2);             // ← safe here
    }

    lv_timer_handler();   // ← redraws only what changed
}
```

If the RISC-V board later runs a multi-core configuration with separate sensor
cores, wrap update calls in an LVGL mutex:

```c
lv_lock();              // acquire LVGL mutex
ui_update_health(bpm, spo2);
lv_unlock();            // release
```

---

## Quick reference — which file to edit

| Goal | File |
|---|---|
| New OLED / TFT display | `src/display_driver.c` (RISC-V section) |
| Display size change | `src/display_driver.h` (`DISPLAY_HOR_RES/VER_RES`) + `lv_conf.h` (`SDL_HOR/VER_RES`) |
| New touch controller | `src/input_driver.c` → `touch_read_cb` stub |
| New button / encoder | `src/input_driver.c` → new `lv_indev_create()` with `LV_INDEV_TYPE_KEYPAD` |
| Gesture thresholds | `src/gesture.h` (`GESTURE_CLICK_MAX_MS`, etc.) |
| New UI screen | `main.c` → add tile, widget handles, update function |
| Enable a new LVGL widget | `app/lv_conf.h` → set `LV_USE_xxx 1` |
| Change OLED I2C address | `src/display_driver.c` (inside the RISC-V BSP I2C call) |
