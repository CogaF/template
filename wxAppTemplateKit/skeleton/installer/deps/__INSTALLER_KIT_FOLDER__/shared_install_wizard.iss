; Copyright (C) 2024-2026 Fation Coga.
;
; Common Inno Setup [Code] bits for previous-install detection, upgrade / uninstall / install-elsewhere
; prompting, and opposite-mode cleanup (so a per-user install doesn't leave an all-users menu entry
; dangling and vice versa). Each project's own .iss appends this to its [Code] section with:
;
;     #include "{#SharedKitDir}\shared_install_wizard.iss"
;
; The host .iss must declare the AppId it uses, as a preprocessor variable named "AppIdValue",
; before including this file. Example:
;
;     #define AppIdValue "{144D3490-B300-42A0-B521-C16DA00787DA}"
;     AppId={{#AppIdValue}
;     ...
;     [Code]
;     #include "{#SharedKitDir}\shared_install_wizard.iss"
;
; Everything here lives in the [Code] section of the including .iss - do NOT emit section markers.

const
  PrevInstallRegPath_HKLM = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\';
  PrevInstallRegPath_HKCU = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\';

var
  SharedWiz_FoundHive: Integer;     { 0 = none, 1 = HKLM (admin), 2 = HKCU (per-user), 3 = both }
  SharedWiz_FoundVersion: String;
  SharedWiz_FoundLocation: String;
  SharedWiz_FoundUninstallString: String;
  SharedWiz_FoundHiveForUninst: Integer;

function SharedWiz_ReadOne(RootKey: Integer; AppIdKey: String; var Ver, Loc, Uninst: String): Boolean;
var
  SubKey: String;
begin
  Result := False;
  SubKey := PrevInstallRegPath_HKLM + AppIdKey + '_is1';
  if RegQueryStringValue(RootKey, SubKey, 'DisplayVersion', Ver) then
  begin
    RegQueryStringValue(RootKey, SubKey, 'InstallLocation', Loc);
    RegQueryStringValue(RootKey, SubKey, 'UninstallString', Uninst);
    Result := True;
  end;
end;

procedure SharedWiz_DetectPrevious();
var
  Ver, Loc, Uninst: String;
  AppIdKey: String;
  HasHKLM, HasHKCU: Boolean;
  HKLM_Ver, HKLM_Loc, HKLM_Uninst: String;
  HKCU_Ver, HKCU_Loc, HKCU_Uninst: String;
begin
  AppIdKey := '{#AppIdValue}';
  HasHKLM := SharedWiz_ReadOne(HKLM, AppIdKey, HKLM_Ver, HKLM_Loc, HKLM_Uninst);
  HasHKCU := SharedWiz_ReadOne(HKCU, AppIdKey, HKCU_Ver, HKCU_Loc, HKCU_Uninst);

  if HasHKLM and HasHKCU then
    SharedWiz_FoundHive := 3
  else if HasHKLM then
    SharedWiz_FoundHive := 1
  else if HasHKCU then
    SharedWiz_FoundHive := 2
  else
    SharedWiz_FoundHive := 0;

  { Prefer the hive we are currently installing into, so "upgrade in place" naturally hits the
    matching one. If it's absent, fall back to the other. }
  if IsAdminInstallMode() and HasHKLM then
  begin
    SharedWiz_FoundVersion := HKLM_Ver;
    SharedWiz_FoundLocation := HKLM_Loc;
    SharedWiz_FoundUninstallString := HKLM_Uninst;
    SharedWiz_FoundHiveForUninst := 1;
  end
  else if (not IsAdminInstallMode()) and HasHKCU then
  begin
    SharedWiz_FoundVersion := HKCU_Ver;
    SharedWiz_FoundLocation := HKCU_Loc;
    SharedWiz_FoundUninstallString := HKCU_Uninst;
    SharedWiz_FoundHiveForUninst := 2;
  end
  else if HasHKLM then
  begin
    SharedWiz_FoundVersion := HKLM_Ver;
    SharedWiz_FoundLocation := HKLM_Loc;
    SharedWiz_FoundUninstallString := HKLM_Uninst;
    SharedWiz_FoundHiveForUninst := 1;
  end
  else if HasHKCU then
  begin
    SharedWiz_FoundVersion := HKCU_Ver;
    SharedWiz_FoundLocation := HKCU_Loc;
    SharedWiz_FoundUninstallString := HKCU_Uninst;
    SharedWiz_FoundHiveForUninst := 2;
  end;
end;

function SharedWiz_RunUninstaller(UninstStr: String): Boolean;
var
  ResultCode: Integer;
  Cmd, Params: String;
  QuotePos: Integer;
begin
  Result := False;
  if UninstStr = '' then Exit;

  { UninstallString is typically "C:\path\unins000.exe" /SILENT-friendly. Split on the closing quote
    if quoted, otherwise take the whole thing as the exe. We force /SILENT so the operator isn't
    yanked into a second wizard mid-install. }
  if (Length(UninstStr) > 0) and (UninstStr[1] = '"') then
  begin
    QuotePos := Pos('"', Copy(UninstStr, 2, Length(UninstStr))) + 1;
    Cmd := Copy(UninstStr, 2, QuotePos - 2);
    Params := Trim(Copy(UninstStr, QuotePos + 1, Length(UninstStr)));
  end
  else
  begin
    Cmd := UninstStr;
    Params := '';
  end;
  if Pos('/SILENT', UpperCase(Params)) = 0 then
  begin
    if Params <> '' then Params := Params + ' ';
    Params := Params + '/SILENT /NORESTART';
  end;
  Result := Exec(Cmd, Params, '', SW_SHOW, ewWaitUntilTerminated, ResultCode) and (ResultCode = 0);
end;

function SharedWiz_PromptAndAct(): Boolean;
var
  Msg, Target: String;
  Choice: Integer;
  ThisMode, OtherMode: String;
begin
  Result := True; { True = proceed with this installation, False = abort }

  if SharedWiz_FoundHive = 0 then Exit; { nothing previously installed }

  if IsAdminInstallMode() then
  begin
    ThisMode := 'all users (admin)';
    OtherMode := 'this user only';
  end
  else
  begin
    ThisMode := 'this user only';
    OtherMode := 'all users (admin)';
  end;

  if SharedWiz_FoundHive = 3 then
  begin
    { Both hives carry an install - rare but possible. Warn and offer to clean the opposite one. }
    Msg := 'This program is already installed in BOTH modes on this machine' + #13#10 +
           '  - for all users (admin install)' + #13#10 +
           '  - for this user only' + #13#10#13#10 +
           'Continuing will upgrade the ' + ThisMode + ' copy in place and' + #13#10 +
           'uninstall the ' + OtherMode + ' copy (to avoid two Start Menu entries).' + #13#10#13#10 +
           'Proceed?';
    if MsgBox(Msg, mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
      Exit;
    end;
    { We'll uninstall the opposite-mode install in the Post install step below. }
    Exit;
  end;

  if ((SharedWiz_FoundHive = 1) and IsAdminInstallMode()) or
     ((SharedWiz_FoundHive = 2) and (not IsAdminInstallMode())) then
  begin
    { Same-mode previous install -> standard upgrade path. Let Inno's own AppId machinery handle
      the overwrite, with a short info prompt so the operator knows a previous version is there. }
    Target := SharedWiz_FoundLocation;
    if Target = '' then Target := '(unknown folder)';
    Msg := 'Version ' + SharedWiz_FoundVersion + ' of this program is already installed for ' +
           ThisMode + ' in:' + #13#10 + '  ' + Target + #13#10#13#10 +
           'Continue with the upgrade?';
    if MsgBox(Msg, mbConfirmation, MB_YESNO) = IDNO then
    begin
      Result := False;
      Exit;
    end;
    Exit;
  end;

  { Opposite-mode previous install -> two Start Menu entries would collide. Offer uninstall + go,
    or cancel. }
  Target := SharedWiz_FoundLocation;
  if Target = '' then Target := '(unknown folder)';
  Msg := 'This program is already installed for ' + OtherMode + ' in:' + #13#10 +
         '  ' + Target + #13#10 +
         'Version: ' + SharedWiz_FoundVersion + #13#10#13#10 +
         'You are about to install it for ' + ThisMode + '. Two Start Menu entries from two' + #13#10 +
         'installs of the same program would conflict.' + #13#10#13#10 +
         'Yes = uninstall the ' + OtherMode + ' copy first, then install for ' + ThisMode + '.' + #13#10 +
         'No  = cancel this install (the ' + OtherMode + ' copy stays).';
  Choice := MsgBox(Msg, mbConfirmation, MB_YESNO);
  if Choice = IDNO then
  begin
    Result := False;
    Exit;
  end;

  if not SharedWiz_RunUninstaller(SharedWiz_FoundUninstallString) then
  begin
    MsgBox('Could not run the uninstaller for the ' + OtherMode + ' copy.' + #13#10 +
           'Please uninstall it manually (Settings > Apps) and run this installer again.',
           mbError, MB_OK);
    Result := False;
    Exit;
  end;
end;

{ --- Inno event functions. The host .iss must NOT define these itself - this file provides them.
      If the host needs extra logic in InitializeSetup or CurStepChanged, extend these. --- }

function InitializeSetup(): Boolean;
begin
  SharedWiz_DetectPrevious();
  Result := SharedWiz_PromptAndAct();
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  AppIdKey, Uninst: String;
  Ver, Loc: String;
  OtherRoot: Integer;
begin
  if CurStep <> ssPostInstall then Exit;
  if SharedWiz_FoundHive <> 3 then Exit;

  { Both-hive case: after installing into the current mode, scrub the opposite-mode copy so there
    is only one Start Menu entry. }
  AppIdKey := '{#AppIdValue}';
  if IsAdminInstallMode() then
    OtherRoot := HKCU
  else
    OtherRoot := HKLM;
  if SharedWiz_ReadOne(OtherRoot, AppIdKey, Ver, Loc, Uninst) then
    SharedWiz_RunUninstaller(Uninst);
end;
