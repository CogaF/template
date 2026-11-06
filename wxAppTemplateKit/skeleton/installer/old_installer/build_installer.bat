@echo off
rem Copyright (C) 2024-2026 Fation Coga. All rights reserved.
rem This file is part of Template App - see LICENSE.md.
rem
rem Thin wrapper for the CogaF shared installer kit:
rem   1) Sets the per-project vars (APP_NAME, APP_EXE, DATA_FOLDER, VERSION_HEADER, SLN_FILE,
rem      ISS_FILE).
rem   2) Finds the shared installer_common.bat in this order:
rem         a) %COGAF_SHARED_REDIST%\installer_common.bat  (env var override)
rem         b) ..\..\redist\installer_common.bat           (default shared folder, one up from
rem                                                        the projects parent, next to the
rem                                                        VC redist exes)
rem      If neither is present, falls back to the LOCAL copy in this project's installer\
rem      folder AFTER asking the operator for confirmation (so you know you may be running on a
rem      stale version).
rem   3) Calls the chosen installer_common.bat, forwarding every arg.
rem
rem Usage mirrors installer_common.bat:
rem   installer\build_installer.bat                 the builds of Builds\
rem   installer\build_installer.bat build           first builds Release and Release_DLL (MSBuild)
rem   installer\build_installer.bat E:\Builds       use a different Builds root
rem   installer\build_installer.bat -y              never prompt (CI / git hook); auto-use local

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
if defined COGAF_SHARED_REDIST set "SHARED_PATH=%COGAF_SHARED_REDIST%\installer_common.bat"
set "PARENT_PATH="
pushd "%REPO%\..\.." 2>nul
if not errorlevel 1 ( set "PARENT_PATH=!CD!\redist\installer_common.bat" & popd )

if defined SHARED_PATH if exist "%SHARED_PATH%" set "COMMON=%SHARED_PATH%"
if not defined COMMON if defined PARENT_PATH if exist "%PARENT_PATH%" set "COMMON=%PARENT_PATH%"

if defined COMMON (
    echo Using shared installer_common: %COMMON%
) else (
    set "LOCAL=%REPO%\installer\installer_common.bat"
    if not exist "!LOCAL!" (
        echo.
        echo installer_common.bat not found at any known location:
        if defined SHARED_PATH echo   %SHARED_PATH%
        if defined PARENT_PATH echo   %PARENT_PATH%
        echo   !LOCAL!  ^(local fallback - also missing^)
        echo.
        echo Clone or copy the shared installer kit into one of those folders.
        goto :fail
    )
    echo.
    echo Shared installer_common.bat not found. Checked:
    if defined SHARED_PATH echo   %SHARED_PATH%
    if defined PARENT_PATH echo   %PARENT_PATH%
    for %%F in ("!LOCAL!") do set "LOCAL_DATE=%%~tF"
    echo.
    echo A LOCAL copy is available: !LOCAL! ^(dated !LOCAL_DATE!^).
    echo Note: the local copy may be older than the shared one.
    if "!ASSUMEYES!"=="1" (
        echo -y given: using the local copy without prompting.
        set "COMMON=!LOCAL!"
    ) else (
        set /p USE_LOCAL=Use the local copy? [y/N]
        if /i "!USE_LOCAL!"=="y"   set "COMMON=!LOCAL!"
        if /i "!USE_LOCAL!"=="yes" set "COMMON=!LOCAL!"
        if not defined COMMON (
            echo Aborted. To stop seeing this prompt, either:
            if defined PARENT_PATH echo   - put installer_common.bat at %PARENT_PATH%
            echo   - or set COGAF_SHARED_REDIST to a folder containing it.
            goto :fail
        )
    )
)

call "%COMMON%" %*
exit /b %ERRORLEVEL%

:fail
echo.
pause
exit /b 1
