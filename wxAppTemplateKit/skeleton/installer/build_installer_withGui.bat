@echo off
rem Copyright (C) 2024-2026 Fation Coga. All rights reserved.
rem This file is part of Template App - see LICENSE.md.
rem
rem Thin wrapper for the CogaF shared installer kit, GUI flavour: shows a selector (architecture /
rem linking / privilege / previous-install awareness) and builds a narrower, lighter setup.
rem   1) Sets the per-project vars (APP_NAME, APP_EXE, DATA_FOLDER, VERSION_HEADER, SLN_FILE,
rem      ISS_FILE, REPO).
rem   2) Finds installer_common_gui.bat in this order:
rem         a) %COGAF_SHARED_REDIST%\installer_common_gui.bat  (env var override)
rem         b) ..\..\redist\installer_common_gui.bat           (default shared folder)
rem         c) the LOCAL copy in this project's installer\ folder (asks first, like build_installer.bat)
rem      If the GUI variant is missing everywhere, falls back to installer_common.bat (same lookup
rem      order) and so to the full build without the selector.
rem   3) Calls the chosen script, forwarding every arg.
rem
rem Usage mirrors build_installer.bat (see there); -y never prompts and skips nothing else.

setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0.."
set "REPO=%CD%"

rem --- Project-specific
set "APP_NAME=Template App"
set "APP_EXE=Template App.exe"
set "DATA_FOLDER=Template App data"
set "VERSION_HEADER=include\Version.h"
set "SLN_FILE=TemplateApp.sln"
set "ISS_FILE=%REPO%\installer\TemplateApp.iss"

set "ASSUMEYES=0"
for %%A in (%*) do ( if /i "%%~A"=="-y" set "ASSUMEYES=1"
                     if /i "%%~A"=="/y" set "ASSUMEYES=1" )

set "COMMON="
set "KIND="
for %%N in (installer_common_gui.bat installer_common.bat) do (
    if not defined COMMON call :find "%%N"
)
if not defined COMMON goto :missing
echo Using shared %KIND%: %COMMON%
if /i "%KIND%"=="installer_common.bat" echo ^(GUI variant not found: running the full build without the selector.^)

call "%COMMON%" %*
exit /b %ERRORLEVEL%

rem --- :find <name>  sets COMMON / KIND when the file exists at a, b or (confirmed) c
:find
set "NAME=%~1"
set "SHARED_PATH="
if defined COGAF_SHARED_REDIST set "SHARED_PATH=%COGAF_SHARED_REDIST%\%NAME%"
set "PARENT_PATH="
pushd "%REPO%\..\.." 2>nul
if not errorlevel 1 ( set "PARENT_PATH=!CD!\redist\%NAME%" & popd )
set "LOCAL=%REPO%\installer\%NAME%"
if defined SHARED_PATH if exist "!SHARED_PATH!" ( set "COMMON=!SHARED_PATH!" & set "KIND=%NAME%" & exit /b 0 )
if defined PARENT_PATH if exist "!PARENT_PATH!" ( set "COMMON=!PARENT_PATH!" & set "KIND=%NAME%" & exit /b 0 )
if exist "!LOCAL!" (
    for %%F in ("!LOCAL!") do set "LOCAL_DATE=%%~tF"
    echo.
    echo Shared %NAME% not found; a LOCAL copy is available: !LOCAL! ^(dated !LOCAL_DATE!^).
    echo Note: the local copy may be older than the shared one.
    if "!ASSUMEYES!"=="1" (
        echo -y given: using the local copy without prompting.
        set "COMMON=!LOCAL!" & set "KIND=%NAME%"
    ) else (
        set "USE_LOCAL="
        set /p USE_LOCAL=Use the local copy? [y/N]
        if /i "!USE_LOCAL!"=="y"   ( set "COMMON=!LOCAL!" & set "KIND=%NAME%" )
        if /i "!USE_LOCAL!"=="yes" ( set "COMMON=!LOCAL!" & set "KIND=%NAME%" )
    )
)
exit /b 0

:missing
echo.
echo Neither installer_common_gui.bat nor installer_common.bat was found. Checked:
if defined COGAF_SHARED_REDIST echo   %COGAF_SHARED_REDIST%
echo   %REPO%\..\..\redist
echo   %REPO%\installer  ^(local fallback^)
echo Clone or copy the shared installer kit into one of those folders, or set COGAF_SHARED_REDIST.
echo.
pause
exit /b 1
