# Copyright (C) 2024-2026 Fation Coga.
#
# WinForms selector shown by installer_common_gui.bat. Writes key=value lines to -OutFile and
# exits 0 on OK, 1 on Cancel. The bat parses the file, sets env vars, delegates to the core
# installer_common.bat.

[CmdletBinding()]
param(
    [string]$AppName = 'CogaF application',
    [string]$HasX64Static  = '',
    [string]$HasX64Dynamic = '',
    [string]$HasX86Static  = '',
    [string]$HasX86Dynamic = '',
    [Parameter(Mandatory = $true)][string]$OutFile
)

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

# Dark palette - matches the house rule for small tools.
$bg = [System.Drawing.Color]::FromArgb(40, 40, 40)
$panel = [System.Drawing.Color]::FromArgb(50, 50, 50)
$fg = [System.Drawing.Color]::FromArgb(230, 230, 230)
$fgDim = [System.Drawing.Color]::FromArgb(140, 140, 140)
$accent = [System.Drawing.Color]::FromArgb(110, 180, 230)

$form = New-Object System.Windows.Forms.Form
$form.Text = "Installer options - $AppName"
$form.Size = New-Object System.Drawing.Size(540, 540)
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.MinimizeBox = $false
$form.BackColor = $bg
$form.ForeColor = $fg
$form.Font = New-Object System.Drawing.Font('Segoe UI', 9)

function New-Label($text, $x, $y, $w, $h) {
    $l = New-Object System.Windows.Forms.Label
    $l.Text = $text; $l.Location = New-Object System.Drawing.Point($x, $y)
    $l.Size = New-Object System.Drawing.Size($w, $h); $l.ForeColor = $fg
    return $l
}

function New-Group($title, $x, $y, $w, $h) {
    $g = New-Object System.Windows.Forms.GroupBox
    $g.Text = $title; $g.Location = New-Object System.Drawing.Point($x, $y)
    $g.Size = New-Object System.Drawing.Size($w, $h); $g.ForeColor = $accent
    return $g
}

function New-Check($text, $x, $y, $w, $checked, $enabled = $true) {
    $c = New-Object System.Windows.Forms.CheckBox
    $c.Text = $text; $c.Location = New-Object System.Drawing.Point($x, $y)
    $c.Size = New-Object System.Drawing.Size($w, 22); $c.Checked = $checked
    $c.Enabled = $enabled
    if ($enabled) { $c.ForeColor = $fg } else { $c.ForeColor = $fgDim }
    return $c
}

function New-Radio($text, $x, $y, $w, $checked) {
    $r = New-Object System.Windows.Forms.RadioButton
    $r.Text = $text; $r.Location = New-Object System.Drawing.Point($x, $y)
    $r.Size = New-Object System.Drawing.Size($w, 22); $r.Checked = $checked; $r.ForeColor = $fg
    return $r
}

$headerLbl = New-Label "Pick which builds to pack and who the installer is for." 15 10 500 20
$form.Controls.Add($headerLbl)

# ---- Variants group ---------------------------------------------------------
$varGrp = New-Group 'Builds to include' 15 40 495 170
$form.Controls.Add($varGrp)

$hx64s = ($HasX64Static  -eq '1')
$hx64d = ($HasX64Dynamic -eq '1')
$hx86s = ($HasX86Static  -eq '1')
$hx86d = ($HasX86Dynamic -eq '1')

$cbX64Static  = New-Check ('x64 static'  + $(if ($hx64s) { '' } else { '   (not built)' })) 20 30 200 $hx64s $hx64s
$cbX64Dynamic = New-Check ('x64 dynamic' + $(if ($hx64d) { '' } else { '   (not built)' })) 20 60 200 $hx64d $hx64d
$cbX86Static  = New-Check ('Win32 static'  + $(if ($hx86s) { '' } else { '   (not built)' })) 240 30 200 $hx86s $hx86s
$cbX86Dynamic = New-Check ('Win32 dynamic' + $(if ($hx86d) { '' } else { '   (not built)' })) 240 60 200 $hx86d $hx86d
$varGrp.Controls.AddRange(@($cbX64Static, $cbX64Dynamic, $cbX86Static, $cbX86Dynamic))

