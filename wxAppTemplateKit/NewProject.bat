@echo off
rem Copyright (C) 2026 Fation Coga - SPDX-License-Identifier: LGPL-3.0-or-later
rem Creates a new application from this kit - double-click, or pass the same options as NewProject.ps1
rem (e.g. NewProject.bat -Name "Reactor Control" -GitHub -Visibility private).
rem Nothing here points to the kit author's accounts: git and GitHub use whoever is signed in on this PC.
title New wxWidgets application
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0NewProject.ps1" %*
set RESULT=%ERRORLEVEL%
echo.
pause
exit /b %RESULT%
