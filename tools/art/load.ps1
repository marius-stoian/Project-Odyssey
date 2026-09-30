param([string]$Path)
Add-Type -AssemblyName System.Drawing
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not ('Sheet' -as [type])) { Add-Type -Path "$here\Sheet.cs" -ReferencedAssemblies System.Collections,System.Runtime,System.Console }
$b = [System.Drawing.Bitmap]::new($Path)
$d = $b.LockBits([System.Drawing.Rectangle]::new(0,0,$b.Width,$b.Height), 'ReadOnly', 'Format32bppArgb')
$px = New-Object byte[] ($b.Width*$b.Height*4)
for ($y=0; $y -lt $b.Height; $y++) { [Runtime.InteropServices.Marshal]::Copy([IntPtr]::Add($d.Scan0, $y*$d.Stride), $px, $y*$b.Width*4, $b.Width*4) }
$b.UnlockBits($d); $w=$b.Width; $h=$b.Height; $b.Dispose()
[Sheet]::new($w,$h,$px)