$noneLbl = New-Label '' 20 100 450 40
$noneLbl.ForeColor = [System.Drawing.Color]::FromArgb(255, 170, 110)
$varGrp.Controls.Add($noneLbl)
if (-not ($hx64s -or $hx64d -or $hx86s -or $hx86d)) {
    $noneLbl.Text = "No release builds detected under Builds\. Run build_installer.bat with 'build' first, or build from Visual Studio."
}

# ---- Privilege group --------------------------------------------------------
$privGrp = New-Group 'Install for' 15 220 495 90
$form.Controls.Add($privGrp)

$rbBoth  = New-Radio 'Ask every run (all users OR this user)' 20 30 450 $true
$rbAdmin = New-Radio 'All users only (requires admin)'         20 55 450 $false
$rbUser  = New-Radio 'This user only (no admin needed)'        240 55 240 $false
$privGrp.Controls.AddRange(@($rbBoth, $rbAdmin, $rbUser))

# ---- Previous-install-aware checkbox ---------------------------------------
$prevGrp = New-Group 'Previous-install awareness' 15 320 495 90
$form.Controls.Add($prevGrp)

$cbPrevAware = New-Check 'Detect an existing install and offer upgrade / uninstall / install elsewhere' 20 30 460 $true
$prevGrp.Controls.Add($cbPrevAware)

$prevHint = New-Label 'Uncheck for a standalone superlight setup that just installs, no checks.' 20 55 460 20
$prevHint.ForeColor = $fgDim
$prevGrp.Controls.Add($prevHint)

# ---- Buttons ---------------------------------------------------------------
$okBtn = New-Object System.Windows.Forms.Button
$okBtn.Text = 'Build installer'; $okBtn.Location = New-Object System.Drawing.Point(285, 450)
$okBtn.Size = New-Object System.Drawing.Size(120, 30); $okBtn.BackColor = $panel; $okBtn.ForeColor = $fg
$okBtn.FlatStyle = 'Flat'

$cancelBtn = New-Object System.Windows.Forms.Button
$cancelBtn.Text = 'Cancel'; $cancelBtn.Location = New-Object System.Drawing.Point(415, 450)
$cancelBtn.Size = New-Object System.Drawing.Size(95, 30); $cancelBtn.BackColor = $panel; $cancelBtn.ForeColor = $fg
$cancelBtn.FlatStyle = 'Flat'
$cancelBtn.DialogResult = 'Cancel'

$form.AcceptButton = $okBtn
$form.CancelButton = $cancelBtn
$form.Controls.AddRange(@($okBtn, $cancelBtn))

$okBtn.Add_Click({
    if (-not ($cbX64Static.Checked -or $cbX64Dynamic.Checked -or $cbX86Static.Checked -or $cbX86Dynamic.Checked)) {
        [System.Windows.Forms.MessageBox]::Show('Pick at least one build variant to include.', 'Nothing selected',
            [System.Windows.Forms.MessageBoxButtons]::OK, [System.Windows.Forms.MessageBoxIcon]::Warning) | Out-Null
        return
    }
    $form.DialogResult = 'OK'
    $form.Close()
})

$result = $form.ShowDialog()
if ($result -ne 'OK') { exit 1 }

$priv = 'both'
if ($rbAdmin.Checked) { $priv = 'admin' }
elseif ($rbUser.Checked) { $priv = 'user' }

$lines = @()
$lines += "INCLUDE_X64_STATIC="  + $(if ($cbX64Static.Checked)  { '1' } else { '0' })
$lines += "INCLUDE_X64_DYNAMIC=" + $(if ($cbX64Dynamic.Checked) { '1' } else { '0' })
$lines += "INCLUDE_X86_STATIC="  + $(if ($cbX86Static.Checked)  { '1' } else { '0' })
$lines += "INCLUDE_X86_DYNAMIC=" + $(if ($cbX86Dynamic.Checked) { '1' } else { '0' })
$lines += "PRIV=$priv"
$lines += "PREV_AWARE=" + $(if ($cbPrevAware.Checked) { '1' } else { '0' })

[System.IO.File]::WriteAllLines($OutFile, $lines)
exit 0
