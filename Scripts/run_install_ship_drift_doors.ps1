# Close R7 doors + install ShipMotion (full editor, -ExecutePythonScript).
param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$script = Join-Path $PSScriptRoot 'install_ship_drift_doors.py'
$log = Join-Path $root ('Saved/Verification/{0}-ShipDrift-Doors-Install.log' -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
& $Editor $project "-ExecutePythonScript=$script" -unattended -nop4 -NoSound "-abslog=$log" `
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
$code = $LASTEXITCODE
if (-not (Select-String -Path $log -Pattern 'SHIP_DRIFT_DOORS_INSTALL' -Quiet)) {
    throw "install marker missing in $log (exit=$code)"
}
Write-Host "INSTALL_PASS exit=$code log=$log"
