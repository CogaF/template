@echo off
rem Copyright (C) 2024-2026 Fation Coga. All rights reserved.
rem This file is part of Template App - see LICENSE.md.
rem
rem Thin wrapper for the CogaF shared installer kit:
rem   1) Sets the per-project vars (APP_NAME, APP_EXE, DATA_FOLDER, VERSION_HEADER, SLN_FILE,
rem      ISS_FILE).
rem   2) Finds the shared installer_common.bat in this order:
rem         a) %__INSTALLER_ENV_VAR__%\installer_common.bat   (the installer-kit env var chosen when
rem                                                          this project was created)
rem         b) deps\__INSTALLER_KIT_FOLDER__\installer_common.bat   (the copy bundled in this project;
rem                                                          see installer\deps\README.md)
rem      The bundled copy may be older than the shared kit, so using it is announced.
rem   3) Calls the chosen installer_common.bat, forwarding every arg.
rem
rem Usage mirrors installer_common.bat:
rem   installer\build_installer.bat                 the builds of Builds\
rem   installer\build_installer.bat build           first builds Release and Release_DLL (MSBuild)
rem   installer\build_installer.bat E:\Builds       use a different Builds root
rem   installer\build_installer.bat -y              never prompt (CI / git hook)

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

rem --- Was -y passed? Then skip the shared-vs-local prompt and silently use the best available.
set "ASSUMEYES=0"
for %%A in (%*) do ( if /i "%%~A"=="-y" set "ASSUMEYES=1"
                     if /i "%%~A"=="/y" set "ASSUMEYES=1" )

rem --- Find the shared common script
set "COMMON="
set "SHARED_PATH="
if defined __INSTALLER_ENV_VAR__ set "SHARED_PATH=%__INSTALLER_ENV_VAR__%\installer_common.bat"
set "BUNDLED_PATH=%~dp0deps\__INSTALLER_KIT_FOLDER__\installer_common.bat"

if defined SHARED_PATH if exist "%SHARED_PATH%" set "COMMON=%SHARED_PATH%"
if defined COMMON (
    echo Using shared installer_common ^(env var __INSTALLER_ENV_VAR__^): !COMMON!
) else (
    if not exist "%BUNDLED_PATH%" (
        echo.
        echo installer_common.bat not found. Checked:
        echo   %%__INSTALLER_ENV_VAR__%%  ^(!__INSTALLER_ENV_VAR__!^)
        echo   !BUNDLED_PATH!  ^(bundled copy - also missing^)
        echo.
        echo Restore installer\deps or set __INSTALLER_ENV_VAR__ to a folder holding the installer kit.
        goto :fail
    )
    echo Env var __INSTALLER_ENV_VAR__ not usable: using the bundled copy !BUNDLED_PATH!
    set "COMMON=!BUNDLED_PATH!"
)

call "%COMMON%" %*
exit /b %ERRORLEVEL%

:fail
echo.
pause
exit /b 1
