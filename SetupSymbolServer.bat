@echo off
REM ========================================
REM Mundi Engine Symbol Server Setup Script
REM ========================================
REM
REM This script helps team members configure their development environment
REM to use the Mundi symbol server for debugging.
REM
REM Symbol Server: \\172.21.11.115\symbols
REM

echo ================================================
echo Mundi Engine Symbol Server Setup
echo ================================================
echo.

REM Test network connectivity to symbol server
echo [1/3] Testing connection to symbol server...
ping -n 1 172.21.11.115 >nul 2>&1
if errorlevel 1 (
    echo ERROR: Cannot reach symbol server at 172.21.11.115
    echo Please check your network connection.
    echo.
    pause
    exit /b 1
)
echo SUCCESS: Symbol server is reachable
echo.

REM Test access to network share
echo [2/3] Testing access to network share...
if exist "\\172.21.11.115\symbols\" (
    echo SUCCESS: Can access \\172.21.11.115\symbols
) else (
    echo WARNING: Cannot access \\172.21.11.115\symbols
    echo You may need to provide credentials or request access.
)
echo.

REM Check if symstore.exe exists
echo [3/3] Checking symstore.exe installation...
if exist "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe" (
    echo SUCCESS: symstore.exe found
    echo Your builds will automatically publish symbols to the server
) else (
    echo WARNING: symstore.exe not found
    echo.
    echo Symbols will NOT be published when you build the project.
    echo To fix this, install Windows Debugging Tools:
    echo   1. Open Visual Studio Installer
    echo   2. Click "Modify" on Visual Studio 2022
    echo   3. Go to "Individual components" tab
    echo   4. Check "Debugging Tools for Windows"
    echo   5. Click "Modify" to install
)
echo.

echo ================================================
echo Visual Studio Configuration Instructions
echo ================================================
echo.
echo To enable symbol server debugging in Visual Studio 2022:
echo.
echo 1. Open Visual Studio 2022
echo 2. Go to: Tools ^> Options ^> Debugging ^> Symbols
echo 3. Click the folder icon to add a new symbol location
echo 4. Add: \\172.21.11.115\symbols
echo 5. (Optional) Enable "Microsoft Symbol Servers" for Windows symbols
echo 6. (Optional) Set cache directory: C:\SymbolCache
echo 7. Click OK to save
echo.
echo ================================================
echo Setup Complete!
echo ================================================
echo.

pause
