@echo off
rem Copyright (C) 2024-2026 Fation Coga. All rights reserved.
rem
rem Shared Inno Setup installer driver, used by every CogaF project's own
rem installer\build_installer.bat (thin wrapper). The wrapper just sets the per-project vars and
rem calls this script. Centralising the heavy lifting means a bug fix / improvement here benefits
rem every project at once; each project keeps a LOCAL COPY of this file in its own installer\
rem folder as a graceful fallback (see the wrapper for the shared-vs-local prompt).
rem
rem Expected inputs (set by the caller):
rem   REPO            - absolute path of the project root (parent of installer\)
rem   APP_NAME        - "Reactor Control", "SQLite DB Tools", ...
rem   APP_EXE         - "Reactor Control.exe"
rem   DATA_FOLDER     - "Reactor Control data"
rem   VERSION_HEADER  - path from REPO to the header with APP_VERSION_* defines
rem   SLN_FILE        - name of the .sln (for the optional "build" arg)
rem   ISS_FILE        - absolute path to the project's Inno Setup .iss
rem
rem Optional:
rem   COGAF_SHARED_REDIST - absolute path of a shared folder that holds vc_redist.*.exe (and this
rem                         very script). If set, it wins over ..\..\redist. If unset, the lookup
rem                         defaults to ..\..\redist first, then installer\redist in the project.
rem   BUILDS              - override the Builds root (default: REPO\Builds)
rem
rem Positional args (same as the per-project wrapper forwards):
rem   build       also MSBuild the two x64 Release configurations before packaging
rem   -y          assume Yes to the "only one variant found" prompt (CI / git hook)
rem   <path>      use that path as the Builds root

setlocal EnableExtensions EnableDelayedExpansion

if not defined REPO           ( echo installer_common: REPO not set            & exit /b 1 )
if not defined APP_NAME       ( echo installer_common: APP_NAME not set        & exit /b 1 )
if not defined APP_EXE        ( echo installer_common: APP_EXE not set         & exit /b 1 )
if not defined DATA_FOLDER    ( echo installer_common: DATA_FOLDER not set     & exit /b 1 )
if not defined VERSION_HEADER ( echo installer_common: VERSION_HEADER not set  & exit /b 1 )
if not defined SLN_FILE       ( echo installer_common: SLN_FILE not set        & exit /b 1 )
if not defined ISS_FILE       ( echo installer_common: ISS_FILE not set        & exit /b 1 )

rem Wrapper may have pre-set BUILDS to a non-standard layout (e.g. wxHexLab uses build\winVSC).
if not defined BUILDS set "BUILDS=%REPO%\Builds"
set "DOBUILD=0"
set "ASSUMEYES=0"
for %%A in (%*) do (
    if /i "%%~A"=="build" (set "DOBUILD=1") else (
        if /i "%%~A"=="-y" (set "ASSUMEYES=1") else (
            if /i "%%~A"=="/y" (set "ASSUMEYES=1") else set "BUILDS=%%~fA"
        )
    )
)

rem --- Inno Setup's compiler
set "ISCC="
for %%P in ("%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" "%ProgramFiles%\Inno Setup 6\ISCC.exe" "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe") do (
    if not defined ISCC if exist "%%~P" set "ISCC=%%~P"
)
if not defined ISCC for /f "delims=" %%P in ('where ISCC.exe 2^>nul') do if not defined ISCC set "ISCC=%%P"
if not defined ISCC (
    echo Inno Setup 6 not found: install it from https://jrsoftware.org/isdl.php ^(default folder^) and run this again.
    goto :fail
)

