param(
    [ValidateSet('Solo','Host','Client')][string]$Mode = 'Solo',
    [int]$Port = 17777,
    [switch]$StartPaused,
    [string]$PackagedExe,
    [ValidateSet('WaterLab','OceanFreighterR7','FreighterDeck','ShiftTest','CargoLab','LeakTest')][string]$MapName = 'WaterLab'
)
$ErrorActionPreference = 'Stop'
if ($Port -lt 1024 -or $Port -gt 65535) { throw 'Use a port from 1024 to 65535.' }
$projectRoot = Split-Path -Parent $PSScriptRoot
$argsList = @()
if ($PackagedExe) {
    $exe = (Resolve-Path -LiteralPath $PackagedExe).Path
} else {
    $exe = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
    $argsList += ('"' + (Join-Path $projectRoot 'CalmyPiratesCo.uproject') + '"')
}
$map = switch ($Mode) {
    'Host' { "/Game/Maps/${MapName}?listen" }
    'Client' { "127.0.0.1:$Port" }
    default { "/Game/Maps/$MapName" }
}
if ($Mode -eq 'Host' -and (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue)) { throw "Port $Port is already occupied." }
$argsList += @($map, '-game', '-DisablePython', "-port=$Port", '-MULTIHOME=127.0.0.1', 
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
# Visible game windows are intentional. Loopback only: no firewall changes or Internet exposure.
if ($StartPaused) { $argsList += '-CalmyStartPaused' }
$process = Start-Process -FilePath $exe -ArgumentList $argsList -PassThru
Write-Output "$Mode game started (PID $($process.Id)). Local loopback multiplayer only."
