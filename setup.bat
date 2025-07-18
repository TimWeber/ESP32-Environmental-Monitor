@echo off
setlocal enabledelayedexpansion
cls
echo.
echo ========================================
echo    ESP32 Environmental Monitor Setup
echo ========================================
echo.
echo This script will help you configure your ESP32 Environmental Monitor
echo for first-time use.
echo.

REM Check if Python is available for JSON processing
python --version >nul 2>&1
if !errorlevel! neq 0 (
    echo ERROR: Python is required but not found.
    echo Please install Python and try again.
    echo.
    pause
    exit /b 1
)

REM Check if template exists
if not exist "config\settings.template.jsonc" (
    echo ERROR: Template file not found: config\settings.template.jsonc
    echo Please ensure you have the complete project files.
    echo.
    pause
    exit /b 1
)

echo Step 1: Creating data directory...
if not exist "data" mkdir data

echo.
echo Step 2: Converting template to JSON...
REM Create a Python script to strip comments and convert JSONC to JSON
set "PYTHON_SCRIPT=temp_convert.py"
echo import re > !PYTHON_SCRIPT!
echo import sys >> !PYTHON_SCRIPT!
echo. >> !PYTHON_SCRIPT!
echo def strip_jsonc_comments(content): >> !PYTHON_SCRIPT!
echo     # Remove single-line comments (// ...) >> !PYTHON_SCRIPT!
echo     content = re.sub(r'//.*$', '', content, flags=re.MULTILINE) >> !PYTHON_SCRIPT!
echo     # Remove multi-line comments (/* ... */) >> !PYTHON_SCRIPT!
echo     content = re.sub(r'/\*.*?\*/', '', content, flags=re.DOTALL) >> !PYTHON_SCRIPT!
echo     # Remove empty lines and clean up whitespace >> !PYTHON_SCRIPT!
echo     lines = content.split('\n') >> !PYTHON_SCRIPT!
echo     cleaned_lines = [] >> !PYTHON_SCRIPT!
echo     for line in lines: >> !PYTHON_SCRIPT!
echo         stripped = line.strip() >> !PYTHON_SCRIPT!
echo         if stripped: >> !PYTHON_SCRIPT!
echo             cleaned_lines.append(line) >> !PYTHON_SCRIPT!
echo     return '\n'.join(cleaned_lines) >> !PYTHON_SCRIPT!
echo. >> !PYTHON_SCRIPT!
echo if __name__ == '__main__': >> !PYTHON_SCRIPT!
echo     with open('config/settings.template.jsonc', 'r', encoding='utf-8') as f: >> !PYTHON_SCRIPT!
echo         content = f.read() >> !PYTHON_SCRIPT!
echo     cleaned_content = strip_jsonc_comments(content) >> !PYTHON_SCRIPT!
echo     with open('data/settings.json', 'w', encoding='utf-8') as f: >> !PYTHON_SCRIPT!
echo         f.write(cleaned_content) >> !PYTHON_SCRIPT!

python !PYTHON_SCRIPT!
if !errorlevel! neq 0 (
    echo ERROR: Failed to convert template file.
    echo.
    pause
    exit /b 1
)

del !PYTHON_SCRIPT!
echo [OK] Template converted successfully

echo.
echo ========================================
echo        Configuration Setup
echo ========================================
echo.
echo Please provide the following information for your ESP32:
echo.

REM Get WiFi SSID
set /p WIFI_SSID="Enter your WiFi SSID: "
if "!WIFI_SSID!"=="" (
    echo ERROR: WiFi SSID cannot be empty.
    echo.
    pause
    exit /b 1
)

REM Get WiFi Password
set /p WIFI_PASSWORD="Enter your WiFi password: "
if "!WIFI_PASSWORD!"=="" (
    echo ERROR: WiFi password cannot be empty.
    echo.
    pause
    exit /b 1
)

REM Get Server URL
set /p SERVER_URL="Enter your server URL (e.g., http://your-server.com:8080/api/data): "
if "!SERVER_URL!"=="" (
    echo ERROR: Server URL cannot be empty.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo        Updating Configuration
echo ========================================
echo.

REM Create Python script to update the JSON file
set "UPDATE_SCRIPT=temp_update.py"
echo import json >> !UPDATE_SCRIPT!
echo import sys >> !UPDATE_SCRIPT!
echo. >> !UPDATE_SCRIPT!
echo # Read current settings >> !UPDATE_SCRIPT!
echo with open('data/settings.json', 'r', encoding='utf-8') as f: >> !UPDATE_SCRIPT!
echo     settings = json.load(f) >> !UPDATE_SCRIPT!
echo. >> !UPDATE_SCRIPT!
echo # Update WiFi settings >> !UPDATE_SCRIPT!
echo settings['connectivity']['wifi']['ssid'] = sys.argv[1] >> !UPDATE_SCRIPT!
echo settings['connectivity']['wifi']['password'] = sys.argv[2] >> !UPDATE_SCRIPT!
echo. >> !UPDATE_SCRIPT!
echo # Update server URL >> !UPDATE_SCRIPT!
echo settings['data_transmission']['server']['url'] = sys.argv[3] >> !UPDATE_SCRIPT!
echo. >> !UPDATE_SCRIPT!
echo # Write updated settings >> !UPDATE_SCRIPT!
echo with open('data/settings.json', 'w', encoding='utf-8') as f: >> !UPDATE_SCRIPT!
echo     json.dump(settings, f, indent=2) >> !UPDATE_SCRIPT!

python !UPDATE_SCRIPT! "!WIFI_SSID!" "!WIFI_PASSWORD!" "!SERVER_URL!"
if !errorlevel! neq 0 (
    echo ERROR: Failed to update configuration file.
    echo.
    pause
    exit /b 1
)

del !UPDATE_SCRIPT!
echo [OK] Configuration updated successfully

echo.
echo ========================================
echo        Setup Complete!
echo ========================================
echo.
echo Your ESP32 Environmental Monitor is now configured with:
echo.
echo WiFi SSID: !WIFI_SSID!
echo Server URL: !SERVER_URL!
echo.
echo Next steps:
echo 1. Connect your ESP32 to your computer
echo 2. Run 'run.bat' to build and flash the firmware
echo 3. Monitor the serial output for connection status
echo.
echo Configuration file: data/settings.json
echo.
echo Press any key to exit...
pause >nul 