# Create /Game/Maps/PortHub (minimal pier + mission board).
param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$script = Join-Path $PSScriptRoot 'create_porthub_map.py'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$log = Join-Path $root ("Saved/Verification/{0}-PortHub-Create.log" -f $stamp)
New-Item -ItemType Directory -Force -Path (Split-Path $log) | Out-Null
Get-Process UnrealEditor*,UnrealEditor-Cmd* -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
& $Editor $project "-ExecutePythonScript=$script" -unattended -nop4 -NoSound "-abslog=$log" `
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
$code = $LASTEXITCODE
if (-not (Select-String -Path $log -Pattern 'PORTHUB_MAP_PASS|PORTHUB_MAP_EXISTS' -Quiet)) {
    throw "PortHub marker missing in $log (exit=$code)"
}
Write-Host "PORTHUB_OK exit=$code log=$log"
