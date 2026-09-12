# Hero G1: cargo grip + CrewR8 hand sockets (Editor)
# Stamps sockets on existing meshes via Unreal Python. Does not invent art.
param(
  [string]$EngineRoot = "E:/Games/UE_5.8",
  [switch]$HandsOnly,
  [switch]$CargoOnly
)
$ErrorActionPreference = "Stop"
$Project = (Resolve-Path (Join-Path $PSScriptRoot "..\CalmyPiratesCo.uproject")).Path
$Editor = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $Editor)) { throw "Missing UnrealEditor-Cmd: $Editor" }

function Invoke-EditorPython([string]$ScriptRel) {
  $Script = (Resolve-Path (Join-Path $PSScriptRoot $ScriptRel)).Path
  & $Editor $Project -unattended -nop4 -nosound -NullRHI -ExecutePythonScript="$Script"
  if ($LASTEXITCODE -ne 0) { throw "Editor Python failed ($LASTEXITCODE): $ScriptRel" }
}

if (-not $HandsOnly) { Invoke-EditorPython "stamp_cargo_handle_sockets.py" }
if (-not $CargoOnly) { Invoke-EditorPython "stamp_crew_hand_sockets.py" }
Write-Host "HERO_G1_SOCKET_STAMP_PASS"
