@echo off
echo ESP32 Environmental Monitor - Test Suite Runner
echo ==============================================
echo.

echo Choose your testing approach:
echo.
echo 1. Native Tests (Fast, Windows only, no ESP32 required)
echo    - Tests business logic and algorithms
echo    - Uses mock functions for hardware simulation
echo    - Requires Visual Studio Build Tools
echo.
echo 2. ESP-IDF Tests (Slower, requires ESP32 or QEMU)
echo    - Tests hardware interactions and ESP-IDF APIs
echo    - Can run on real ESP32 hardware or QEMU simulation
echo    - More realistic testing environment
echo.
echo 3. Both (Run native tests first, then ESP-IDF tests)
echo.

set /p choice="Enter your choice (1, 2, or 3): "

if "%choice%"=="1" goto native
if "%choice%"=="2" goto esp32
if "%choice%"=="3" goto both
echo Invalid choice. Please enter 1, 2, or 3.
pause
exit /b 1

:native
echo.
echo Running Native Tests...
echo =====================
call run_native_tests_msvc.bat
goto end

:esp32
echo.
echo Running ESP-IDF Tests...
echo ======================
call run_tests_simple.bat
goto end

:both
echo.
echo Running Both Test Suites...
echo =========================
echo.
echo Step 1: Native Tests
echo --------------------
call run_native_tests_msvc.bat
if errorlevel 1 (
    echo Native tests failed. Continue with ESP-IDF tests? (y/n)
    set /p continue=
    if /i not "%continue%"=="y" goto end
)

echo.
echo Step 2: ESP-IDF Tests
echo ---------------------
call run_tests_simple.bat

:end
echo.
echo Test suite execution complete!
pause 