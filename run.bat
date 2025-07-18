@echo off
setlocal enabledelayedexpansion
cls
echo.
echo ========================================
echo     ESP32 Flash and Monitor Script
echo ========================================
echo.

echo.
echo Step 1: Checking if settings have been modified...
if not exist "spiffs" mkdir spiffs

REM Check if settings file exists
if not exist "data\settings.json" (
    echo.
    echo ========================================
    echo        ERROR: No settings file found!
    echo ========================================
    echo.
    echo The file 'data/settings.json' does not exist.
    echo.
    echo Please:
    echo 1. Copy 'config/settings.template.jsonc' to 'data/settings.json'
    echo 2. Edit 'data/settings.json' with your actual settings
    echo 3. Run this script again
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

REM Check if settings file contains template values
findstr /c:"YOUR_WIFI_SSID" "data\settings.json" >nul 2>&1
if !errorlevel! equ 0 (
    echo.
    echo ====================================================
    echo    ERROR: Settings file contains template values!
    echo ====================================================
    echo.
    echo The file 'data/settings.json' contains template values.
    echo Please edit it with your actual settings:
    echo.
    echo - Replace 'YOUR_WIFI_SSID' with your WiFi network name
    echo - Replace 'YOUR_WIFI_PASSWORD' with your WiFi password
    echo - Replace 'your-server.local' with your server URL
    echo - Update validation thresholds as needed
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

REM Check if spiffs/settings.json exists
if not exist "spiffs\settings.json" (
    echo Settings file not found in spiffs, copying and building...
    set NEED_BUILD=1
) else (
    REM Compare files using fc command
    fc "data\settings.json" "spiffs\settings.json" >nul 2>&1
    if !errorlevel! equ 0 (
        echo.
        echo [OK] Settings unchanged, skipping build
        set NEED_BUILD=0
    ) else (
        echo Settings modified, copying and building...
        set NEED_BUILD=1
    )
)

if !NEED_BUILD!==1 (
    echo.
    echo Step 2: Copying latest settings.json to spiffs directory...
    copy "data\settings.json" "spiffs\settings.json" >nul 2>&1
    if !errorlevel! equ 0 (
        echo.
        echo [OK] Settings copied successfully
    ) else (
        echo.
        echo [ERROR] Failed to copy settings.json
        pause
        exit /b 1
    )

    echo.
    echo Step 3: Building project with latest settings...
    idf.py build
    if !errorlevel! equ 0 (
        echo.
        echo [OK] Build completed successfully
    ) else (
        echo.
        echo [ERROR] Build failed
        pause
        exit /b 1
    )
) else (
    echo.
    echo Step 2: [OK] Skipping build - settings unchanged
)

if !NEED_BUILD!==1 (
    echo.
    echo Step 4: Flashing ESP32...
    idf.py flash
    if !errorlevel! equ 0 (
        echo.
        echo [OK] Flash completed successfully
    ) else (
        echo.
        echo [ERROR] Flash failed
        echo Please check:
        echo - ESP32 is connected
        echo - Correct COM port is set
        echo - ESP32 is in download mode
        pause
        exit /b 1
    )
) else (
    echo.
    echo Step 3: [OK] Skipping flash - no build performed
)

echo.
if !NEED_BUILD!==1 (
    echo Step 5: Starting monitor...
) else (
    echo Step 4: Starting monitor...
)
echo Press Ctrl+] to exit monitor
echo.
idf.py monitor

echo.
echo =====================================================
echo        Monitor session ended
echo =====================================================
pause 