rem --- Optional MSBuild step
if "%DOBUILD%"=="1" (
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if not exist "!VSWHERE!" ( echo vswhere not found: is Visual Studio installed? & goto :fail )
    set "MSBUILD="
    for /f "usebackq delims=" %%M in (`"!VSWHERE!" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do if not defined MSBUILD set "MSBUILD=%%M"
    if not defined MSBUILD ( echo MSBuild not found & goto :fail )
    for %%C in (Release Release_DLL) do (
        echo Building %%C ^| x64 ...
        "!MSBUILD!" "%REPO%\%SLN_FILE%" /m /nologo /v:minimal /p:Configuration=%%C /p:Platform=x64
        if errorlevel 1 ( echo The build %%C ^| x64 failed. & goto :fail )
    )
    for %%C in (Release Release_DLL) do (
        "!MSBUILD!" "%REPO%\%SLN_FILE%" /m /nologo /v:minimal /p:Configuration=%%C /p:Platform=Win32 >nul 2>&1
    )
)

rem --- Which x64 variants are built? Prompt if only one.
set "HAVE_X64_STATIC=0"
set "HAVE_X64_DYNAMIC=0"
if exist "%BUILDS%\x64\Release\%APP_EXE%"     set "HAVE_X64_STATIC=1"
if exist "%BUILDS%\x64\Release_DLL\%APP_EXE%" set "HAVE_X64_DYNAMIC=1"

if "%HAVE_X64_STATIC%%HAVE_X64_DYNAMIC%"=="00" (
    echo No x64 Release build found under %BUILDS%\x64\^(Release^|Release_DLL^).
    echo Build at least one Release configuration first ^(Visual Studio, or: installer\build_installer.bat build^).
    goto :fail
)
if "%HAVE_X64_STATIC%%HAVE_X64_DYNAMIC%" NEQ "11" (
    if "%HAVE_X64_STATIC%"=="1" ( set "ONLYONE=Release (static)" ) else ( set "ONLYONE=Release_DLL (dynamic)" )
    echo.
    echo Only one x64 variant was found: !ONLYONE!.
    echo The other Release variant is NOT built - the installer would offer only this one choice.
    if "%ASSUMEYES%"=="0" (
        set /p ANS=Continue packaging with only !ONLYONE! ? [y/N]
        if /i not "!ANS!"=="y" if /i not "!ANS!"=="yes" ( echo Aborted. & goto :fail )
    ) else (
        echo -y given: continuing without prompting.
    )
)

rem --- Optional Win32 pair
set "HAVE_X86_STATIC=0"
set "HAVE_X86_DYNAMIC=0"
if exist "%BUILDS%\Win32\Release\%APP_EXE%"     set "HAVE_X86_STATIC=1"
if exist "%BUILDS%\Win32\Release_DLL\%APP_EXE%" set "HAVE_X86_DYNAMIC=1"
set "HAVEX86=0"
if "%HAVE_X86_STATIC%%HAVE_X86_DYNAMIC%"=="11" set "HAVEX86=1"
if "%HAVEX86%"=="1" ( echo Including the 32 bit build. ) else ( echo No 32 bit build found - the setup is 64 bit only. )

rem --- Version + commit number from %VERSION_HEADER%
for /f "tokens=3" %%V in ('findstr /r /c:"#define APP_VERSION_MAJOR " "%REPO%\%VERSION_HEADER%"') do set "VMAJ=%%V"
for /f "tokens=3" %%V in ('findstr /r /c:"#define APP_VERSION_MINOR " "%REPO%\%VERSION_HEADER%"') do set "VMIN=%%V"
for /f "tokens=3" %%V in ('findstr /r /c:"#define APP_VERSION_PATCH " "%REPO%\%VERSION_HEADER%"') do set "VPAT=%%V"
for /f tokens^=2^ delims^=^" %%V in ('findstr /r /c:"#define APP_VERSION_PRERELEASE " "%REPO%\%VERSION_HEADER%"') do set "VPRE=%%V"
set "VERSION=%VMAJ%.%VMIN%.%VPAT%"
if not "%VPRE%"=="" set "VERSION=%VERSION%-%VPRE%"
set "COMMIT=0"
rem Per every CogaF project's CLAUDE.md: cmt = commit count - 1 (first commit of master = cmt.0).
for /f %%C in ('git -C "%REPO%" rev-list --count HEAD 2^>nul') do set /a COMMIT=%%C-1
if %COMMIT% LSS 0 set "COMMIT=0"
echo %APP_NAME% %VERSION% (cmt.%COMMIT%)

rem --- VC redist lookup. Priority:
rem   1) %COGAF_SHARED_REDIST%\                (env var wins)
rem   2) %REPO%\..\..\redist\                   (two levels up from the project - the shared default)
rem   3) %REPO%\installer\redist\               (project-local fallback)
rem Downloaded files are stored at the FIRST writable slot in that same order, so a project with
rem the shared setup never redownloads, and a one-off project that does not have the shared setup
rem keeps a local copy.
set "SHARED_REDIST="
if defined COGAF_SHARED_REDIST set "SHARED_REDIST=%COGAF_SHARED_REDIST%"
if not defined SHARED_REDIST (
    pushd "%REPO%\..\.." 2>nul
    if not errorlevel 1 ( set "SHARED_REDIST=!CD!\redist" & popd )
)
set "LOCAL_REDIST=%REPO%\installer\redist"
set "REDIST="
for %%R in ("%SHARED_REDIST%" "%LOCAL_REDIST%") do (
    if not defined REDIST if exist "%%~R\vc_redist.x64.exe" set "REDIST=%%~R"
)
rem Target dir for a (re)download: shared if the parent folder is writable, else local.
set "REDIST_TARGET=%LOCAL_REDIST%"
if defined SHARED_REDIST (
    if not exist "%SHARED_REDIST%" mkdir "%SHARED_REDIST%" >nul 2>&1
    rem Test writability by touching a probe file.
    > "%SHARED_REDIST%\.write_probe" echo ok 2>nul
    if exist "%SHARED_REDIST%\.write_probe" ( set "REDIST_TARGET=%SHARED_REDIST%" & del "%SHARED_REDIST%\.write_probe" >nul 2>&1 )
)
if not defined REDIST set "REDIST=%REDIST_TARGET%"
if not exist "%REDIST%" mkdir "%REDIST%"
if not exist "%REDIST%\vc_redist.x64.exe" (
    echo Downloading the Visual C++ runtime ^(x64^) into %REDIST% ...
    curl -L --fail -o "%REDIST%\vc_redist.x64.exe" "https://aka.ms/vs/17/release/vc_redist.x64.exe"
    if errorlevel 1 ( echo Download failed: put vc_redist.x64.exe into %REDIST% yourself. & del "%REDIST%\vc_redist.x64.exe" 2>nul & goto :fail )
)
for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "(Get-Item '%REDIST%\vc_redist.x64.exe').VersionInfo.FileVersion"`) do set "RT_x64=%%F"
set "RT_x86=%RT_x64%"
if "%HAVEX86%"=="1" (
    if not exist "%REDIST%\vc_redist.x86.exe" (
        echo Downloading the Visual C++ runtime ^(x86^) into %REDIST% ...
        curl -L --fail -o "%REDIST%\vc_redist.x86.exe" "https://aka.ms/vs/17/release/vc_redist.x86.exe"
        if errorlevel 1 ( echo Download failed: put vc_redist.x86.exe into %REDIST% yourself. & del "%REDIST%\vc_redist.x86.exe" 2>nul & goto :fail )
    )
    for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "(Get-Item '%REDIST%\vc_redist.x86.exe').VersionInfo.FileVersion"`) do set "RT_x86=%%F"
)
echo Visual C++ runtime carried: x64 %RT_x64% x86 %RT_x86%  (from %REDIST%)

rem --- The installer.
"%ISCC%" /Qp ^
    "/DAppVersion=%VERSION%" "/DAppVersionNum=%VMAJ%.%VMIN%.%VPAT%.%COMMIT%" ^
    "/DRepoDir=%REPO%" "/DBuildsDir=%BUILDS%" "/DRedistDir=%REDIST%" ^
    "/DRuntimeX64=%RT_x64%" "/DRuntimeX86=%RT_x86%" ^
    "/DHaveX64Static=%HAVE_X64_STATIC%" "/DHaveX64Dynamic=%HAVE_X64_DYNAMIC%" ^
    "/DHaveX86=%HAVEX86%" ^
    "%ISS_FILE%"
if errorlevel 1 ( echo Inno Setup failed. & goto :fail )
echo.
echo Done: %REPO%\installer\Output\%APP_NAME% %VERSION% setup.exe
echo.
echo   This single setup.exe carries every bundled variant:
echo     - OS architecture chosen automatically (x64 on 64-bit Windows, Win32 on 32-bit)
echo     - Static vs Dynamic (DLL) picked on the "Linking" wizard page
echo     - Admin install (Program Files, system-wide) OR per-user install
echo       (no UAC, %%LocalAppData%%\Programs\%APP_NAME%) picked on the
echo       first wizard screen. The per-user branch is the no-admin fallback -
echo       the folder picker still shows, so the operator can drop the files
echo       anywhere they can write (USB stick, Desktop, a share).
exit /b 0

:fail
echo.
pause
exit /b 1
