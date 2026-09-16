# Apply watertight door visuals to all 61 R7 doors (full editor, -ExecutePythonScript).
param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$script = Join-Path $PSScriptRoot 'upgrade_door_visuals_v1.py'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$log = Join-Path $root ("Saved/Verification/{0}-DoorVisual-V1-Install.log" -f $stamp)
New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
& $Editor $project "-ExecutePythonScript=$script" -unattended -nop4 -NoSound "-abslog=$log" `
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
$code = $LASTEXITCODE
if (-not (Select-String -Path $log -Pattern 'DOOR_VISUAL_V1_PASS' -Quiet)) {
    throw "door visual marker missing in $log (exit=$code)"
}
Write-Host "DOOR_VISUAL_V1_PASS exit=$code log=$log"
