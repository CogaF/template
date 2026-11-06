; Copyright (C) 2024-2026 Fation Coga. All rights reserved.
; This file is part of Template App - see LICENSE.md.
;
; Inno Setup script of the Template App installer: ONE setup.exe. It installs the x64 build on 64 bit Windows, the
; Win32 build on 32 bit Windows (only if one was bundled), and asks whether the static or the dynamic (DLL) build is
; wanted - skipping that page if only one of the two variants was packaged. Built by build_installer.bat (which passes
; the defines below):
;     ISCC /DAppVersion=0.10.0-rc.99 /DAppVersionNum=0.10.0.498 /DBuildsDir=...\Builds /DRepoDir=... /DRedistDir=... TemplateApp.iss
; Builds used: Builds\<x64|Win32>\<Release|Release_DLL>\ (the Visual Studio output folders).

#define AppName "Template App"
#define AppExe "Template App.exe"
#define DataFolder "Template App data"
#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef AppVersionNum
  #define AppVersionNum "0.0.0.0"
#endif
#ifndef RepoDir
  #define RepoDir ".."
#endif
#ifndef BuildsDir
  #define BuildsDir RepoDir + "\Builds"
#endif
#ifndef RedistDir
  #define RedistDir RepoDir + "\installer\redist"
#endif
#ifndef RuntimeX64
  #define RuntimeX64 "14.0.0"
#endif
#ifndef RuntimeX86
  #define RuntimeX86 "14.0.0"
#endif
#ifndef HaveX64Static
  #define HaveX64Static "1"
#endif
#ifndef HaveX64Dynamic
  #define HaveX64Dynamic "1"
#endif
#ifndef HaveX86
  #define HaveX86 "0"
#endif

