
# NusaCore UI Display Layer Architecture

This document describes the high-level UI controller and data-binding layer for the NusaCore wearable framework. This layer acts as the bridge between raw sensor data (RTC, PPG, step counter) and the underlying LVGL widgets generated via EEZ Studio.

## Architecture Overview

The UI subsystem is completely decoupled from the underlying hardware execution environment:


```

[Raw Sensors / Systems] ---> [display_layer.c] ---> [LVGL Core] ---> [display_driver.c]
(RTC, PPG, Step Counter)     (Formats text & states)   (Widgets)      (SDL2 Linux / RISC-V OLED)

```

By keeping the formatting logic inside `display_layer.c`, we can fully unit-test the interface logic on a host Linux terminal without opening graphic windows or utilizing physical target sensors.

---

## API Reference

Include `"display_layer.h"` to interact with these functions.

### 1. Clock and Date Updates
```c
void ui_update_clock_and_date(const struct tm *time_info);

```

* **Purpose**: Updates the core temporal layouts.
* **Behavior**: Formats hours/minutes/seconds into `HH:MM:SS` on `objects.clock_label` and maps dates into `DD/MM/YYYY` on `objects.date_label`.
* **Safety**: Safely drops out if `time_info` or target assets are uninitialized (`NULL`).

### 2. Health Metrics Data Binding

```c
void ui_update_health_metrics(int bpm, int spo2, int steps);

```

* **Purpose**: Batch updates current tracking metrics onto active sensor tiles.
* **Formatting Rules**:
* Heart Rate: Appends trailing ` bpm` context (e.g., `"72 bpm"`).
* Blood Oxygen: Formats into `SpO2: X%` string layouts.
* Steps: Appends trailing ` steps` context (e.g., `"1420 steps"`).



### 3. Activity Recognition State Machine

```c
void ui_update_activity_state(int bpm);

```

* **Purpose**: Drives the UI text display reflecting the user's active state.
* **Internal Threshold Layouts**:
* `BPM > 80`: Displays `State: RUNNING`
* `72 < BPM <= 80`: Displays `State: WALKING`
* `BPM <= 72`: Displays `State:  STILL `


* **Note**: This function updates *display boundaries only*. The true algorithmic classifier engine is isolated in backend threads.

### 4. Alert & Notification Routing

```c
void ui_update_notifications(const struct tm *time_info);

```

* **Purpose**: Controls global priority notification panels.
* **Behavior**: Displays `ALARM!\nWake Up!` inside a programmatic window or updates to standard clear-text states.

---

## Integration Example (How to push data from `main.c`)

When building hardware loops or simulation paths, dispatch metrics directly through the display layer interface functions:

```c
#include "display_layer.h"

// Inside your execution loop...
time_t raw_time = time(NULL);
struct tm *time_info = localtime(&raw_time);

// 1. Pass time data
ui_update_clock_and_date(time_info);

// 2. Fetch sensor readings and pass straight to UI
int current_bpm = read_ppg_sensor_bpm();
int current_spo2 = read_ppg_sensor_spo2();
int cumulative_steps = get_step_count();

ui_update_health_metrics(current_bpm, current_spo2, cumulative_steps);
ui_update_activity_state(current_bpm);
ui_update_notifications(time_info);

```

## Unit Testing

To verify this display translation logic remains flawless across modifications, navigate to the test repository path and run:

```bash
cd tests
make ui

```