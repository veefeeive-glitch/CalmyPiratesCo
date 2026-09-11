param([string]$Label='baseline',[switch]$NoBrain,[switch]$NoWaterMesh,[int]$Quality=-1)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
if(Get-Process UnrealEditor -ErrorAction SilentlyContinue){throw 'Close other Unreal game/editor processes before an isolated performance measurement.'}
$evidencePath=Join-Path $projectRoot ('Saved/Verification/Performance-S1-'+$Label+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $evidencePath | Out-Null
$settingsPath=Join-Path $projectRoot 'Saved/Config/WindowsEditor/GameUserSettings.ini'
$settingsBackup=Join-Path $evidencePath 'GameUserSettings-before.ini'
if(Test-Path -LiteralPath $settingsPath){Copy-Item -LiteralPath $settingsPath -Destination $settingsBackup}
$commands='t.MaxFPS 0,r.VSync 0'
if($Quality -ge 0){$commands+=",sg.ViewDistanceQuality $Quality,sg.ShadowQuality $Quality,sg.GlobalIlluminationQuality $Quality,sg.ReflectionQuality $Quality,sg.PostProcessQuality $Quality,sg.EffectsQuality $Quality,sg.TextureQuality $Quality,sg.AntiAliasingQuality $Quality"}
$gameArgs=@(('"'+(Join-Path $projectRoot 'CalmyPiratesCo.uproject')+'"'),'/Game/Maps/OceanFreighterR7','-game','-windowed','-ResX=1280','-ResY=800','-NoSound','-DisablePython','-unattended',"-CalmyPerformanceProbe=$Label",('-ExecCmds="'+$commands+'"'),('-abslog="'+(Join-Path $evidencePath 'Performance.log')+'"'),'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($NoBrain){$gameArgs+='-PerfNoBrain'}
if($NoWaterMesh){$gameArgs+='-PerfNoWaterMesh'}
$testProcess=Start-Process -FilePath 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $gameArgs -PassThru -WindowStyle Hidden
Write-Output "Evidence: $evidencePath"
try {
    if(-not $testProcess.WaitForExit(150000) -or $testProcess.ExitCode -ne 0){throw 'Performance probe failed/timed out'}
    $result=Select-String -LiteralPath (Join-Path $evidencePath 'Performance.log') -Pattern 'CALMY_PERF .* RESULT'
    if(-not $result){throw 'No performance result'}
    $result.Line
} finally {
    if(-not $testProcess.HasExited){Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue}
    if(Test-Path -LiteralPath $settingsBackup){Copy-Item -LiteralPath $settingsBackup -Destination $settingsPath}
}
