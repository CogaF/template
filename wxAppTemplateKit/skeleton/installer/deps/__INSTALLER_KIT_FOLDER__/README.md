# win-installer-kit

Shared bits for every CogaF Windows desktop project's installer. One canonical
`installer_common.bat` and one cached copy of the Microsoft Visual C++ runtime, so no
project re-implements either and no machine re-downloads the redist.

## What's in the folder

- `installer_common.bat` — the shared Inno Setup driver. Every project's own
  `installer\build_installer.bat` is a thin ~50-line wrapper that sets per-project vars
  (`APP_NAME`, `APP_EXE`, `DATA_FOLDER`, `VERSION_HEADER`, `SLN_FILE`, `ISS_FILE`) and
  calls this script.
- `installer_common_gui.bat` — same contract, but shows a selector first (arch / linking /
  privilege / previous-install awareness) and forwards the choices to `installer_common.bat`.
  Called by each project's `build_installer_withGui.bat`.
- `gui/build_selector.ps1` — the WinForms selector shown by `installer_common_gui.bat`.
- `shared_setup.iss` — common Inno Setup `[Setup]` directives (PrivilegesRequired,
  PrivilegesRequiredOverridesAllowed, Compression, WizardStyle, MinVersion, …). Each project
  `#include`s it from its own `.iss`.
- `shared_install_wizard.iss` — common `[Code]` block that detects a previous install (per-user
  or all-users), offers upgrade / uninstall / install-elsewhere, and cleans up an opposite-mode
  copy so a Start Menu entry never duplicates.
- `vc_redist.x64.exe`, `vc_redist.x86.exe` — downloaded by the first `build_installer.bat`
  run that needs them. **Not** committed (see `.gitignore`); they live here because most
  projects can share one copy.
- `tools/sync_local_fallback.ps1` — refreshes every project's *local-fallback* copy of
  `installer_common.bat` from this canonical one. See *Workflow* below.

## Two installer flavours per project

Every project ships two thin wrappers in `installer\`:

- **`build_installer.bat`** → full-feature build: packs every Release variant found, prompts
  admin-vs-user at install time, previous-install-aware. The go-to for public releases.
- **`build_installer_withGui.bat`** → shows the selector. Pick one arch, one linking, one
  install mode, optionally turn off previous-install awareness — produces a small, specific
  single-mode setup (the "superlight for a known machine" case). Delegates to
  `installer_common_gui.bat` here.

Both produce the same `Output\<AppName>-vX.Y.Z-setupInstaller.exe`; the second is just smaller
and narrower in what it accepts at install time.

## Install (once per machine)

1. Clone this repo somewhere convenient. Suggested: next to your other project folders so
   the default `..\..\redist\` lookup lands here without any env var.

    ```
    git clone https://github.com/CogaF/win-installer-kit C:\CogaF\win-installer-kit
    ```

2. Point the env var at the clone (Windows → System Properties → Environment Variables).
   Can be user-scope — no admin needed.

    ```
    COGAF_SHARED_REDIST=C:\CogaF\win-installer-kit
    ```

    If you instead clone this repo at `<parent of your projects>\redist\`, you can skip the
    env var — the wrapper's `..\..\redist\` fallback lands there automatically.

3. From any project, run `installer\build_installer.bat`. The first time, it downloads the
   two `vc_redist.*.exe` into this folder (so they're cached for every other project too).
   Subsequent builds use the cached copies.

## Lookup order

Both the wrapper and the common script look in this order, for `installer_common.bat` AND
for `vc_redist.*.exe`:

1. `%COGAF_SHARED_REDIST%\` (env var set)
2. `..\..\redist\` (the project's parent-of-parent)
3. Each project's own `installer\redist\` (for the redist exes) or
   `installer\installer_common.bat` (for the common script — with a Yes/No prompt before
   use, so the operator knows it may be stale)

(1) and (2) are "shared"; (3) is the local-fallback, for a clone without the shared folder
yet. The first writable slot in that order also receives the downloaded redist on first
need — so a workspace with the shared folder never redownloads, and an isolated project
keeps a local copy.

## Update workflow

The idea is **one place to fix, every project picks it up on next build**.

1. Edit `installer_common.bat` here; commit + push. Every project immediately uses the new
   version on the next `build_installer.bat` run.
2. Run `tools\sync_local_fallback.ps1 <path-to-parent-folder-of-projects>` to refresh
   every project's local-fallback copy (so a fresh clone next week still gets today's
   version if the shared folder isn't set up). The script walks the parent folder, finds
   every `installer\installer_common.bat`, overwrites it with this canonical one, and
   commits per repo with a short "sync from win-installer-kit" message.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File tools\sync_local_fallback.ps1 C:\CogaF
```

## What the common script expects (contract)

Each project's thin wrapper sets:
- `APP_NAME`         — e.g. `Reactor Control`
- `APP_EXE`          — e.g. `Reactor Control.exe`
- `DATA_FOLDER`      — e.g. `Reactor Control data`
- `VERSION_HEADER`   — path from repo root to a header with `APP_VERSION_MAJOR / MINOR / PATCH / PRERELEASE` defines
- `SLN_FILE`         — e.g. `ReactorControl.sln`
- `ISS_FILE`         — absolute path to the project's Inno Setup `.iss`
- `REPO`             — absolute path of the project root (parent of `installer\`); usually the wrapper sets it with `cd /d "%~dp0.." & set "REPO=%CD%"`
- `BUILDS` (optional) — override for projects with a non-standard output layout (e.g. `%REPO%\build\winVSC`)

The common script then:
- Finds Inno Setup's `ISCC.exe`.
- Optionally rebuilds (`build` arg): MSBuild x64 Release + Release_DLL.
- Scans `Builds\x64\(Release|Release_DLL)\<APP_EXE>`; prompts if only one variant is present (`-y` skips).
- Includes the Win32 pair when both Win32 variants are built.
- Reads version + commit count from `VERSION_HEADER` (commit = `git rev-list --count HEAD - 1`).
- Ensures the redist exes, downloading into the highest-priority writable slot.
- Runs `ISCC` with the standard defines (`/DAppVersion`, `/DBuildsDir`, `/DRedistDir`,
  `/DRuntimeX64`, `/DRuntimeX86`, `/DHaveX64Static`, `/DHaveX64Dynamic`, `/DHaveX86`).

See any project's `installer\build_installer.bat` for a concrete example wrapper.

## Why this repo

Previously each project had its own copy of `build_installer.bat`. A fix to the shared
logic meant editing 4–8 files; drift was certain. Moving the heavy lifting here and keeping
the per-project file as a ~50-line wrapper means one edit = every project. The local
fallback is kept so a fresh clone still builds on a machine that hasn't set this repo up
yet — with an explicit "use local?" prompt so you know when you're on potentially-stale
logic.
