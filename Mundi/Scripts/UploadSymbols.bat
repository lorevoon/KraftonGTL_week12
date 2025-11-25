@echo off
REM ========================================
REM Mundi Engine Symbol Upload Script
REM ========================================
REM
REM Uploads EXE and PDB files to Symbol Server
REM Usage: Run from project root directory
REM

echo ================================================
echo Mundi Symbol Server Upload
echo ================================================
echo.

REM Configuration
set SYMSTORE="C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe"
set SYMBOLS=\\172.21.11.115\symbols
set PLATFORM=x64

REM Check if symstore.exe exists
if not exist %SYMSTORE% (
    echo ERROR: symstore.exe not found
    echo.
    echo Please install Windows Debugging Tools:
    echo   1. Open Visual Studio Installer
    echo   2. Modify Visual Studio 2022
    echo   3. Individual Components tab
    echo   4. Check "Debugging Tools for Windows"
    echo.
    pause
    exit /b 1
)

REM Select Configuration
echo Select build configuration:
echo   1. Debug
echo   2. Release
echo.
set /p CHOICE=Enter choice (1 or 2):

if "%CHOICE%"=="1" (
    set CONFIG=Debug
) else if "%CHOICE%"=="2" (
    set CONFIG=Release
) else (
    echo Invalid choice
    pause
    exit /b 1
)

echo.
echo Selected: %CONFIG%
echo.

REM Change to project root directory (2 levels up from Scripts)
cd /d "%~dp0..\.."

REM Check if build exists
if not exist "Binaries\%CONFIG%\Mundi.exe" (
    echo ERROR: Build not found at Binaries\%CONFIG%\Mundi.exe
    echo Current directory: %CD%
    echo Please build the project first.
    echo.
    pause
    exit /b 1
)

REM Upload to Symbol Server
echo ================================================
echo Uploading to Symbol Server
echo ================================================
echo.
echo Target: %SYMBOLS%
echo Configuration: %CONFIG%
echo.

REM Upload EXE
echo [1/2] Uploading Mundi.exe...
%SYMSTORE% add /r /f "Binaries\%CONFIG%\Mundi.exe" /s %SYMBOLS% /t "Mundi" /v "%CONFIG%_%PLATFORM%" /compress
echo.

REM Upload PDB
echo [2/2] Uploading PDB files...
%SYMSTORE% add /r /f "Binaries\%CONFIG%\*.pdb" /s %SYMBOLS% /t "Mundi" /v "%CONFIG%_%PLATFORM%" /compress
echo.

echo ================================================
echo Upload Complete!
echo ================================================
echo.
echo Symbols uploaded to: %SYMBOLS%
echo Configuration: %CONFIG%
echo.
pause
