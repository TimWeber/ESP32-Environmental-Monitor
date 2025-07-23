# Native Test Makefile for ESP32 Environmental Monitor
# This allows running unit tests on the PC without ESP32 hardware

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g -DNATIVE_TESTING
INCLUDES = -I. -Iunit -Iunit/mocks -I../main -I../main/sensors -I../main/core -I../main/network

# Unity test framework (you'll need to install Unity separately)
UNITY_DIR = unity
UNITY_SRC = $(UNITY_DIR)/src/unity.c

# Test source files
TEST_SRCS = native_test_runner.c \
            unit/network/test_wifi_manager.c \
            unit/network/test_http_client.c \
            unit/sensors/test_sensor_health.c \
            unit/core/test_config_manager.c \
            unit/core/test_health_monitor.c \
            unit/mocks/mockI2cFunctions.c \
            unit/mocks/mockWifiFunctions.c

# Object files
OBJS = $(TEST_SRCS:.c=.o) $(UNITY_SRC:.c=.o)

# Target executable
TARGET = native_tests

# Default target
all: $(TARGET)
# Build the test executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Run tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

# Install Unity (if not already installed)
install-unity:
	@echo "Installing Unity test framework..."
	@if [ ! -d "$(UNITY_DIR)" ]; then \
		git clone https://github.com/ThrowTheSwitch/Unity.git $(UNITY_DIR); \
	else \
		echo "Unity already installed"; \
	fi

# Setup for first-time use
setup: install-unity all

.PHONY: all test clean install-unity setup 