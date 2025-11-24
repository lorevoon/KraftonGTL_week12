@echo off
setlocal enabledelayedexpansion
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
echo [1/4] Testing connection to symbol server...
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
echo [2/4] Testing access to network share...
if exist "\\172.21.11.115\symbols\" (
    echo SUCCESS: Can access \\172.21.11.115\symbols
) else (
    echo WARNING: Cannot access \\172.21.11.115\symbols
    echo You may need to provide credentials or request access.
)
echo.

REM Check if symstore.exe exists
echo [3/4] Checking symstore.exe installation...
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

REM Set _NT_SYMBOL_PATH environment variable with Symbol Server protocol
echo [4/4] Setting up _NT_SYMBOL_PATH environment variable...
setx _NT_SYMBOL_PATH "srv*C:\SymbolCache*\\172.21.11.115\symbols" >nul 2>&1
if errorlevel 1 (
    echo WARNING: Failed to set _NT_SYMBOL_PATH environment variable
    echo You may need to run this script as Administrator.
) else (
    echo SUCCESS: _NT_SYMBOL_PATH set with Symbol Server protocol
    echo   Format: srv*C:\SymbolCache*\\172.21.11.115\symbols
    echo   - Local cache: C:\SymbolCache
    echo   - Symbol server: \\172.21.11.115\symbols

    REM Verify the environment variable was set correctly
    echo.
    echo Verifying environment variable...
    for /f "tokens=2*" %%a in ('reg query "HKEY_CURRENT_USER\Environment" /v _NT_SYMBOL_PATH 2^>nul ^| findstr /i "_NT_SYMBOL_PATH"') do (
        set VERIFIED_PATH=%%b
    )

    if defined VERIFIED_PATH (
        echo VERIFIED: _NT_SYMBOL_PATH = !VERIFIED_PATH!
        echo Visual Studio will use this path for symbol lookup after restart.
    ) else (
        echo WARNING: Could not verify environment variable in registry
    )
)
echo.

echo ================================================
echo Visual Studio Configuration Instructions
echo ================================================
echo.
echo The _NT_SYMBOL_PATH environment variable has been set.
echo Visual Studio will automatically use this path for symbol lookup.
echo.
echo IMPORTANT: You must restart Visual Studio for changes to take effect.
echo.
echo Optional: You can also manually verify in Visual Studio:
echo   Tools ^> Options ^> Debugging ^> Symbols
echo   - Check "Environment Variable: _NT_SYMBOL_PATH" is enabled
echo   - (Optional) Set cache directory: C:\SymbolCache
echo.
echo ================================================
echo Setup Complete!
echo ================================================
echo.

pause
