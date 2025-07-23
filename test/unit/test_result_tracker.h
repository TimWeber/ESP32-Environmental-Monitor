/*
 * Test Result Tracker for ESP32 Environmental Monitor Tests
 * 
 * This header provides functions to track and report test results properly.
 */

#ifndef TEST_RESULT_TRACKER_H
#define TEST_RESULT_TRACKER_H

#include <stdbool.h>

// Test result tracking
typedef struct {
    const char* testName;
    bool passed;
    const char* failureMessage;
    int lineNumber;
    const char* fileName;
} TestResult;

// Initialize test result tracking
void testResultInit(void);

// Start tracking a test
void testResultStartTest(const char* testName);

// Record test success
void testResultRecordSuccess(const char* testName);

// Record test failure
void testResultRecordFailure(const char* testName, const char* message, int line, const char* file);

// Get the result of the current test
bool testResultGetCurrentTestResult(void);

// Print test summary
void testResultPrintSummary(void);

// Get test statistics
int testResultGetTotalTests(void);
int testResultGetPassedTests(void);
int testResultGetFailedTests(void);

#endif // TEST_RESULT_TRACKER_H 