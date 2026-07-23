@echo off
title Laundry IoT - Khoi dong server
echo.
echo  ================================
echo   LAUNDRY 24/7 - Khoi dong...
echo  ================================
echo.

:: %~dp0 = thu muc chua file .bat nay (tranh loi tieng Viet trong duong dan)
cd /d "%~dp0laundry-iot-backend"

if %errorlevel% neq 0 (
    echo [LOI] Khong tim thay thu muc laundry-iot-backend!
    pause
    exit /b 1
)

node tunnel.js
pause
