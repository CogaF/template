# wxAppTemplate kit

A starting point for Windows desktop applications in C++20 with **wxWidgets**, **SQLite** and
**serial ports** - the reusable foundations of a real lab-control application (logging, settings,
translations, themes, versioning, serial transactions, database), stripped of everything
project-specific, plus a script that turns it into a new, named project with the builds you want.

Copyright (C) 2026 Fation Coga - [GNU LGPL v3.0 or later](LICENSE.md).

## Quick start

1. Install the dependencies and set `WXWIN`, `VC_SQLITE`, `VC_WJWWOOD_SERIAL` - see
   [DEPENDENCIES.md](DEPENDENCIES.md).
2. Double-click **`NewProject.bat`** and answer the questions:
   - application name (e.g. `Reactor Control`) - window title, exe name, data folder;
   - project identifier (e.g. `ReactorControl`) - `.sln` / `.vcxproj` name;
   - destination folder;
   - builds: any of the 8 below (`x64`, `win32`, `static`, `dll`, `debug`, `release`, `all`, or numbers);
   - copyright holder; optionally a git repository with a first commit.
3. Open the generated `.sln` and build. The exe is in `Builds\<Platform>\<Configuration>\`.

Without questions: `powershell -ExecutionPolicy Bypass -File NewProject.ps1 -Name "Reactor Control"
-Builds "x64,win32" -Destination C:\dev\ReactorControl -Yes`.

The `skeleton\` folder is itself a complete project (`TemplateApp.sln`) that builds as it is.

## Builds

| # | Platform | Configuration | wxWidgets | Output |
|---|---|---|---|---|
| 1 | x64 | Debug | static libs | `Builds\x64\Debug` |
| 2 | x64 | Release | static libs | `Builds\x64\Release` |
| 3 | x64 | Debug_DLL | DLLs | `Builds\x64\Debug_DLL` |
| 4 | x64 | Release_DLL | DLLs | `Builds\x64\Release_DLL` |
| 5-8 | Win32 | same four | | `Builds\Win32\...` |

"static" links wxWidgets into the exe; "DLL" uses the wxWidgets DLLs (copied next to the exe by the
build). Both use the DLL C runtime by default - see DEPENDENCIES.md, "C runtime", for a fully static exe.

Every build:
- increments a **build number** (`BuildCounter.txt`) and records the **git commit** and **build date**
  (`include\GeneratedBuildInfo.h`, both gitignored);
- writes `<App name>.exe` (always the same name - the debugger's target) **and a versioned copy**
  `<App name> v0.1.0-rc.1b12.exe` (`D` appended for Debug). Older versioned copies are kept;
- copies the language files into `<App name> data\` and, for DLL builds, the wxWidgets DLLs.

## What is inside

| File | What it gives you |
|---|---|
| `AppInfo.h` | the application name, data folder name, copyright - in one place |
| `Version.h`, `BuildInfo.*` | semantic version + history; build number, git commit, build date; window title `Name (v0.1.0-rc.1b12, 2026-09-26 14:03)` |
| `DataDir.*` | every file lives in `<exe folder>\<App name> data\` (created on demand) - one folder to back up |
| `Log.*` | thread-safe log to file + GUI; levels None...Trace; the GUI drains a bounded queue on a timer (never flooded, nothing lost at startup) |
| `AppSettings.*` | `key=value` settings, merge-safe and crash-safe writes (temp file + rename) |
| `I18n.*`, `UVT.h` | run-time translations from `language-<code>.xml`; missing or empty entries fall back to English; `UVT.h` holds every visible text |
| `Theme.*` | light/dark at startup; live "Use now" switch that also themes windows opened later |
| `SerialConfig.*` | port settings, text persistence, port discovery, timeout floor (a 0/0 timeout blocks forever on Windows) |
| `SerialLink.*` | thread-safe request/reply on one port: timed lock, double input flush for RS-485, read until the frame is complete, abortable |
| `SerialWorker.*` | background transaction queue; results delivered on the GUI thread |
| `Database.*` | SQLite RAII wrapper: statements, transactions, WAL mode |
| `ThreadUtils.h` | `AliveGuard`, `CallLater` (delayed GUI call that is skipped if its window is gone) |
| `SerialConfigDialog.*` | port / baud / format / timeouts dialog |
| `MainWindow.*`, `App.*` | skeleton window: menus, open/close port, send hex and see the reply, live log, event database, theme / language / log level with Restart now · Use now · Save for next start · Cancel, Version Info, About |

## Extending it

- **A device protocol:** build request bytes, then submit a `SerialWorker::Job` with a `FrameComplete`
  that tells from the first bytes how long the reply is (length byte, end marker, ...). See the
  comment in `MainWindow::onSend()`.
- **A new visible text:** add it to `UVT.h`, show it with `tr(UVT::MY_TEXT)`, add the translation to
  each `languages\language-<code>.xml`.
- **A new language:** copy `language-it.xml` to `language-<code>.xml`, set `language` and `name` in its
  first element, translate the values. No rebuild needed.
- **A release:** update `Version.h` (numbers + history entry), commit, tag, build Release, hand out the
  versioned exe.

## Keeping translated text intact

Keep user-visible text in `wxString`. `wxString::ToStdString()` converts through the C locale and
returns an **empty** string for non-Latin scripts (Arabic, Russian, Chinese, ...). When a
`std::string` is unavoidable (files, SQLite), use `utf8_string()` and read back with
`wxString::FromUTF8()`.
