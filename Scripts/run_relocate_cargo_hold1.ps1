# Relocate R7 cargo into HOLD 1 (full editor, -ExecutePythonScript).
param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$script = Join-Path $PSScriptRoot 'relocate_cargo_hold1.py'
$log = Join-Path $root ('Saved/Verification/{0}-CargoHold1-Relocate.log' -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
& $Editor $project "-ExecutePythonScript=$script" -unattended -nop4 -NoSound "-abslog=$log" `
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
$code = $LASTEXITCODE
if (-not (Select-String -Path $log -Pattern 'CARGO_HOLD1_RELOCATE' -Quiet)) {
    throw "relocate marker missing in $log (exit=$code)"
}
Write-Host "CARGO_HOLD1_PASS exit=$code log=$log"
