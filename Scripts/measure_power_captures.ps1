param(
    [Parameter(Mandatory)][string]$Normal,
    [Parameter(Mandatory)][string]$Degraded,
    [Parameter(Mandatory)][string]$OutputJson
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$normalImage = [System.Drawing.Bitmap]::new((Resolve-Path -LiteralPath $Normal).Path)
$degradedImage = [System.Drawing.Bitmap]::new((Resolve-Path -LiteralPath $Degraded).Path)
try {
    if ($normalImage.Width -ne $degradedImage.Width -or $normalImage.Height -ne $degradedImage.Height) { throw 'Capture dimensions differ.' }
    function Measure-Image([System.Drawing.Bitmap]$Image) {
        [double]$luminance = 0
        [long]$orange = 0; [long]$green = 0; [long]$cyan = 0; [long]$brightNeutral = 0
        for ($y = 100; $y -lt $Image.Height; $y += 2) {
            for ($x = 0; $x -lt $Image.Width; $x += 2) {
                $p = $Image.GetPixel($x, $y); $r = [double]$p.R; $g = [double]$p.G; $b = [double]$p.B
                $luminance += .2126*$r + .7152*$g + .0722*$b
                if ($r -gt 75 -and $r -gt $g*1.22 -and $r -gt $b*1.45) { ++$orange }
                if ($g -gt 55 -and $g -gt $r*1.22 -and $g -gt $b*1.04) { ++$green }
                if ($b -gt 55 -and $b -gt $r*1.25 -and $g -gt $r*1.12) { ++$cyan }
                if ([Math]::Min($r,[Math]::Min($g,$b)) -gt 70 -and [Math]::Max($r,[Math]::Max($g,$b)) - [Math]::Min($r,[Math]::Min($g,$b)) -lt 42) { ++$brightNeutral }
            }
        }
        $samples = [Math]::Ceiling(($Image.Height-100)/2) * [Math]::Ceiling($Image.Width/2)
        [ordered]@{ meanLuminance=[Math]::Round($luminance/$samples,2); orangePixels=$orange; greenPixels=$green; cyanPixels=$cyan; brightNeutralPixels=$brightNeutral; samples=$samples }
    }
    $normalMetrics = Measure-Image $normalImage
    $degradedMetrics = Measure-Image $degradedImage
    $ratio = [Math]::Round($degradedMetrics.meanLuminance / $normalMetrics.meanLuminance,3)
    $result = [ordered]@{ normal=$normalMetrics; degraded=$degradedMetrics; degradedLuminanceRatio=$ratio; dimensions="$($normalImage.Width)x$($normalImage.Height)" }
    $result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $OutputJson -Encoding utf8
    if ($ratio -ge .88 -or $ratio -le .3 -or $degradedMetrics.orangePixels -lt 100 -or $degradedMetrics.greenPixels -lt 100 -or $degradedMetrics.cyanPixels -lt 100 -or $degradedMetrics.brightNeutralPixels -lt 100) {
        throw "Power-light capture thresholds failed. Metrics: $OutputJson"
    }
    Write-Output "POWER_CAPTURE_METRICS_PASS $OutputJson"
} finally { $normalImage.Dispose(); $degradedImage.Dispose() }
