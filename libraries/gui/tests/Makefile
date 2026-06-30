# =============================================================================
# NusaCore GUI Framework — Test Makefile
# Runs on host Linux (x86/x64). No SDL2, no LVGL, no hardware required.
#
# Usage:
#   make          — Build and run BOTH test suites
#   make gesture  — Build and run ONLY the gesture engine test suite
#   make ui       — Build and run ONLY the UI layers test suite
#   make clean    — Remove all test binaries and build directory
# =============================================================================

CC        = gcc
BUILD_DIR = build

CFLAGS = -Wall -Wextra -g -O0 \
		 -DUNIT_TESTING	    \
         -I..            \
         -I../src

# Sources definitions
GESTURE_SRCS = test_gesture.c ../src/gesture.c
UI_SRCS      = test_ui_layers.c ../src/display_layer.c

# Binary targets output paths
TARGET_GESTURE = $(BUILD_DIR)/test_gesture
TARGET_UI      = $(BUILD_DIR)/test_ui_layers

.PHONY: all clean gesture ui

# 1. Default Option: Runs both test sets sequentially
all: gesture ui
	@echo "================================================"
	@echo "  All unit testing pipelines passed completely. "
	@echo "================================================"

# 2. Isolated Option: Build and execute ONLY the Gesture suite
gesture: $(TARGET_GESTURE)
	@echo ""
	@echo "Running Gesture Processing Engine Tests..."
	@echo "----------------------------------------"
	@./$(TARGET_GESTURE)

# 3. Isolated Option: Build and execute ONLY the UI Layer suite
ui: $(TARGET_UI)
	@echo ""
	@echo "Running UI Display Layer Structural Tests..."
	@echo "----------------------------------------"
	@./$(TARGET_UI)

# Rule to generate the build directory folder
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compilation target rule for the gesture binary
$(TARGET_GESTURE): $(GESTURE_SRCS) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(GESTURE_SRCS) -o $@

# Compilation target rule for the UI layers binary
$(TARGET_UI): $(UI_SRCS) | $(BUILD_DIR)
	@$(CC) $(CFLAGS) $(UI_SRCS) -o $@

clean:
	@rm -rf $(BUILD_DIR)
	@echo "Test build directory cleaned successfully."