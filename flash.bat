@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo     3DBRINT MINER - OTOMATIK YUKLEME ARACI
echo ===================================================

set ARDUINO_CLI="C:\Users\prome\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
set FQBN=esp32:esp32:esp32c3:PartitionScheme=huge_app,CDCOnBoot=cdc
set BUILD_DIR="c:\Users\prome\Desktop\nerdminer\build"

echo Cihaz araniyor...
%ARDUINO_CLI% board list

echo.
echo Cihazin bagli oldugu COM portunu yazin (Ornek: COM11):
set /p PORT=Port: 

if "%PORT%"=="" (
    set PORT=COM11
)

echo %PORT% portuna yukleniyor...
%ARDUINO_CLI% upload -p %PORT% --fqbn %FQBN% --input-dir %BUILD_DIR%

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ===================================================
    echo      TEBRIKLER! YUKLEME BASARIYLA TAMAMLANDI!
    echo ===================================================
) else (
    echo.
    echo [HATA] Yukleme basarisiz oldu. Lutfen kabloyu ve COM portunu kontrol edin.
)

pause
