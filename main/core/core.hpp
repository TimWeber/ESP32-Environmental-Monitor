#pragma once

/**
 * @brief Core system components for the ESP32 Environmental Monitoring System
 * 
 * This module contains the foundational system components:
 * - RTOSManager: Multi-task system management
 * - I2CManager: Hardware I2C bus management
 * 
 * These components provide the core infrastructure that other modules depend on.
 */

#include "managers/I2CManager.hpp"
#include "managers/rtos_manager.hpp"

// Core system configuration
namespace Core {
    // System-wide constants
    static constexpr uint32_t DEFAULT_SENSOR_READING_INTERVAL_MS = 30000;
    static constexpr uint32_t DEFAULT_NETWORK_TRANSMISSION_INTERVAL_MS = 30000;
    static constexpr uint32_t DEFAULT_MONITOR_REPORT_INTERVAL_MS = 60000;
    
    // Task priorities (higher number = higher priority)
    static constexpr UBaseType_t SENSOR_TASK_PRIORITY = 5;
    static constexpr UBaseType_t NETWORK_TASK_PRIORITY = 4;
    static constexpr UBaseType_t MONITOR_TASK_PRIORITY = 3;
    static constexpr UBaseType_t WATCHDOG_TASK_PRIORITY = 6;
    
    // Task stack sizes
    static constexpr uint32_t SENSOR_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t NETWORK_TASK_STACK_SIZE = 8192;
    static constexpr uint32_t MONITOR_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t WATCHDOG_TASK_STACK_SIZE = 2048;
} 