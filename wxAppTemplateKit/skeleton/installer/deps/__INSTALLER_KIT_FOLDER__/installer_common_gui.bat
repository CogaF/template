@echo off
rem Copyright (C) 2024-2026 Fation Coga.
rem
rem GUI entry point for the shared CogaF installer driver. A project's build_installer_withGui.bat
rem sets the usual project env vars (APP_NAME, APP_EXE, DATA_FOLDER, VERSION_HEADER, SLN_FILE,
rem ISS_FILE, REPO, optional BUILDS) and calls this script. We show a PowerShell WinForms selector,
rem export its answers as env vars, then delegate to installer_common.bat.
rem
rem To bypass the GUI (e.g. from CI), set INSTALLER_GUI=skip before calling.

setlocal EnableExtensions

if /i "%INSTALLER_GUI%"=="skip" goto run_core

rem -------- Resolve the GUI script path --------
set "GUI_PS="
if exist "%~dp0gui\build_selector.ps1" set "GUI_PS=%~dp0gui\build_selector.ps1"
if not defined GUI_PS if defined COGAF_SHARED_REDIST if exist "%COGAF_SHARED_REDIST%\gui\build_selector.ps1" set "GUI_PS=%COGAF_SHARED_REDIST%\gui\build_selector.ps1"
if not defined GUI_PS if exist "%~dp0..\..\redist\gui\build_selector.ps1" set "GUI_PS=%~dp0..\..\redist\gui\build_selector.ps1"
if not defined GUI_PS (
    echo [installer_common_gui] GUI selector script not found. Falling back to full-build.
    goto run_core
)

rem -------- Probe which variants are built --------
if not defined BUILDS set "BUILDS=%REPO%\Builds"
set "HAS_X64_STATIC=0"
set "HAS_X64_DYNAMIC=0"
set "HAS_X86_STATIC=0"
set "HAS_X86_DYNAMIC=0"
if exist "%BUILDS%\x64\Release\%APP_EXE%"      set "HAS_X64_STATIC=1"
if exist "%BUILDS%\x64\Release_DLL\%APP_EXE%"  set "HAS_X64_DYNAMIC=1"
if exist "%BUILDS%\Win32\Release\%APP_EXE%"    set "HAS_X86_STATIC=1"
if exist "%BUILDS%\Win32\Release_DLL\%APP_EXE%" set "HAS_X86_DYNAMIC=1"

rem -------- Launch the GUI --------
set "GUI_OUT=%TEMP%\cogaf_installer_gui_%RANDOM%.txt"
powershell -ExecutionPolicy Bypass -NoProfile -File "%GUI_PS%" -AppName "%APP_NAME%" -HasX64Static %HAS_X64_STATIC% -HasX64Dynamic %HAS_X64_DYNAMIC% -HasX86Static %HAS_X86_STATIC% -HasX86Dynamic %HAS_X86_DYNAMIC% -OutFile "%GUI_OUT%"
if errorlevel 1 (
    echo [installer_common_gui] GUI was cancelled. Nothing was built.
    if exist "%GUI_OUT%" del "%GUI_OUT%"
    endlocal
    exit /b 2
)

rem -------- Load the GUI answers as env vars --------
for /f "usebackq tokens=1,* delims==" %%A in ("%GUI_OUT%") do set "GUI_%%A=%%B"
if exist "%GUI_OUT%" del "%GUI_OUT%"

rem Translate GUI_* into flag defines passed as /D to ISCC via installer_common.bat.
set "INSTALLER_SKIP_X64_STATIC=0"
set "INSTALLER_SKIP_X64_DYNAMIC=0"
set "INSTALLER_SKIP_X86_STATIC=0"
set "INSTALLER_SKIP_X86_DYNAMIC=0"
if "%GUI_INCLUDE_X64_STATIC%"=="0"  set "INSTALLER_SKIP_X64_STATIC=1"
if "%GUI_INCLUDE_X64_DYNAMIC%"=="0" set "INSTALLER_SKIP_X64_DYNAMIC=1"
if "%GUI_INCLUDE_X86_STATIC%"=="0"  set "INSTALLER_SKIP_X86_STATIC=1"
if "%GUI_INCLUDE_X86_DYNAMIC%"=="0" set "INSTALLER_SKIP_X86_DYNAMIC=1"

set "INSTALLER_EXTRA_DEFS="
if /i "%GUI_PRIV%"=="admin" set "INSTALLER_EXTRA_DEFS=/DPrivReqAdmin"
if /i "%GUI_PRIV%"=="user"  set "INSTALLER_EXTRA_DEFS=/DPrivReqUser"
if "%GUI_PREV_AWARE%"=="0"  set "INSTALLER_EXTRA_DEFS=%INSTALLER_EXTRA_DEFS% /DNoPrevInstallCheck"

:run_core
rem -------- Delegate to the core driver --------
set "CORE=%~dp0installer_common.bat"
if not exist "%CORE%" (
    echo [installer_common_gui] installer_common.bat not found next to this script: %CORE%
    endlocal
    exit /b 1
)
call "%CORE%" %*
set "RC=%ERRORLEVEL%"
endlocal & exit /b %RC%
