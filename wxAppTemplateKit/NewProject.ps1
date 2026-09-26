<#
    Copyright (C) 2026 Fation Coga
    SPDX-License-Identifier: LGPL-3.0-or-later

    NewProject.ps1 - creates a new wxWidgets application from the "skeleton" folder of this kit.

    Asks for (or takes as parameters):
      - the application name     e.g. "Reactor Control"  -> window title, exe name, data folder
      - the project identifier   e.g. "ReactorControl"   -> .sln/.vcxproj names (letters/digits only)
      - the destination folder
      - the builds to keep       x64 / Win32  x  Debug / Release  x  static / DLL wxWidgets
      - the copyright holder

    Then copies the skeleton, renames files, replaces names, generates new project GUIDs, removes
    the builds you did not choose from the .sln/.vcxproj, and (optionally) creates a git repository.

    Double-click NewProject.bat, or run for example:
      powershell -ExecutionPolicy Bypass -File NewProject.ps1 -Name "Reactor Control" -Builds "x64" -Destination C:\dev\ReactorControl
#>
[CmdletBinding()]
param(
    [string]$Name,
    [string]$Identifier,
    [string]$Destination,
    # "all", "x64", "win32", "static", "dll", "debug", "release", or numbers from the list: "1,2,5"
    [string]$Builds,
    [string]$Holder,
    [switch]$NoGit,
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

Write-Host ''
Write-Host '=== New wxWidgets application from the template ===' -ForegroundColor Cyan
Write-Host ''
if (-not (Test-Path (Join-Path $Skeleton "$TemplateId.vcxproj"))) {
    throw "The skeleton folder was not found next to this script ($Skeleton)."
}

# --- 1. Names -------------------------------------------------------------------------------------
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

if ([string]::IsNullOrWhiteSpace($Holder)) {
    $Holder = Ask 'Copyright holder' $TemplateHolder
}
$year = (Get-Date).Year.ToString()

Write-Host ''
Write-Host "Application name : $Name"
Write-Host "Identifier       : $Identifier"
Write-Host "Destination      : $Destination"
Write-Host ("Builds           : " + (($selected | ForEach-Object { "$($_.Config)|$($_.Platform)" }) -join ', '))
Write-Host "Copyright        : Copyright (C) $year $Holder"
if (-not $Yes) {
    $go = Read-Host 'Create the project? [Y/n]'
    if ($go -and $go.Trim().ToLowerInvariant().StartsWith('n')) { Write-Host 'Cancelled.'; exit 1 }
}

# --- 3. Copy ----------------------------------------------------------------------------------------
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$skip = @('Builds', 'obj', '.vs')
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

# --- 4. Rename files --------------------------------------------------------------------------------
Get-ChildItem -Recurse -Force -File $Destination | Where-Object { $_.Name -like "*$TemplateId*" } | ForEach-Object {
    Rename-Item -Path $_.FullName -NewName ($_.Name -replace [regex]::Escape($TemplateId), $Identifier)
}

# --- 5. Replace names, holder, GUIDs ----------------------------------------------------------------
$newProjectGuid  = [guid]::NewGuid().ToString().ToUpperInvariant()
$newSolutionGuid = [guid]::NewGuid().ToString().ToUpperInvariant()
$textExtensions = @('.h', '.hpp', '.cpp', '.c', '.rc', '.sln', '.vcxproj', '.filters', '.xml', '.md', '.txt', '.gitignore', '.props')
Get-ChildItem -Recurse -Force -File $Destination | Where-Object { $textExtensions -contains $_.Extension.ToLowerInvariant() -or $_.Name -eq '.gitignore' } | ForEach-Object {
    $bom = Test-HasBom $_.FullName
    $text = Read-TextFile $_.FullName
    $new = $text.Replace("Copyright (C) $TemplateYear $TemplateHolder", "Copyright (C) $year $Holder")
    $new = $new.Replace($TemplateName, $Name).Replace($TemplateId, $Identifier)
    $new = $new.Replace($TemplateProjectGuid, $newProjectGuid).Replace($TemplateProjectGuid.ToLowerInvariant(), $newProjectGuid)
    $new = $new.Replace($TemplateSolutionGuid, $newSolutionGuid)
    if ($new -ne $text) { Write-TextFile $_.FullName $new $bom }
}

# --- 6. Remove the builds not chosen ----------------------------------------------------------------
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

# --- 7. git -----------------------------------------------------------------------------------------
$git = Get-Command git -ErrorAction SilentlyContinue
if ($git -and -not $NoGit) {
    $makeRepo = Ask 'Create a git repository with a first commit? [Y/n]' 'Y'
    if (-not $makeRepo.ToLowerInvariant().StartsWith('n')) {
        Push-Location $Destination
        try {
            git init -q
            git add -A
            git commit -q -m "Create $Name from the wxAppTemplate kit"
            Write-Host 'git repository created.'
        } finally { Pop-Location }
    }
}

Write-Host ''
Write-Host "Done: $Destination" -ForegroundColor Green
Write-Host "Open $Identifier.sln in Visual Studio - see DEPENDENCIES.md for WXWIN, VC_SQLITE and VC_WJWWOOD_SERIAL."
