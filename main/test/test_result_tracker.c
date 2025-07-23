/*
 * Test Result Tracker Implementation for ESP32 Environmental Monitor Tests
 * 
 * This file provides the implementation for tracking and reporting test results.
 */

#include "test_result_tracker.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "TEST_TRACKER";

// Test statistics
static int totalTests = 0;
static int passedTests = 0;
static int failedTests = 0;

// Current test tracking
static TestResult currentTest = {0};
static bool testInProgress = false;

// Initialize test result tracking
void testResultInit(void) {
    totalTests = 0;
    passedTests = 0;
    failedTests = 0;
    testInProgress = false;
    memset(&currentTest, 0, sizeof(TestResult));
    ESP_LOGI(TAG, "Test result tracker initialised");
}

// Start tracking a test
void testResultStartTest(const char* testName) {
    if (testInProgress) {
        ESP_LOGW(TAG, "Previous test not completed, starting new test: %s", testName);
    }
    
    currentTest.testName = testName;
    currentTest.passed = true; // Assume success initially
    currentTest.failureMessage = NULL;
    currentTest.lineNumber = 0;
    currentTest.fileName = NULL;
    testInProgress = true;
    
    ESP_LOGI(TAG, "Starting test: %s", testName);
    totalTests++;
}

// Record test success
void testResultRecordSuccess(const char* testName) {
    if (!testInProgress || strcmp(currentTest.testName, testName) != 0) {
        ESP_LOGW(TAG, "Test success recorded for unknown test: %s", testName);
        return;
    }
    
    currentTest.passed = true;
    passedTests++;
    testInProgress = false;
    
    ESP_LOGI(TAG, "✅ PASS: %s", testName);
}

// Record test failure
void testResultRecordFailure(const char* testName, const char* message, int line, const char* file) {
    if (!testInProgress || strcmp(currentTest.testName, testName) != 0) {
        ESP_LOGW(TAG, "Test failure recorded for unknown test: %s", testName);
        return;
    }
    
    currentTest.passed = false;
    currentTest.failureMessage = message;
    currentTest.lineNumber = line;
    currentTest.fileName = file;
    failedTests++;
    testInProgress = false;
    
    ESP_LOGE(TAG, "❌ FAIL: %s", testName);
    if (message) {
        ESP_LOGE(TAG, "   Reason: %s", message);
    }
    if (file && line > 0) {
        ESP_LOGE(TAG, "   Location: %s:%d", file, line);
    }
}

// Get the result of the current test
bool testResultGetCurrentTestResult(void) {
    return currentTest.passed;
}

// Print test summary
void testResultPrintSummary(void) {
    ESP_LOGI(TAG, "Test Summary:");
    ESP_LOGI(TAG, "   Total Tests: %d", totalTests);
    ESP_LOGI(TAG, "   Passed: %d", passedTests);
    ESP_LOGI(TAG, "   Failed: %d", failedTests);
    
    if (totalTests > 0) {
        float successRate = (float)passedTests / totalTests * 100.0f;
        ESP_LOGI(TAG, "   Success Rate: %.1f%%", successRate);
    }
    
    if (failedTests > 0) {
        ESP_LOGE(TAG, "⚠️  Some tests failed!");
    } else {
        ESP_LOGI(TAG, "✅ All tests passed!");
    }
}

// Get test statistics
int testResultGetTotalTests(void) {
    return totalTests;
}

int testResultGetPassedTests(void) {
    return passedTests;
}

int testResultGetFailedTests(void) {
    return failedTests;
} 