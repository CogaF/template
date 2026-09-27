# Dependencies

Everything a project created from this kit needs to build and run on Windows, how to install it,
and where the project expects to find it.

| Dependency | Version | License | Used for | Found through |
|---|---|---|---|---|
| Visual Studio | 2022 (v143) or 2026 (v145) | Microsoft | compiler, MSBuild | the project uses the toolset installed as default |
| Windows SDK | 10 / 11 | Microsoft | Windows API | installed with Visual Studio |
| wxWidgets | 3.3.x (3.3.1 tested, 3.3.3 recommended) | wxWindows Library Licence 3.1 | GUI | environment variable `WXWIN` |
| SQLite | 3.x | Public domain | database (`Database` class) | environment variable `VC_SQLITE` |
| serial (CogaF fork of wjwwood/serial) | fork of 1.2.1 | MIT | serial ports (`SerialLink`) | environment variable `VC_WJWWOOD_SERIAL` |
| Git | any | GPL-2.0 | commit id shown in Version Info (optional) | on `PATH` |

All three libraries have licences compatible with the LGPL-3.0 of the template: an application built
from it may link them statically or dynamically.

---

## 1. Visual Studio

Install **Visual Studio 2022 or 2026** with the workload **"Desktop development with C++"**. For
32-bit builds (Win32) nothing else is needed; the x86 compilers are part of that workload.

The project uses whatever **platform toolset** is the default of the installed Visual Studio
(`$(DefaultPlatformToolset)`), C++20 and `/utf-8`, so the same project opens in 2022 and 2026 without
"retarget" prompts.

## 2. Environment variables

The project finds the libraries through three variables. Set them once (Start → "Edit the system
environment variables" → Environment Variables → New), or in a Command Prompt:

```bat
setx WXWIN             C:\libs\wxWidgets-3.3.3
setx VC_SQLITE         C:\libs\sqlite
setx VC_WJWWOOD_SERIAL C:\libs\serial
```

Restart Visual Studio afterwards (it reads the variables only at startup).

If your folders are laid out differently, override the properties `WxLibDir`, `SqliteIncludeDir`,
`SqliteLibDir`, `SerialIncludeDir` or `SerialLibDir` in a `Directory.Build.props` next to the `.sln`
instead of editing the project - see the "UserMacros" group at the top of the `.vcxproj`.

## 3. wxWidgets

1. Download the source from <https://www.wxwidgets.org/downloads/> (or `git clone --recurse-submodules
   https://github.com/wxWidgets/wxWidgets.git` and check out a 3.3 tag) and set `WXWIN` to that folder.
2. Open `%WXWIN%\build\msw\wx_vc17.sln` and build the configurations you need (Build → Batch Build):

| Template build | wxWidgets configuration | Platform | Library folder produced |
|---|---|---|---|
| Debug / Release | Debug / Release | x64 | `lib\vc_x64_lib` |
| Debug_DLL / Release_DLL | DLL Debug / DLL Release | x64 | `lib\vc_x64_dll` |
| Debug / Release | Debug / Release | Win32 | `lib\vc_lib` |
| Debug_DLL / Release_DLL | DLL Debug / DLL Release | Win32 | `lib\vc_dll` |

   The same can be done from a *Developer Command Prompt* in `%WXWIN%\build\msw`:
   `nmake /f makefile.vc BUILD=release TARGET_CPU=X64` (add `SHARED=1` for DLLs, `BUILD=debug`
   for debug, omit `TARGET_CPU` in an x86 prompt for Win32).

The project never lists the wxWidgets `.lib` files: `wx/msw/setup.h` (found through
`%WXWIN%\include\msvc`) links the right ones automatically for the configuration and platform. DLL
builds copy the wxWidgets DLLs next to the exe after each build.

**C runtime.** wxWidgets builds by default with the DLL C runtime (`/MD`, `/MDd`), and so does the
template - even the "static" builds link wxWidgets statically but use the DLL runtime. A computer
without Visual Studio then needs the **Microsoft Visual C++ Redistributable (x64 or x86)** installed.
For a single, fully self-contained exe, rebuild wxWidgets (and SQLite and serial) with the static
runtime (`nmake ... RUNTIME_LIBS=static`; for SQLite `build_all.bat static-crt`, which writes to
`Builds_StaticCRT\` - point `SqliteIncludeDir`/`SqliteLibDir` there) and set *C/C++ → Code
Generation → Runtime Library* to `/MT` (`/MTd` for Debug) in the project.

## 4. SQLite

Built with **SQLite3_builder** (<https://github.com/CogaF/SQLite3_builder>): a Visual Studio solution
that builds the SQLite amalgamation as a static library and a DLL, Debug and Release, x64 and x86.

```bat
git clone https://github.com/CogaF/SQLite3_builder.git C:\libs\SQLite3_builder
C:\libs\SQLite3_builder\build_all.bat
setx VC_SQLITE C:\libs\SQLite3_builder
```

Layout it produces (`x86` is the Win32 platform):

```
%VC_SQLITE%\Builds\include\sqlite3.h
%VC_SQLITE%\Builds\x64\Debug\lib\sqlite3.lib        %VC_SQLITE%\Builds\x64\Release\lib\sqlite3.lib
%VC_SQLITE%\Builds\x64\Debug\dll\sqlite3.dll + .lib  %VC_SQLITE%\Builds\x64\Release\dll\sqlite3.dll + .lib
%VC_SQLITE%\Builds\x86\...                            (the same for Win32)
```

The static configurations (Debug, Release) link `lib\sqlite3.lib`; the DLL configurations
(Debug_DLL, Release_DLL) link `dll\sqlite3.lib` and the post-build step copies `sqlite3.dll` next to
the exe.

## 5. serial (CogaF fork of wjwwood/serial)

<https://github.com/CogaF/serial> - a fork of <https://github.com/wjwwood/serial> (MIT) with:

- `getPort()` returns `std::wstring` on Windows (the original narrowed the internal wide string with a
  compiler warning about possible loss of characters);
- `open()` on a port that is already open reconfigures it with the current settings instead of
  throwing an exception;
- Visual Studio project updated (toolset v145, C++20).

Build `visual_studio\visual_studio.sln` (project `serial`, a static library) for Debug and Release,
x64 and Win32, and arrange the result as:

```
%VC_WJWWOOD_SERIAL%\include\serial\serial.h
%VC_WJWWOOD_SERIAL%\x64\Debug\serial.lib     %VC_WJWWOOD_SERIAL%\x64\Release\serial.lib
%VC_WJWWOOD_SERIAL%\Win32\Debug\serial.lib   %VC_WJWWOOD_SERIAL%\Win32\Release\serial.lib
```

Note: the fork's Win32 **Debug** configuration keeps Visual Studio's default output folder
(`Debug\` at the solution root) - move that `serial.lib` to `Win32\Debug\`, or set its output folder
to `$(SolutionDir)$(Platform)\$(Configuration)\` like the other configurations.

The fork currently builds on Windows only: its Unix backend still returns `std::string` from
`getPort()` while `serial.h` declares `std::wstring`. The template itself does not call `getPort()`.

## 6. At run time

| Build | Files next to the exe |
|---|---|
| static (Debug, Release) | the exe |
| DLL (Debug_DLL, Release_DLL) | the exe + the wxWidgets DLLs and `sqlite3.dll` (copied by the build) |
| all | the folder `<App name> data\` with the language files (copied by the build) |

Plus the Visual C++ Redistributable on computers without Visual Studio (see "C runtime" above).
Everything the application writes (settings, log, databases) goes into `<App name> data\`.
