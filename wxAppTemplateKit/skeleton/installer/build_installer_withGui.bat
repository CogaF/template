@echo off
rem Copyright (C) 2024-2026 Fation Coga. All rights reserved.
rem This file is part of Template App - see LICENSE.md.
rem
rem Thin wrapper for the CogaF shared installer kit, GUI flavour: shows a selector (architecture /
rem linking / privilege / previous-install awareness) and builds a narrower, lighter setup.
rem   1) Sets the per-project vars (APP_NAME, APP_EXE, DATA_FOLDER, VERSION_HEADER, SLN_FILE,
rem      ISS_FILE, REPO).
rem   2) Finds installer_common_gui.bat in this order:
rem         a) %__INSTALLER_ENV_VAR__%\installer_common_gui.bat   (installer-kit env var)
rem         b) deps\__INSTALLER_KIT_FOLDER__\installer_common_gui.bat   (bundled copy, see deps\README.md)
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

rem --- :find <name>  sets COMMON / KIND when the file exists in the env var folder or the bundled copy
:find
set "NAME=%~1"
if defined __INSTALLER_ENV_VAR__ if exist "%__INSTALLER_ENV_VAR__%\%NAME%" ( set "COMMON=%__INSTALLER_ENV_VAR__%\%NAME%" & set "KIND=%NAME%" & exit /b 0 )
set "BUNDLED=%~dp0deps\__INSTALLER_KIT_FOLDER__\%NAME%"
if exist "!BUNDLED!" (
    echo Env var __INSTALLER_ENV_VAR__ has no %NAME%: using the bundled copy !BUNDLED!
    set "COMMON=!BUNDLED!" & set "KIND=%NAME%"
)
exit /b 0

:missing
echo.
echo Neither installer_common_gui.bat nor installer_common.bat was found. Checked:
if defined __INSTALLER_ENV_VAR__ echo   %__INSTALLER_ENV_VAR__%
echo   %~dp0deps\__INSTALLER_KIT_FOLDER__  ^(bundled copy^)
echo Restore installer\deps or set __INSTALLER_ENV_VAR__ to a folder holding the installer kit.
echo.
pause
exit /b 1
