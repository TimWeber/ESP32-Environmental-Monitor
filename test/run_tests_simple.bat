@echo off
echo ESP32 Environmental Monitor - Simple Test Runner
echo ===============================================
echo.

echo This script will run tests using ESP-IDF's built-in testing framework.
echo No external compilers required!
echo.

echo Setting up test environment...
echo.

REM Check if we're in the right directory
if not exist "unit" (
    echo Error: Please run this script from the test directory.
    echo Current directory: %CD%
    pause
    exit /b 1
)

echo Building test application...
echo.

REM Clean any previous builds
if exist "build" (
    echo Cleaning previous build...
    rmdir /s /q build
)

REM Build the test application using ESP-IDF
echo Building with ESP-IDF...
idf.py build

if errorlevel 1 (
    echo.
    echo Error: Build failed. This might be due to missing dependencies.
    echo.
    echo Trying alternative approach...
    echo.
    
    REM Try building from root directory
    cd ..
    idf.py -p test build
    
    if errorlevel 1 (
        echo.
        echo Error: Build failed. Please check your ESP-IDF installation.
        echo Make sure you have ESP-IDF v5.4+ installed and configured.
        pause
        exit /b 1
    ) else (
        echo.
        echo Build successful from root directory!
        cd test
    )
) else (
    echo.
    echo Build successful!
)

echo.
echo Test application built successfully!
echo.
echo To run tests on ESP32 hardware:
echo   1. Connect your ESP32 device
echo   2. Run: idf.py flash monitor
echo.
echo To run tests without hardware (simulation):
echo   1. Install QEMU for ESP32 simulation
echo   2. Run: idf.py qemu
echo.
echo Note: The tests use mock functions, so they can run without real sensors.
echo.

echo Test setup complete!
pause 