<#
    Copyright (C) 2026 Fation Coga
    SPDX-License-Identifier: LGPL-3.0-or-later

    NewProject.ps1 - creates a new wxWidgets application from the "skeleton" folder of this kit.

    Asks for (or takes as parameters):
      - the application name     e.g. "Reactor Control"  -> window title, exe name, data folder
      - the project identifier   e.g. "ReactorControl"   -> .sln/.vcxproj names (letters/digits only)
      - the destination folder
      - the builds to keep       x64 / Win32  x  Debug / Release  x  static / DLL wxWidgets
      - the About information    copyright holder, author, contact e-mail, description
      - the icon text            two words, green over red (the template's is TEMP over LATE)
      - git                      a local repository (branch, extra ignore patterns, first commit)
      - GitHub                   optionally a new repository on YOUR GitHub account: name, owner,
                                 private/public, description, homepage, topics, issues, wiki

    Then copies the skeleton, renames files, replaces names, generates new project GUIDs, removes
    the builds you did not choose from the .sln/.vcxproj, fills in AppInfo.h, draws the icon and
    (optionally) creates the git repository and publishes it.

    Nothing here is tied to the kit's author: the defaults come from the PC it runs on (git config,
    the GitHub CLI's login) and the repository is created and pushed with the account of whoever runs
    it. GitHub publishing uses the GitHub CLI (https://cli.github.com, "gh auth login" once) or, without
    it, the URL of an empty repository you created yourself.

    Double-click NewProject.bat, or run for example:
      powershell -ExecutionPolicy Bypass -File NewProject.ps1 -Name "Reactor Control" -Builds "x64" -Destination C:\dev\ReactorControl
      powershell -ExecutionPolicy Bypass -File NewProject.ps1 -Name "Reactor Control" -GitHub -Visibility private -Yes
#>
[CmdletBinding()]
param(
    [string]$Name,
    [string]$Identifier,
    [string]$Destination,
    # "all", "x64", "win32", "static", "dll", "debug", "release", or numbers from the list: "1,2,5"
    [string]$Builds,
    [string]$Holder,
    [string]$Author,
    # Contact e-mail shown in the About window; "-" for none
    [string]$Contact,
    [string]$Description,
    # Icon words "TOP/BOTTOM" (e.g. "RCT/CTRL"); "-" keeps the template's TEMP/LATE icon
    [string]$IconText,
    [switch]$NoGit,
    # Branch of the first commit
    [string]$Branch = 'main',
    # Extra .gitignore patterns, comma separated (e.g. "*.db, notes/"); "-" for none
    [string]$Ignore,
    # Publish on GitHub (with -Yes; asked otherwise)
    [switch]$GitHub,
    # "name" or "owner/name" (default: the identifier on your account)
    [string]$GitHubRepo,
    [ValidateSet('', 'private', 'public', 'internal')]
    [string]$Visibility,
    [string]$Homepage,
    # Comma separated, e.g. "wxwidgets, cpp, serial"
    [string]$Topics,
    [switch]$NoIssues,
    [switch]$NoWiki,
    # URL of an existing EMPTY repository to push to, instead of creating one with the GitHub CLI
    [string]$RemoteUrl,
    [switch]$Yes      # accept every default without asking
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$KitRoot  = $PSScriptRoot
$Skeleton = Join-Path $KitRoot 'skeleton'
$TemplateName   = 'Template App'
$TemplateId     = 'TemplateApp'
$TemplateHolder = 'Fation Coga'
$TemplateYear   = '2026'
$TemplateProjectGuid  = '3F6B2A41-8C7D-4E59-9A1B-5D2C7E8F9A10'
$TemplateSolutionGuid = '9E4D1C2B-7A6F-4B83-A5C9-1F2E3D4C5B6A'

$AllBuilds = @(
    @{ N = 1; Config = 'Debug';       Platform = 'x64';   Text = 'x64   Debug        wxWidgets static' },
    @{ N = 2; Config = 'Release';     Platform = 'x64';   Text = 'x64   Release      wxWidgets static' },
    @{ N = 3; Config = 'Debug_DLL';   Platform = 'x64';   Text = 'x64   Debug        wxWidgets DLL' },
    @{ N = 4; Config = 'Release_DLL'; Platform = 'x64';   Text = 'x64   Release      wxWidgets DLL' },
    @{ N = 5; Config = 'Debug';       Platform = 'Win32'; Text = 'Win32 Debug        wxWidgets static' },
    @{ N = 6; Config = 'Release';     Platform = 'Win32'; Text = 'Win32 Release      wxWidgets static' },
    @{ N = 7; Config = 'Debug_DLL';   Platform = 'Win32'; Text = 'Win32 Debug        wxWidgets DLL' },
    @{ N = 8; Config = 'Release_DLL'; Platform = 'Win32'; Text = 'Win32 Release      wxWidgets DLL' }
)

function Ask([string]$Question, [string]$Default) {
    if ($Yes) { return $Default }
    $shown = if ($Default) { "$Question [$Default]" } else { $Question }
    $answer = Read-Host $shown
    if ([string]::IsNullOrWhiteSpace($answer)) { return $Default }
    return $answer.Trim()
}

function AskYesNo([string]$Question, [bool]$Default) {
    while ($true) {
        $answer = Ask "$Question $(if ($Default) { '[Y/n]' } else { '[y/N]' })" ''
        if ([string]::IsNullOrWhiteSpace($answer)) { return $Default }
        $a = $answer.ToLowerInvariant()
        if ($a.StartsWith('y')) { return $true }
        if ($a.StartsWith('n')) { return $false }
        Write-Host '  Please answer y or n.'
    }
}

# "-" (or "none") clears a default.
function Clear-Dash([string]$Value) {
    if ($null -eq $Value) { return '' }
    $v = $Value.Trim()
    if ($v -eq '-' -or $v.ToLowerInvariant() -eq 'none') { return '' }
    return $v
}

# Runs a program, returns its exit code and output (stdout and stderr) without stopping the script.
function Invoke-Quiet([string]$Exe, [string[]]$Arguments) {
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $out = & $Exe @Arguments 2>&1 | ForEach-Object { "$_" }
        return @{ Code = $LASTEXITCODE; Output = (($out | Where-Object { $_ }) -join "`n").Trim() }
    } catch {
        return @{ Code = 1; Output = "$_" }
    } finally { $ErrorActionPreference = $old }
}

# Runs a program with its output on the console; returns true if it succeeded.
function Invoke-Live([string]$Exe, [string[]]$Arguments) {
    $old = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $Exe @Arguments 2>&1 | ForEach-Object { Write-Host "  $_" }
        return ($LASTEXITCODE -eq 0)
    } finally { $ErrorActionPreference = $old }
}

function Select-Builds([string]$Spec) {
    $spec = $Spec.ToLowerInvariant() -replace '\s', ''
    if ($spec -eq '' -or $spec -eq 'all') { return $AllBuilds }
    $chosen = @()
    foreach ($token in $spec.Split(',')) {
        switch -Regex ($token) {
            '^\d+$'   { $n = [int]$token; $chosen += $AllBuilds | Where-Object { $_.N -eq $n } }
            '^x64$'   { $chosen += $AllBuilds | Where-Object { $_.Platform -eq 'x64' } }
            '^(win32|x86|32)$' { $chosen += $AllBuilds | Where-Object { $_.Platform -eq 'Win32' } }
            '^static$' { $chosen += $AllBuilds | Where-Object { $_.Config -notlike '*_DLL' } }
            '^dll$'    { $chosen += $AllBuilds | Where-Object { $_.Config -like '*_DLL' } }
            '^debug$'  { $chosen += $AllBuilds | Where-Object { $_.Config -like 'Debug*' } }
            '^release$' { $chosen += $AllBuilds | Where-Object { $_.Config -like 'Release*' } }
            default   { throw "Unknown build selection '$token'." }
        }
    }
    $numbers = @($chosen | ForEach-Object { $_.N } | Sort-Object -Unique)
    if ($numbers.Count -eq 0) { throw 'No build selected.' }
    return @($AllBuilds | Where-Object { $numbers -contains $_.N })
}

# Two icon words from the application name: "Reactor Control" -> REACTOR / CONTROL,
# "SerialPM" -> SERIAL / PM, "Monitor" -> MONI / TOR.
function Get-IconWords([string]$AppName) {
    $parts = @($AppName -split '[^A-Za-z0-9]+' | Where-Object { $_ })
    if ($parts.Count -eq 1) { $parts = @([regex]::Split($parts[0], '(?<=[a-z0-9])(?=[A-Z])') | Where-Object { $_ }) }
    if ($parts.Count -eq 0) { return @('MY', 'APP') }
    if ($parts.Count -eq 1) {
        $w = $parts[0]
        if ($w.Length -le 3) { return @($w.ToUpperInvariant(), '') }
        $half = [int][Math]::Ceiling($w.Length / 2)
        return @($w.Substring(0, $half).ToUpperInvariant(), $w.Substring($half).ToUpperInvariant())
    }
    return @($parts[0].ToUpperInvariant(), (($parts | Select-Object -Skip 1) -join '').ToUpperInvariant())
}

# owner/name from a GitHub URL (https://github.com/owner/name(.git) or git@github.com:owner/name.git).
function Get-GitHubSlug([string]$Url) {
    if ($Url -match 'github\.com[:/]+([^/]+)/([^/]+?)(\.git)?/?$') { return "$($Matches[1])/$($Matches[2])" }
    return ''
}

# A C++ string literal body: backslashes and quotes escaped.
function ConvertTo-CString([string]$Text) {
    return $Text.Replace('\', '\\').Replace('"', '\"')
}

function Write-TextFile([string]$Path, [string]$Text, [bool]$Bom) {
    $encoding = New-Object System.Text.UTF8Encoding($Bom)
    [System.IO.File]::WriteAllText($Path, $Text, $encoding)
}

function Read-TextFile([string]$Path) {
    return [System.IO.File]::ReadAllText($Path)   # detects and drops a UTF-8 BOM
}

function Test-HasBom([string]$Path) {
    $bytes = [System.IO.File]::ReadAllBytes($Path)
    return ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF)
}

function Write-Section([string]$Title) {
    Write-Host ''
    Write-Host "--- $Title ---" -ForegroundColor Cyan
}

Write-Host ''
Write-Host '=== New wxWidgets application from the template ===' -ForegroundColor Cyan
Write-Host 'Enter accepts the [default]; "-" leaves an optional field empty.'
if (-not (Test-Path (Join-Path $Skeleton "$TemplateId.vcxproj"))) {
    throw "The skeleton folder was not found next to this script ($Skeleton)."
}

$git = Get-Command git -ErrorAction SilentlyContinue
$gh  = Get-Command gh -ErrorAction SilentlyContinue
function Get-GitConfig([string]$Key) {
    if (-not $git) { return '' }
    return (Invoke-Quiet 'git' @('config', '--get', $Key)).Output
}

# --- 1. Names -------------------------------------------------------------------------------------
Write-Section 'Project'
while ([string]::IsNullOrWhiteSpace($Name)) {
    $Name = Ask 'Application name (window title and exe name, e.g. Reactor Control)' 'My App'
}
$invalid = [System.IO.Path]::GetInvalidFileNameChars()
if ($Name.IndexOfAny($invalid) -ge 0) { throw "The name '$Name' contains characters not allowed in a file name." }

if ([string]::IsNullOrWhiteSpace($Identifier)) {
    $suggested = ($Name -replace '[^A-Za-z0-9]', '')
    if ($suggested -eq '' -or $suggested -match '^\d') { $suggested = 'App' + $suggested }
    $Identifier = Ask 'Project identifier (letters and digits: .sln/.vcxproj name)' $suggested
}
if ($Identifier -notmatch '^[A-Za-z][A-Za-z0-9_]*$') { throw "The identifier '$Identifier' must start with a letter and contain only letters, digits and _." }

if ([string]::IsNullOrWhiteSpace($Destination)) {
    $Destination = Ask 'Destination folder' (Join-Path (Split-Path $KitRoot -Parent) $Identifier)
}
$Destination = [System.IO.Path]::GetFullPath($Destination)
if ((Test-Path $Destination) -and (Get-ChildItem -Force $Destination | Select-Object -First 1)) {
    throw "The destination folder '$Destination' exists and is not empty."
}

# --- 2. Builds --------------------------------------------------------------------------------------
if ([string]::IsNullOrWhiteSpace($Builds)) {
    Write-Host ''
    Write-Host 'Builds (Visual Studio configurations) to include:'
    foreach ($b in $AllBuilds) { Write-Host ("  [{0}] {1}" -f $b.N, $b.Text) }
    Write-Host '  Enter numbers (1,2,5), groups (x64, win32, static, dll, debug, release, combined with commas) or all.'
    $Builds = Ask 'Builds' 'x64'
}
$selected = Select-Builds $Builds
$selectedNumbers = @($selected | ForEach-Object { $_.N })
$removed  = @($AllBuilds | Where-Object { $selectedNumbers -notcontains $_.N })

# --- 3. About information ---------------------------------------------------------------------------
Write-Section 'About window'
# Defaults from this PC (never the kit author's): git's user.name / user.email, else the Windows user.
$gitUser  = Get-GitConfig 'user.name'
$gitEmail = Get-GitConfig 'user.email'
$pcLogin  = if ($env:USERNAME) { $env:USERNAME } elseif ($env:USER) { $env:USER } else { 'me' }
$pcUser   = if ($gitUser) { $gitUser } else { $pcLogin }
if ([string]::IsNullOrWhiteSpace($Holder)) { $Holder = Ask 'Copyright holder' $pcUser }
if ([string]::IsNullOrWhiteSpace($Author)) { $Author = Ask 'Author' $Holder }
if (-not $PSBoundParameters.ContainsKey('Contact')) { $Contact = Ask 'Contact e-mail shown in About ("-" for none)' $gitEmail }
$Contact = Clear-Dash $Contact
if (-not $PSBoundParameters.ContainsKey('Description')) { $Description = Ask 'One-line description' "$Name - a wxWidgets application." }
$Description = Clear-Dash $Description
$year = (Get-Date).Year.ToString()

if (-not $PSBoundParameters.ContainsKey('IconText')) {
    $w = Get-IconWords $Name
    $IconText = Ask 'Icon text "TOP/BOTTOM" - green over red ("-" keeps TEMP/LATE)' ("{0}/{1}" -f $w[0], $w[1]).TrimEnd('/')
}
$IconText = Clear-Dash $IconText
$iconWords = @()
if ($IconText) { $iconWords = @($IconText.Split('/') | ForEach-Object { $_.Trim() }) }
if ($iconWords.Count -gt 2) { throw "The icon text '$IconText' has more than two words (TOP/BOTTOM)." }

# --- 4. git and GitHub ------------------------------------------------------------------------------
$makeRepo = $false
$publish = ''          # '' | 'gh' | 'url'
$repoOwner = ''; $repoName = ''; $repoSlug = ''
$extraIgnore = @()
$commitName = ''; $commitEmail = ''
if (-not $git) {
    if (-not $NoGit) { Write-Host ''; Write-Host 'git was not found on this PC: no repository will be created (https://git-scm.com).' -ForegroundColor Yellow }
}
elseif (-not $NoGit) {
    Write-Section 'git'
    $makeRepo = AskYesNo 'Create a git repository with a first commit?' $true
}
if ($makeRepo) {
    $Branch = Ask 'Branch name' $Branch
    if ($Branch -notmatch '^[A-Za-z0-9._/-]+$') { throw "The branch name '$Branch' is not valid." }

    # The commit identity: git's own if set, else asked and stored in THIS repository only.
    if (-not $gitUser)  { $commitName  = Ask 'Your name for the commits (git user.name, stored in this repository only)' $Author }
    if (-not $gitEmail) { $commitEmail = Ask 'Your e-mail for the commits (git user.email, stored in this repository only)' $(if ($Contact) { $Contact } else { "$pcLogin@users.noreply.github.com" }) }

    Write-Host 'Already ignored: build output (Builds\, obj\, .vs\, *.user), BuildCounter.txt, GeneratedBuildInfo.h, Doxygen docs\.'
    if (-not $PSBoundParameters.ContainsKey('Ignore')) { $Ignore = Ask 'Also ignore (comma separated patterns, e.g. *.db, notes/; "-" for none)' '-' }
    $extraIgnore = @((Clear-Dash $Ignore).Split(',') | ForEach-Object { $_.Trim() } | Where-Object { $_ })

    Write-Section 'GitHub'
    $ghLogin = ''
    if ($gh) {
        $status = Invoke-Quiet 'gh' @('api', 'user', '--jq', '.login')
        if ($status.Code -eq 0 -and $status.Output) { $ghLogin = $status.Output }
    }
    if ($RemoteUrl) { $publish = 'url' }
    elseif ($ghLogin) {
        Write-Host "GitHub CLI signed in as '$ghLogin' (this PC's account) - the repository is created there."
        if (AskYesNo 'Publish the project on GitHub?' ($GitHub.IsPresent -or -not $Yes)) { $publish = 'gh' }
    }
    else {
        if ($gh) { Write-Host 'The GitHub CLI is installed but not signed in ("gh auth login" to create repositories from here).' }
        else { Write-Host 'The GitHub CLI (https://cli.github.com) is not installed: a repository can still be pushed to an EMPTY one you created at https://github.com/new.' }
        $RemoteUrl = Clear-Dash (Ask 'URL of an empty repository to push to ("-" to keep it local)' '-')
        if ($RemoteUrl) { $publish = 'url' }
    }

    if ($publish -eq 'gh') {
        if ([string]::IsNullOrWhiteSpace($GitHubRepo)) { $GitHubRepo = Ask 'Repository ("name" or "owner/name" for an organisation)' "$ghLogin/$Identifier" }
        if ($GitHubRepo -notmatch '/') { $GitHubRepo = "$ghLogin/$GitHubRepo" }
        while ((Invoke-Quiet 'gh' @('repo', 'view', $GitHubRepo, '--json', 'name')).Code -eq 0) {
            if ($Yes) { throw "The repository $GitHubRepo already exists on GitHub." }
            $GitHubRepo = Ask "$GitHubRepo already exists - another name" ''
            if ($GitHubRepo -and $GitHubRepo -notmatch '/') { $GitHubRepo = "$ghLogin/$GitHubRepo" }
            if (-not $GitHubRepo) { throw 'Cancelled.' }
        }
        $repoSlug = $GitHubRepo
        if ([string]::IsNullOrWhiteSpace($Visibility)) {
            do { $Visibility = (Ask 'Visibility: private, public or internal (organisations)' 'private').ToLowerInvariant() }
            while (@('private', 'public', 'internal') -notcontains $Visibility)
        }
        if (-not $PSBoundParameters.ContainsKey('Homepage')) { $Homepage = Ask 'Homepage URL ("-" for none)' '-' }
        $Homepage = Clear-Dash $Homepage
        if (-not $PSBoundParameters.ContainsKey('Topics')) { $Topics = Ask 'Topics (comma separated, "-" for none)' 'wxwidgets, cpp, windows' }
        $Topics = Clear-Dash $Topics
        if (-not $NoIssues -and -not $Yes) { $NoIssues = -not (AskYesNo 'Enable issues (problem reports)?' $true) }
        if (-not $NoWiki -and -not $Yes) { $NoWiki = -not (AskYesNo 'Enable the wiki?' $false) } elseif ($Yes) { $NoWiki = $true }
    }
    elseif ($publish -eq 'url') {
        $repoSlug = Get-GitHubSlug $RemoteUrl
    }
    if ($repoSlug) { $repoOwner, $repoName = $repoSlug.Split('/', 2) }
}

# Links shown in the About window: the GitHub repository when there is one.
$website = ''; $issues = ''; $devPage = ''
if ($repoSlug) {
    $website = "https://github.com/$repoSlug"
    if (-not $NoIssues) { $issues = "$website/issues" }
    $devPage = "https://github.com/$repoOwner"
}

Write-Section 'Summary'
Write-Host "Application name : $Name"
Write-Host "Identifier       : $Identifier"
Write-Host "Destination      : $Destination"
Write-Host ("Builds           : " + (($selected | ForEach-Object { "$($_.Config)|$($_.Platform)" }) -join ', '))
Write-Host "Copyright        : Copyright (C) $year $Holder"
Write-Host "Author / contact : $Author$(if ($Contact) { " <$Contact>" })"
Write-Host "Description      : $Description"
Write-Host ("Icon             : " + $(if ($iconWords.Count) { $iconWords -join ' / ' } else { 'TEMP / LATE (template)' }))
if ($makeRepo) {
    Write-Host ("git              : branch $Branch" + $(if ($extraIgnore.Count) { ", also ignoring " + ($extraIgnore -join ', ') }))
    if ($publish -eq 'gh') {
        Write-Host ("GitHub           : $repoSlug ($Visibility" + $(if ($NoIssues) { ', no issues' }) + $(if ($NoWiki) { ', no wiki' }) + ')' + $(if ($Topics) { ", topics: $Topics" }))
    } elseif ($publish -eq 'url') {
        Write-Host "Push to          : $RemoteUrl"
    } else { Write-Host 'GitHub           : not published (local repository only)' }
} else { Write-Host 'git              : no repository' }
if (-not $Yes) {
    if (-not (AskYesNo 'Create the project?' $true)) { Write-Host 'Cancelled.'; exit 1 }
}

# --- 5. Copy ----------------------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$skip = @('Builds', 'obj', '.vs', 'docs')
Get-ChildItem -Force $Skeleton | Where-Object { $skip -notcontains $_.Name } | ForEach-Object {
    Copy-Item -Recurse -Force $_.FullName $Destination
}
foreach ($generated in @('BuildCounter.txt', 'include\GeneratedBuildInfo.h')) {
    $p = Join-Path $Destination $generated
    if (Test-Path $p) { Remove-Item -Force $p }
}
# The licence and dependency notes travel with the project.
foreach ($doc in @('COPYING', 'COPYING.LESSER', 'DEPENDENCIES.md')) {
    $src = Join-Path $KitRoot $doc
    if (Test-Path $src) { Copy-Item -Force $src $Destination }
}

# --- 6. Rename files --------------------------------------------------------------------------------
Get-ChildItem -Recurse -Force -File $Destination | Where-Object { $_.Name -like "*$TemplateId*" } | ForEach-Object {
    Rename-Item -Path $_.FullName -NewName ($_.Name -replace [regex]::Escape($TemplateId), $Identifier)
}

# --- 7. Replace names, holder, GUIDs ----------------------------------------------------------------
$newProjectGuid  = [guid]::NewGuid().ToString().ToUpperInvariant()
$newSolutionGuid = [guid]::NewGuid().ToString().ToUpperInvariant()
$textExtensions = @('.h', '.hpp', '.cpp', '.c', '.rc', '.sln', '.vcxproj', '.filters', '.xml', '.md', '.txt', '.gitignore', '.props', '.ps1')
Get-ChildItem -Recurse -Force -File $Destination | Where-Object { $textExtensions -contains $_.Extension.ToLowerInvariant() -or $_.Name -eq '.gitignore' -or $_.Name -eq 'Doxyfile' } | ForEach-Object {
    $bom = Test-HasBom $_.FullName
    $text = Read-TextFile $_.FullName
    $new = $text.Replace("Copyright (C) $TemplateYear $TemplateHolder", "Copyright (C) $year $Holder")
    $new = $new.Replace($TemplateName, $Name).Replace($TemplateId, $Identifier)
    $new = $new.Replace($TemplateProjectGuid, $newProjectGuid).Replace($TemplateProjectGuid.ToLowerInvariant(), $newProjectGuid)
    $new = $new.Replace($TemplateSolutionGuid, $newSolutionGuid)
    if ($new -ne $text) { Write-TextFile $_.FullName $new $bom }
}

# --- 8. AppInfo.h: author, contact, links, description ----------------------------------------------
$appInfoPath = Join-Path $Destination 'include\AppInfo.h'
$appInfo = Read-TextFile $appInfoPath
$oldDescription = ''
if ($appInfo -match '(?s)kDescription =\s*((?:"(?:[^"\\]|\\.)*"\s*)+);') {
    $oldDescription = (([regex]::Matches($Matches[1], '"((?:[^"\\]|\\.)*)"') | ForEach-Object { $_.Groups[1].Value }) -join '')
}
function Set-AppInfo([string]$Key, [string]$Value) {
    $pattern = '(?s)(inline constexpr const char\* ' + $Key + ' =)\s*(?:"(?:[^"\\]|\\.)*"\s*)+;'
    if ($script:appInfo -notmatch $pattern) { throw "AppInfo.h: $Key not found." }
    $literal = ConvertTo-CString $Value
    $script:appInfo = [regex]::Replace($script:appInfo, $pattern, { param($m) $m.Groups[1].Value + ' "' + $literal + '";' })
}
Set-AppInfo 'kDescription' $Description
Set-AppInfo 'kTagline' ''
Set-AppInfo 'kAuthor' $Author
Set-AppInfo 'kContact' $Contact
Set-AppInfo 'kWebsite' $website
Set-AppInfo 'kIssues' $issues
Set-AppInfo 'kDeveloperPage' $devPage
Set-AppInfo 'kLicensePath' $(if ($website) { "/blob/$Branch/LICENSE.md" } else { '' })
Write-TextFile $appInfoPath $appInfo $false
# The template's description and remark are no longer shown: drop their translations.
Get-ChildItem -Force (Join-Path $Destination 'languages') -Filter 'language-*.xml' | ForEach-Object {
    $lines = [System.IO.File]::ReadAllLines($_.FullName)
    $keep = @($lines | Where-Object { $_ -notmatch 'key="Why TEMP over LATE\?' -and -not ($oldDescription -and $oldDescription -ne $Description -and $_.Contains('key="' + $oldDescription.Replace('\"', '&quot;') + '"')) })
    if ($keep.Count -ne $lines.Count) { Write-TextFile $_.FullName (($keep -join "`n") + "`n") (Test-HasBom $_.FullName) }
}

# --- 9. Icon ----------------------------------------------------------------------------------------
if ($iconWords.Count -gt 0) {
    try {
        $top = $iconWords[0]
        $bottom = if ($iconWords.Count -gt 1) { $iconWords[1] } else { '' }
        & (Join-Path $Destination 'art\make-icon.ps1') -Top $top -Bottom $bottom -OutDir (Join-Path $Destination 'art') | Out-Null
        Write-Host "Icon drawn: $($iconWords -join ' / ')"
    } catch {
        Write-Host "The icon could not be drawn ($($_.Exception.Message)) - the template's is kept; run art\make-icon.ps1 later." -ForegroundColor Yellow
    }
}

# --- 10. Remove the builds not chosen ---------------------------------------------------------------
if ($removed.Count -gt 0) {
    $vcxproj = Join-Path $Destination "$Identifier.vcxproj"
    $doc = New-Object System.Xml.XmlDocument
    $doc.PreserveWhitespace = $true
    $doc.Load($vcxproj)
    $ns = New-Object System.Xml.XmlNamespaceManager($doc.NameTable)
    $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
    foreach ($b in $removed) {
        $pair = "$($b.Config)|$($b.Platform)"
        foreach ($node in @($doc.SelectNodes("//m:ProjectConfiguration[@Include='$pair']", $ns))) {
            [void]$node.ParentNode.RemoveChild($node)
        }
        $condition = "'`$(Configuration)|`$(Platform)'=='$pair'"
        foreach ($node in @($doc.SelectNodes('//*[@Condition]', $ns))) {
            if ($node.GetAttribute('Condition') -eq $condition) { [void]$node.ParentNode.RemoveChild($node) }
        }
    }
    $settings = New-Object System.Xml.XmlWriterSettings
    $settings.Encoding = New-Object System.Text.UTF8Encoding($true)
    $settings.NewLineChars = "`r`n"
    $writer = [System.Xml.XmlWriter]::Create($vcxproj, $settings)
    $doc.Save($writer)
    $writer.Close()

    $sln = Join-Path $Destination "$Identifier.sln"
    $slnBom = Test-HasBom $sln
    $lines = (Read-TextFile $sln) -split "`r?`n"
    $keep = foreach ($line in $lines) {
        $drop = $false
        foreach ($b in $removed) {
            $pattern = '(^|[\s.=])' + [regex]::Escape("$($b.Config)|$($b.Platform)") + '(\s|\.|$)'
            if ($line -match $pattern) { $drop = $true; break }
        }
        if (-not $drop) { $line }
    }
    Write-TextFile $sln (($keep -join "`r`n")) $slnBom
}

# --- 11. git ----------------------------------------------------------------------------------------
if ($makeRepo) {
    if ($extraIgnore.Count -gt 0) {
        $gi = Join-Path $Destination '.gitignore'
        $text = (Read-TextFile $gi).TrimEnd() + "`n# Added when the project was created`n" + ($extraIgnore -join "`n") + "`n"
        Write-TextFile $gi $text $false
    }
    Push-Location $Destination
    try {
        $ok = (Invoke-Quiet 'git' @('init', '-q')).Code -eq 0
        if ($ok) { [void](Invoke-Quiet 'git' @('symbolic-ref', 'HEAD', "refs/heads/$Branch")) }
        if ($ok -and $commitName)  { [void](Invoke-Quiet 'git' @('config', 'user.name', $commitName)) }
        if ($ok -and $commitEmail) { [void](Invoke-Quiet 'git' @('config', 'user.email', $commitEmail)) }
        if ($ok) { $ok = (Invoke-Quiet 'git' @('add', '-A')).Code -eq 0 }
        if ($ok) {
            $r = Invoke-Quiet 'git' @('commit', '-q', '-m', "Create $Name from the wxAppTemplate kit")
            $ok = $r.Code -eq 0
            if (-not $ok) { Write-Host $r.Output -ForegroundColor Yellow }
        }
        if ($ok) { Write-Host "git repository created (branch $Branch)." }
        else { Write-Host 'The git repository could not be created - see the message above.' -ForegroundColor Yellow }

        if ($ok -and $publish -eq 'gh') {
            Write-Host "Creating $repoSlug on GitHub..."
            $ghArgs = @('repo', 'create', $repoSlug, "--$Visibility", '--source', '.', '--remote', 'origin', '--push')
            if ($Description) { $ghArgs += @('--description', $Description.Replace('"', "'")) }
            if ($Homepage) { $ghArgs += @('--homepage', $Homepage) }
            if ($NoIssues) { $ghArgs += '--disable-issues' }
            if ($NoWiki) { $ghArgs += '--disable-wiki' }
            if (Invoke-Live 'gh' $ghArgs) {
                $topicList = @($Topics.Split(',') | ForEach-Object { (($_.Trim().ToLowerInvariant() -replace '\+', 'p') -replace '[^a-z0-9-]+', '-').Trim('-') } | Where-Object { $_ })
                if ($topicList.Count -gt 0) { [void](Invoke-Quiet 'gh' @('repo', 'edit', $repoSlug, '--add-topic', ($topicList -join ','))) }
                Write-Host "Published: $website" -ForegroundColor Green
            } else {
                Write-Host "GitHub did not create the repository. The project and its local repository are ready; later: gh repo create $repoSlug --$Visibility --source . --push" -ForegroundColor Yellow
            }
        }
        elseif ($ok -and $publish -eq 'url') {
            [void](Invoke-Quiet 'git' @('remote', 'add', 'origin', $RemoteUrl))
            Write-Host "Pushing to $RemoteUrl..."
            if (Invoke-Live 'git' @('push', '-u', 'origin', $Branch)) { Write-Host "Pushed: $RemoteUrl" -ForegroundColor Green }
            else { Write-Host "The push failed (does the repository exist, and is it empty?). Later: git push -u origin $Branch" -ForegroundColor Yellow }
        }
    } finally { Pop-Location }
}

Write-Host ''
Write-Host "Done: $Destination" -ForegroundColor Green
Write-Host "Open $Identifier.sln in Visual Studio - see DEPENDENCIES.md for WXWIN, VC_SQLITE and VC_WJWWOOD_SERIAL."
