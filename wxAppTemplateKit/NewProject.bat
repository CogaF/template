@echo off
rem Copyright (C) 2026 Fation Coga - SPDX-License-Identifier: LGPL-3.0-or-later
rem Creates a new application from this kit - double-click, or pass the same options as NewProject.ps1.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0NewProject.ps1" %*
echo.
pause
