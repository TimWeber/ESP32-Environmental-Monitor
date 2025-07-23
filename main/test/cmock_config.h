/*
 * CMock Configuration for ESP32-C3 Environmental Monitor
 * 
 * This file configures CMock for generating mocks of ESP-IDF and
 * project-specific functions for unit testing.
 */

#ifndef CMOCK_CONFIG_H
#define CMOCK_CONFIG_H

// Basic CMock settings
#define CMOCK_MEM_DYNAMIC               1
#define CMOCK_MEM_SIZE                  65536
#define CMOCK_MEM_ALIGN                 2

// Mock function naming
#define CMOCK_MOCK_PREFIX               "Mock"
#define CMOCK_WEAK                      "__attribute__((weak))"

// Include support for various data types
#define CMOCK_MEM_INDEX_TYPE            unsigned int
#define CMOCK_MEM_PTR_AS_INT            unsigned long
#define CMOCK_MEM_ALIGN                 2

// Callback support
#define CMOCK_CALLBACK_INCLUDE_COUNT    1
#define CMOCK_CALLBACK_MAX_CALLS        10

// Array handling
#define CMOCK_MEM_DYNAMIC               1

// Plugin configuration
#define CMOCK_PLUGINS_ALL               1

// ESP32-specific configurations
#define CMOCK_EXCLUDE_SETJMP_H          1

// Function pointer support
#define CMOCK_INCLUDE_FUNCTION_POINTERS 1

// Variadic function support (for printf-style functions)
#define CMOCK_VARIADIC_FUNCTIONS        1

#endif // CMOCK_CONFIG_H
