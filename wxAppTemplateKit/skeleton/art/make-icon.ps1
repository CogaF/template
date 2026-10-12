# Copyright (C) 2026 Fation Coga
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Makes the application icon: two words, the first green over the second red, bold, on a
# transparent background (the same style as GP_Rec's "GP" over "REC"). The template's own icon is
# "TEMP" over "LATE".
#   powershell -ExecutionPolicy Bypass -File make-icon.ps1                       -> TEMP / LATE
#   powershell -ExecutionPolicy Bypass -File make-icon.ps1 -Top RCT -Bottom CTRL -> your own
# Writes app.png (256 px) and app.ico (16-256 px, each size drawn on its own for sharpness) next to
# this script. The .rc file uses app.ico as "appicon": rebuild after running it.
# Needs only Windows PowerShell (System.Drawing), no Python. NewProject.ps1 calls it too.
[CmdletBinding()]
param(
    [string]$Top = 'TEMP',
    [string]$Bottom = 'LATE',
    [string]$OutDir = $PSScriptRoot
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$green = [System.Drawing.Color]::FromArgb(255, 34, 177, 76)
$red   = [System.Drawing.Color]::FromArgb(255, 224, 40, 40)
$sizes = 16, 20, 24, 32, 40, 48, 64, 128, 256
$fontName = 'Arial'
$words = @(@($Top, $Bottom) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | ForEach-Object { $_.Trim() })
if ($words.Count -eq 0) { throw 'Give at least one word (-Top / -Bottom).' }

# Draws one size n x n: 8x larger first, then scaled down (smooth edges at small sizes).
function Render([int]$n) {
    $w = $n * 8
    $big = New-Object System.Drawing.Bitmap $w, $w, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($big)
    $g.Clear([System.Drawing.Color]::Transparent)
    $g.SmoothingMode = 'AntiAlias'
    $g.TextRenderingHint = 'AntiAliasGridFit'
    $rowHeight = $w / $words.Count
    $margin = $w * 0.03
    $format = [System.Drawing.StringFormat]::GenericTypographic
    $family = New-Object System.Drawing.FontFamily $fontName

    # The largest font size at which every word fits its row (same size for all words).
    $size = [double]$w
    while ($true) {
        $path = New-Object System.Drawing.Drawing2D.GraphicsPath
        $ok = $true
        foreach ($t in $words) {
            $path.Reset()
            $path.AddString($t, $family, 1, $size, [System.Drawing.PointF]::Empty, $format)
            $b = $path.GetBounds()
            if ($b.Width -gt ($w - 2 * $margin) -or $b.Height -gt ($rowHeight - 2 * $margin)) { $ok = $false }
        }
        $path.Dispose()
        if ($ok) { break }
        $size -= [Math]::Max(1, $w / 200)
    }

    for ($i = 0; $i -lt $words.Count; $i++) {
        $path = New-Object System.Drawing.Drawing2D.GraphicsPath
        $path.AddString($words[$i], $family, 1, $size, [System.Drawing.PointF]::Empty, $format)
        $b = $path.GetBounds()
        $m = New-Object System.Drawing.Drawing2D.Matrix
        $m.Translate((($w - $b.Width) / 2 - $b.X), ($i * $rowHeight + ($rowHeight - $b.Height) / 2 - $b.Y))
        $path.Transform($m)
        $brush = New-Object System.Drawing.SolidBrush $(if ($i -eq 0) { $green } else { $red })
        $g.FillPath($brush, $path)
        $brush.Dispose(); $path.Dispose()
    }
    $g.Dispose()

    $small = New-Object System.Drawing.Bitmap $n, $n, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $gs = [System.Drawing.Graphics]::FromImage($small)
    $gs.InterpolationMode = 'HighQualityBicubic'
    $gs.PixelOffsetMode = 'HighQuality'
    $gs.DrawImage($big, 0, 0, $n, $n)
    $gs.Dispose(); $big.Dispose()
    return $small
}

# Every size as PNG data, then packed into one .ico (PNG-compressed entries, Vista and later).
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$pngs = @()
foreach ($s in $sizes) {
    $bmp = Render $s
    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    if ($s -eq 256) { $bmp.Save((Join-Path $OutDir 'app.png'), [System.Drawing.Imaging.ImageFormat]::Png) }
    $bmp.Dispose()
    $pngs += , $ms.ToArray()
}

$out = New-Object System.IO.MemoryStream
$bw = New-Object System.IO.BinaryWriter $out
$bw.Write([UInt16]0); $bw.Write([UInt16]1); $bw.Write([UInt16]$sizes.Count)   # ICONDIR
$offset = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {                                       # ICONDIRENTRY
    $dim = if ($sizes[$i] -ge 256) { 0 } else { $sizes[$i] }
    $bw.Write([Byte]$dim); $bw.Write([Byte]$dim); $bw.Write([Byte]0); $bw.Write([Byte]0)
    $bw.Write([UInt16]1); $bw.Write([UInt16]32)
    $bw.Write([UInt32]$pngs[$i].Length); $bw.Write([UInt32]$offset)
    $offset += $pngs[$i].Length
}
foreach ($p in $pngs) { $bw.Write($p) }
$bw.Flush()
[System.IO.File]::WriteAllBytes((Join-Path $OutDir 'app.ico'), $out.ToArray())
Write-Host ("Icon '{0}' written: {1}" -f ($words -join ' / '), (Join-Path $OutDir 'app.ico'))
