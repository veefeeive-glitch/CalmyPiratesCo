# Live HOLD1 route harness: Begin Watch → CalmyLiveHold1 HOLD1 → grab → green zone teleport → settle.
param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [switch]$UseDebugTeleport
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$out = Join-Path $root 'Saved/Verification/LiveHold1-Route'
$harness = Join-Path $root 'Scripts/play_harness.py'
New-Item -ItemType Directory -Force -Path $out | Out-Null
Get-ChildItem $out -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue

Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

$launchArgs = @(
    $project,
    '/Game/Maps/OceanFreighterR7',
    '-game', '-windowed', '-ResX=1280', '-ResY=720',
    '-CalmyStartPaused',
    '-nosplash', '-NoSound'
)
if ($UseDebugTeleport) { $launchArgs += '-CalmyLiveHold1' }

$proc = Start-Process -FilePath $Editor -ArgumentList $launchArgs -PassThru
Start-Sleep -Seconds 35

python $harness seq $out `
    'shot:00-menu' `
    'sleep:1' `
    'tap:ENTER' `
    'sleep:3' `
    'shot:01-after-begin' `
    'sleep:1' `
    'look:0:300' `
    'sleep:0.5' `
    'tap:E' `
    'sleep:1' `
    'tap:E' `
    'sleep:2' `
    'shot:02-held-or-route' `
    'sleep:4' `
    'shot:09-final'

Start-Sleep -Seconds 2
$log = Join-Path $root 'Saved/Logs/CalmyPiratesCo.log'
$okHold = Select-String -Path $log -Pattern 'CALMY_LIVE_HOLD1 teleported_to_hold1' -Quiet
$okGreen = Select-String -Path $log -Pattern 'CALMY_LIVE_HOLD1 teleported_to_green_zone' -Quiet
if (!$proc.HasExited) { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
if ($UseDebugTeleport -and (-not $okHold)) { throw 'LIVE_HOLD1 missing hold teleport marker' }
Write-Host "LIVE_HOLD1_HARNESS_DONE hold=$okHold green=$okGreen out=$out"
