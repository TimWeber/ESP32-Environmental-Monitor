/*
 * Native Test Runner for ESP32 Environmental Monitor
 * 
 * This file provides a native Windows executable that can run unit tests
 * without requiring ESP-IDF or ESP32 hardware. It uses Unity framework
 * and custom mocks to simulate the ESP32 environment.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Unity test framework
#include "unity/src/unity.h"

// Test result tracking
#include "unit/test_result_tracker.h"

// Mock implementations
#include "unit/mocks/mockI2cFunctions.h"
#include "unit/mocks/mockWifiFunctions.h"

// Test function declarations
extern void testWifiManagerInit(void);
extern void testWifiManagerConnect(void);
extern void testHttpClientPost(void);
extern void testSensorHealthTracking(void);

// Sensor tests
extern void testAht21InitSuccess(void);
extern void testAht21InitNoDevice(void);
extern void testAht21ReadData(void);
extern void testEns160InitSuccess(void);
extern void testEns160InitNoDevice(void);
extern void testEns160ReadData(void);
extern void testSensorExceptionHandling(void);
extern void testI2cManager(void);
extern void testDemonstrateFailure(void);

// Configuration tests
extern void testConfigManagerValidConfig(void);
extern void testConfigManagerInvalidConfig(void);
extern void testConfigManagerMissingConfig(void);
extern void testConfigManagerValidation(void);
extern void testConfigManagerSensorConfig(void);
extern void testConfigManagerNetworkConfig(void);
extern void testConfigManagerErrorHandling(void);

// Health monitor tests
extern void testHealthMonitorInit(void);
extern void testHealthMonitorSensorStatus(void);
extern void testHealthMonitorStatistics(void);
extern void testHealthMonitorDataSerialisation(void);

// Test wrapper function to properly track results
#define RUN_TEST_WITH_TRACKING(test_func) do { \
    testResultStartTest(#test_func); \
    test_func(); \
    if (testResultGetCurrentTestResult()) { \
        testResultRecordSuccess(#test_func); \
    } else { \
        testResultRecordFailure(#test_func, "Test assertion failed", __LINE__, __FILE__); \
    } \
} while(0)

void setUp(void) {
    // Set up function called before each test
    printf("Setting up test environment\n");
}

void tearDown(void) {
    // Tear down function called after each test
    printf("Cleaning up test environment\n");
}

/**
 * @brief Main test runner function
 */
void run_all_tests(void) {
    printf("Starting native unit tests for ESP32 Environmental Monitor\n");
    printf("========================================================\n");
    
    // Initialize test result tracking
    testResultInit();
    
    UNITY_BEGIN();
    
    // Network module tests
    printf("Running network module tests...\n");
    RUN_TEST_WITH_TRACKING(testWifiManagerInit);
    RUN_TEST_WITH_TRACKING(testWifiManagerConnect);
    RUN_TEST_WITH_TRACKING(testHttpClientPost);
    
    // Sensor module tests
    printf("Running sensor module tests...\n");
    RUN_TEST_WITH_TRACKING(testSensorHealthTracking);
    RUN_TEST_WITH_TRACKING(testAht21InitSuccess);
    RUN_TEST_WITH_TRACKING(testAht21InitNoDevice);
    RUN_TEST_WITH_TRACKING(testAht21ReadData);
    RUN_TEST_WITH_TRACKING(testEns160InitSuccess);
    RUN_TEST_WITH_TRACKING(testEns160InitNoDevice);
    RUN_TEST_WITH_TRACKING(testEns160ReadData);
    RUN_TEST_WITH_TRACKING(testSensorExceptionHandling);
    RUN_TEST_WITH_TRACKING(testI2cManager);
    RUN_TEST_WITH_TRACKING(testDemonstrateFailure);
    
    // Configuration module tests
    printf("Running configuration module tests...\n");
    RUN_TEST_WITH_TRACKING(testConfigManagerValidConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerInvalidConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerMissingConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerValidation);
    RUN_TEST_WITH_TRACKING(testConfigManagerSensorConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerNetworkConfig);
    RUN_TEST_WITH_TRACKING(testConfigManagerErrorHandling);
    
    // Health monitor module tests
    printf("Running health monitor module tests...\n");
    RUN_TEST_WITH_TRACKING(testHealthMonitorInit);
    RUN_TEST_WITH_TRACKING(testHealthMonitorSensorStatus);
    RUN_TEST_WITH_TRACKING(testHealthMonitorStatistics);
    RUN_TEST_WITH_TRACKING(testHealthMonitorDataSerialisation);
    
    UNITY_END();
    
    // Print test summary
    printf("========================================================\n");
    testResultPrintSummary();
    printf("✅ All native unit tests completed!\n");
}

/**
 * @brief Main function for native test executable
 */
int main(void) {
    printf("ESP32 Environmental Monitor - Native Unit Tests\n");
    printf("==============================================\n");
    printf("Running tests on Windows host (no ESP32 required)\n\n");
    
    // Initialize mock functions
    mockI2cFunctionsInit();
    mockWifiFunctionsInit();
    
    // Run all tests
    run_all_tests();
    
    printf("\nNative testing complete!\n");
    return 0;
} 