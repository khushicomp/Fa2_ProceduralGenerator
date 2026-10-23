# Deterministic extraction from the user-supplied 1536x1024 atlas.
# Tight structural crops exclude surrounding atlas terrain. Ellipse masks stay
# inside tree crowns; all output files have transparent one-pixel sampling gutter.
Add-Type -AssemblyName System.Drawing
$root = Split-Path $PSScriptRoot -Parent
$sheet = [System.Drawing.Bitmap]::FromFile((Join-Path $root 'assets/source/city-atlas.png'))
$items = @(
 @('buildings/residential/01',48,28,57,85,'rect'),
 @('buildings/residential/02',176,39,55,74,'rect'),
 @('buildings/residential/03',290,44,72,61,'rect'),
 @('buildings/residential/04',686,44,56,72,'rect'),
 @('buildings/residential/05',1059,41,62,71,'rect'),
 @('buildings/residential/06',1435,46,61,70,'rect'),
 @('buildings/commercial/01',47,180,85,64,'rect'),
 @('buildings/commercial/02',182,184,83,63,'rect'),
 @('buildings/commercial/03',560,181,78,64,'rect'),
 @('buildings/commercial/04',808,182,80,65,'rect'),
 @('buildings/office/01',448,320,103,79,'rect'),
 @('buildings/office/02',637,312,110,84,'rect'),
 @('buildings/office/03',1008,321,109,74,'rect'),
 @('buildings/office/04',1205,319,111,74,'rect'),
 @('buildings/hospital/01',45,322,119,78,'rect'),
 @('vegetation/trees/01',19,774,45,47,'ellipse'),
 @('vegetation/trees/02',87,783,43,41,'ellipse'),
 @('vegetation/trees/03',157,780,47,47,'ellipse'),
 @('vegetation/trees/04',85,846,41,42,'ellipse')
)
foreach ($item in $items) {
 $name,$x,$y,$w,$h,$shape=$item
 $dest=Join-Path $root ('assets/'+$name+'.png')
 New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
 $bitmap=New-Object System.Drawing.Bitmap ($w+4),($h+4)
 $g=[System.Drawing.Graphics]::FromImage($bitmap)
 $g.Clear([System.Drawing.Color]::Transparent)
 if ($shape -eq 'ellipse') {
  $path=New-Object System.Drawing.Drawing2D.GraphicsPath
  $path.AddEllipse(2,2,$w,$h); $g.SetClip($path); $path.Dispose()
 }
 $g.DrawImage($sheet,[System.Drawing.Rectangle]::new(2,2,$w,$h),[System.Drawing.Rectangle]::new($x,$y,$w,$h),[System.Drawing.GraphicsUnit]::Pixel)
 # Audit every alpha pixel and retain a single sampling gutter, never crop visible art.
 $left=$bitmap.Width; $top=$bitmap.Height; $right=-1; $bottom=-1
 for ($py=0; $py -lt $bitmap.Height; ++$py) {
  for ($px=0; $px -lt $bitmap.Width; ++$px) {
   if ($bitmap.GetPixel($px,$py).A -gt 0) {
    $left=[Math]::Min($left,$px);$top=[Math]::Min($top,$py)
    $right=[Math]::Max($right,$px);$bottom=[Math]::Max($bottom,$py)
   }
  }
 }
 $crop=[System.Drawing.Rectangle]::new($left-1,$top-1,$right-$left+3,$bottom-$top+3)
 $tight=$bitmap.Clone($crop,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
 $tight.Save($dest,[System.Drawing.Imaging.ImageFormat]::Png)
 Write-Output "$name : visible $($right-$left+1)x$($bottom-$top+1), stored $($tight.Width)x$($tight.Height)"
 $tight.Dispose()
 $g.Dispose();$bitmap.Dispose()
}
$sheet.Dispose()
