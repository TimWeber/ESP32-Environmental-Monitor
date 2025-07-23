@echo off
echo ESP32 Environmental Monitor - Native Unit Tests (MSVC)
echo ====================================================
echo.

echo Setting up native testing environment...
echo.

REM Check if Unity is installed
if not exist "unity\src\unity.c" (
    echo Installing Unity test framework...
    git clone https://github.com/ThrowTheSwitch/Unity.git unity
    if errorlevel 1 (
        echo Error: Failed to install Unity. Please check your internet connection.
        pause
        exit /b 1
    )
) else (
    echo Unity test framework already installed.
)

echo.
echo Building native test executable with MSVC...
echo.

REM Try to find Visual Studio
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"

if exist "%VSWHERE%" (
    for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
        set "VS_PATH=%%i"
    )
)

if defined VS_PATH (
    echo Found Visual Studio at: %VS_PATH%
    call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat"
) else (
    echo Warning: Visual Studio not found. Trying to use cl.exe directly...
)

REM Create include directories
if not exist "build" mkdir build

REM Build the test executable with MSVC
echo Compiling native test executable...
cl /W3 /std:c11 /DNATIVE_TESTING ^
    /I. /Iunit /Iunit/mocks /Iunity/src ^
    native_test_runner.c ^
    unit/network/test_wifi_manager.c ^
    unit/network/test_http_client.c ^
    unit/sensors/test_sensor_health.c ^
    unit/core/test_config_manager.c ^
    unit/core/test_health_monitor.c ^
    unit/mocks/mockI2cFunctions.c ^
    unit/mocks/mockWifiFunctions.c ^
    unit/test_result_tracker.c ^
    unity/src/unity.c ^
    /Fe:build/native_tests.exe

if errorlevel 1 (
    echo.
    echo Error: Build failed. Please check that Visual Studio Build Tools are installed.
    echo You can install them from: https://visualstudio.microsoft.com/downloads/
    echo.
    echo Alternative: Try running the ESP-IDF tests instead:
    echo   run_tests_simple.bat
    pause
    exit /b 1
)

echo.
echo Build successful! Running native unit tests...
echo.

REM Run the tests
build\native_tests.exe

if errorlevel 1 (
    echo.
    echo Error: Tests failed or crashed.
    pause
    exit /b 1
)

echo.
echo Native testing complete!
echo.
echo Summary:
echo - Native tests run on Windows without ESP32 hardware
echo - Tests use mock functions to simulate ESP32 environment
echo - For hardware-specific tests, use: run_tests_simple.bat
pause 