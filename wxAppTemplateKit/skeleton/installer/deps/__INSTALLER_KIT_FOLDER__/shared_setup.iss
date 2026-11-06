; Copyright (C) 2024-2026 Fation Coga.
;
; Common Inno Setup [Setup] directives every CogaF project agrees on. Each project's own .iss
; includes this with:
;
;     #ifndef SharedKitDir
;       #error Set SharedKitDir to the win-installer-kit folder before including this file
;     #endif
;     #include "{#SharedKitDir}\shared_setup.iss"
;
; One change here = every project's installer updates on next build. Project-specific directives
; (AppId, AppName, AppVersion, DefaultDirName, OutputBaseFilename, Files, Icons, Code) stay in the
; per-project .iss - only the generic, invariant-across-projects pieces live here.

; --- Privilege mode. Operator chooses "all users (admin)" or "me only" on the first wizard screen
; by default. The build-time GUI can restrict this to only-admin or only-per-user for a "superlight
; for a known machine" build by passing /DPrivReqAdmin or /DPrivReqUser to ISCC.
#ifdef PrivReqAdmin
PrivilegesRequired=admin
#else
#ifdef PrivReqUser
PrivilegesRequired=lowest
#else
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
#endif
#endif

; --- Compression, wizard style, closing behaviour. Same for every project.
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes
RestartApplications=no

; --- Architecture. Allow every architecture at run time; the installer itself decides based on
; which Files sections match Is64BitInstallMode. A per-project /DArchFilter can tighten this.
#ifndef ArchAllowed
  #define ArchAllowed "x86compatible x64compatible"
#endif
#ifndef ArchInstall64
  #define ArchInstall64 "x64compatible"
#endif
ArchitecturesAllowed={#ArchAllowed}
ArchitecturesInstallIn64BitMode={#ArchInstall64}

; --- Minimum supported Windows. Shared across all CogaF projects: Windows 7 SP1 and later.
MinVersion=6.1sp1
