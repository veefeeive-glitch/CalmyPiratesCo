param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [string]$Blender = 'E:\Tools\blender-5.2.1-windows-x64\blender.exe',
    [switch]$ShowWindows
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'CalmyPiratesCo.uproject'
foreach ($file in @($Editor, $Blender, $project)) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing file: $file" }
}
$windowStyle = if ($ShowWindows) { 'Normal' } else { 'Hidden' }
$running = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'")
$ours = @($running | Where-Object { $_.CommandLine -and $_.CommandLine.Contains($project) -and $_.CommandLine -notmatch '(?i)\s-game\b' })
$mcp = Get-NetTCPConnection -LocalPort 8000 -State Listen -ErrorAction SilentlyContinue
if ($mcp -and $mcp.OwningProcess -notin $ours.ProcessId) {
    throw 'Port 8000 belongs to another process/project. Close that editor normally before starting this project.'
}
$blenderProcesses = @(Get-Process blender -ErrorAction SilentlyContinue)
$blenderPort = @(Get-NetTCPConnection -LocalPort 9876 -State Listen -ErrorAction SilentlyContinue)
if ($blenderPort | Where-Object OwningProcess -NotIn $blenderProcesses.Id) {
    throw 'Port 9876 belongs to another process. No process will be stopped automatically.'
}
if (-not $ours) {
    Start-Process -FilePath $Editor -WindowStyle $windowStyle -ArgumentList @(
        ('"' + $project + '"'), '/Game/Maps/WaterLab', '-MULTIHOME=127.0.0.1',
        '-ini:Engine:HTTPServer.Listeners:DefaultBindAddress=127.0.0.1',
        '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=True'
    )
}
if (-not $blenderProcesses) { Start-Process -FilePath $Blender -WindowStyle $windowStyle }
$until = (Get-Date).AddSeconds(45)
do {
    $ports = @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $_.LocalPort -in 8000,9876 })
    if (8000 -in $ports.LocalPort -and 9876 -in $ports.LocalPort) { break }
    Start-Sleep -Seconds 2
} while ((Get-Date) -lt $until)
$ports | Select-Object LocalAddress,LocalPort,OwningProcess | Format-Table -AutoSize
if ($ports | Where-Object LocalAddress -NotIn @('127.0.0.1','::1')) {
    throw 'An MCP listener is not loopback-only. Do not use it until its binding is corrected.'
}
if (8000 -notin $ports.LocalPort -or 9876 -notin $ports.LocalPort) {
    Write-Warning 'One MCP endpoint is still unavailable. Check editor startup and the Blender add-on server.'
}