[Setup]
; Never change AppId: it is how a newer installer finds and replaces an installed version.
AppId={{FD549C3D-5D83-4651-821D-D00C78674D81}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Fation Coga
AppContact=coga.fation@gmail.com
AppCopyright=Copyright (C) 2024-2026 Fation Coga
VersionInfoVersion={#AppVersionNum}
DefaultDirName={autopf}\{#AppName}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}
LicenseFile={#RepoDir}\LICENSE.md
SetupIconFile={#RepoDir}\art\app.ico
OutputDir={#RepoDir}\installer\Output
OutputBaseFilename=Template App {#AppVersion} setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; 64 bit where Windows can run it (also ARM64 Windows 11, emulated), else 32 bit - if a Win32 build is bundled.
ArchitecturesAllowed=x86compatible x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; Dual-mode install per user request (no admin / no fallback problem):
;   - "lowest" is the default so the installer runs without an admin prompt;
;   - PrivilegesRequiredOverridesAllowed=dialog shows the operator a chooser on launch:
;       * "Install for all users (admin, Program Files)" - requires UAC elevation
;       * "Install for me only (no admin, %LocalAppData%\Programs)" - no elevation
;   - PrivilegesRequiredOverridesAllowed=commandline also lets a batch pass /ALLUSERS or /CURRENTUSER
;     non-interactively (used by build_installer.bat for CI / scripted installs).
; Non-admin mode writes into a per-user folder that the operator already owns, so no permission
; stunts are needed; the installer still shows a folder-picker, so "install anywhere I can write"
; is the natural fallback when admin is refused.
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
MinVersion=6.1sp1
CloseApplications=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"

[CustomMessages]
english.LinkTitle=Static or dynamic version
english.LinkDescription=Both work the same; they differ only in how the program is made of files.
english.LinkExplain=The program uses libraries: wxWidgets (the windows) and any project-specific libraries.%n%nStatic: the libraries are built into the one program file. Nothing else to keep next to it - the simplest and safest choice.%n%nDynamic: the program plus its library files (DLL) beside it. Smaller program file; the DLLs must stay with it.%n%nEither way the Microsoft Visual C++ runtime is required: it is installed if this PC does not have it.
english.LinkStatic=Static - one program file (recommended)
english.LinkDynamic=Dynamic - program and library files (DLL)
english.RuntimeStatus=Installing the Microsoft Visual C++ runtime...
english.DesktopIcon=Desktop shortcut
italian.LinkTitle=Versione statica o dinamica
italian.LinkDescription=Funzionano allo stesso modo; differiscono solo per i file di cui e' fatto il programma.
italian.LinkExplain=Il programma usa alcune librerie: wxWidgets (le finestre) ed eventuali librerie specifiche del progetto.%n%nStatica: le librerie sono incluse nell'unico file del programma. Nient'altro da tenere accanto - la scelta piu' semplice e sicura.%n%nDinamica: il programma piu' i suoi file di libreria (DLL) accanto. File del programma piu' piccolo; le DLL devono restare con esso.%n%nIn entrambi i casi serve il runtime Microsoft Visual C++: viene installato se questo PC non lo ha.
italian.LinkStatic=Statica - un solo file del programma (consigliata)
italian.LinkDynamic=Dinamica - programma e file di libreria (DLL)
italian.RuntimeStatus=Installazione del runtime Microsoft Visual C++...
italian.DesktopIcon=Collegamento sul desktop

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopIcon}"; Flags: unchecked

[Dirs]
; The data folder (Settings.db, log, etc.) must be writable by every user of the PC - under Program Files it would not
; be. Kept on uninstall: the settings survive a reinstall. The app itself (pUtl::getDataDirectoryPath) will probe-write
; this folder on first launch and fall back to %APPDATA%\Template App\data if the probe fails (e.g. the user manually
; copied the exe into Program Files without this installer). Settings > "Change data folder..." lets the operator pick
; any other folder - the pointer file is at %APPDATA%\Template App\data_folder.txt (per-user, never admin).
; Admin-mode install into Program Files: add users-modify ACL so standard users can write here.
; Per-user install (non-admin): the whole {app} folder is already owned by the installing user, so
; the Permissions directive is harmless but not strictly needed. uninsneveruninstall keeps the
; data folder across reinstalls either way.
Name: "{app}\{#DataFolder}"; Permissions: users-modify; Flags: uninsneveruninstall

[InstallDelete]
; From dynamic to static: the DLLs of before go (nothing else of the folder).
Type: files; Name: "{app}\*.dll"; Check: IsStatic

[Files]
; 64 bit Windows - whichever variants were packaged (the .bat computes HaveX64Static / HaveX64Dynamic).
#if HaveX64Static == "1"
Source: "{#BuildsDir}\x64\Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode and IsStatic
#endif
#if HaveX64Dynamic == "1"
Source: "{#BuildsDir}\x64\Release_DLL\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode and IsDynamic
Source: "{#BuildsDir}\x64\Release_DLL\*.dll"; DestDir: "{app}"; Flags: ignoreversion; Check: Is64BitInstallMode and IsDynamic
#endif
Source: "{#RedistDir}\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall; Check: Is64BitInstallMode and RuntimeNeeded
; 32 bit Windows (only if a Win32 build was bundled - skipped otherwise)
#if HaveX86 == "1"
Source: "{#BuildsDir}\Win32\Release\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist; Check: (not Is64BitInstallMode) and IsStatic
Source: "{#BuildsDir}\Win32\Release_DLL\{#AppExe}"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist; Check: (not Is64BitInstallMode) and IsDynamic
Source: "{#BuildsDir}\Win32\Release_DLL\*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist; Check: (not Is64BitInstallMode) and IsDynamic
Source: "{#RedistDir}\vc_redist.x86.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall skipifsourcedoesntexist; Check: (not Is64BitInstallMode) and RuntimeNeeded
#endif
; Both: translations (into the data folder), licence / docs. The manuals folder is left optional - the project does not
; currently ship PDFs; skipifsourcedoesntexist keeps the installer buildable if those files appear later.
Source: "{#RepoDir}\languages\language-*.xml"; DestDir: "{app}\{#DataFolder}"; Flags: ignoreversion
Source: "{#RepoDir}\LICENSE.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoDir}\LICENSING.md"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#RepoDir}\CHANGELOG.md"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "{#RepoDir}\docs\*.pdf"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Run]
; VC runtime install is system-wide and needs admin. Skip it in per-user install mode - the
; runtime is carried into installer/redist/ anyway, and a non-admin user who needs it can run
; the vc_redist.*.exe themselves (UAC will prompt). IsAdminInstallMode is Inno Setup's
; "started with admin override", not just "user is admin".
Filename: "{tmp}\vc_redist.x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "{cm:RuntimeStatus}"; Flags: waituntilterminated; Check: Is64BitInstallMode and IsAdminInstallMode and RuntimeNeeded
#if HaveX86 == "1"
Filename: "{tmp}\vc_redist.x86.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "{cm:RuntimeStatus}"; Flags: waituntilterminated; Check: (not Is64BitInstallMode) and IsAdminInstallMode and RuntimeNeeded
#endif
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#AppName}}"; Flags: nowait postinstall skipifsilent

[Code]
var
  LinkPage: TInputOptionWizardPage;
  LinkPageAvailable: Boolean;

procedure InitializeWizard;
begin
  { Only offer the static/dynamic page if BOTH variants were packaged. If only one is present, we
    pick that one and skip the chooser. }
  LinkPageAvailable := ({#HaveX64Static} = 1) and ({#HaveX64Dynamic} = 1);
  if LinkPageAvailable then begin
    LinkPage := CreateInputOptionPage(wpSelectDir, CustomMessage('LinkTitle'), CustomMessage('LinkDescription'),
      CustomMessage('LinkExplain'), True, False);
    LinkPage.Add(CustomMessage('LinkStatic'));
    LinkPage.Add(CustomMessage('LinkDynamic'));
    if GetPreviousData('Link', 'static') = 'dynamic' then LinkPage.SelectedValueIndex := 1
    else LinkPage.SelectedValueIndex := 0;
  end;
end;

procedure RegisterPreviousData(PreviousDataKey: Integer);
begin
  if LinkPageAvailable then begin
    if LinkPage.SelectedValueIndex = 1 then SetPreviousData(PreviousDataKey, 'Link', 'dynamic')
    else SetPreviousData(PreviousDataKey, 'Link', 'static');
  end;
end;

function IsStatic: Boolean;
begin
  if LinkPageAvailable then Result := (LinkPage.SelectedValueIndex = 0)
  else Result := ({#HaveX64Static} = 1);
end;

function IsDynamic: Boolean;
begin
  Result := not IsStatic;
end;

{ The Visual C++ 2015-2026 runtime of the architecture: missing, or older than the one carried. }
function RuntimeNeeded: Boolean;
var
  Key, Carried: String;
  Installed, Major, Minor, Bld: Cardinal;
  Have, Need: Int64;
begin
  if Is64BitInstallMode then begin Key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64'; Carried := '{#RuntimeX64}'; end
  else begin Key := 'SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x86'; Carried := '{#RuntimeX86}'; end;
  Result := True;
  if not RegQueryDWordValue(HKLM, Key, 'Installed', Installed) or (Installed <> 1) then Exit;
  if not (RegQueryDWordValue(HKLM, Key, 'Major', Major) and RegQueryDWordValue(HKLM, Key, 'Minor', Minor)
    and RegQueryDWordValue(HKLM, Key, 'Bld', Bld)) then Exit;
  Have := (Int64(Major) * 1000 + Minor) * 100000 + Bld;
  Need := StrToInt64Def(Copy(Carried, 1, Pos('.', Carried) - 1), 14) * 1000 * 100000;
  Delete(Carried, 1, Pos('.', Carried));
  Need := Need + StrToInt64Def(Copy(Carried, 1, Pos('.', Carried) - 1), 0) * 100000;
  Delete(Carried, 1, Pos('.', Carried));
  if Pos('.', Carried) > 0 then Carried := Copy(Carried, 1, Pos('.', Carried) - 1);
  Need := Need + StrToInt64Def(Carried, 0);
  Result := Have < Need;
end;
