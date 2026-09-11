param([string]$Editor='E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$label='Fly-Travel-R6-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$evidencePath=Join-Path $projectRoot ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $evidencePath | Out-Null
$logPath=Join-Path $evidencePath 'Visual.log'
$gameArgs=@(('"'+(Join-Path $projectRoot 'CalmyPiratesCo.uproject')+'"'),'/Game/Maps/OceanFreighterR7','-game','-windowed','-ResX=1280','-ResY=800','-NoSound','-DisablePython','-unattended','-CalmyFlyTravelProbe=Visual',"-CalmyReviewDir=$label",'-ExecCmds="t.MaxFPS 60"',('-abslog="'+$logPath+'"'),'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$reviewProcess=Start-Process -FilePath $Editor -ArgumentList $gameArgs -PassThru -WindowStyle Hidden
Write-Output "Evidence: $evidencePath"
try {
    if(-not $reviewProcess.WaitForExit(240000)){throw 'Visual probe timeout'}
    if($reviewProcess.ExitCode -ne 0 -or -not (Select-String -LiteralPath $logPath -Pattern 'CALMY_FLY_TRAVEL_TEST Visual PASS' -Quiet)){throw 'Visual probe failed'}
    $pictures=@(Get-ChildItem -LiteralPath $evidencePath -Filter '*-travelling-colony.png')
    if($pictures.Count -ne 3){throw 'Expected three rendered travel screenshots'}
    Select-String -LiteralPath $logPath -Pattern 'CALMY_FLY_TRAVEL_TEST Visual PASS' | ForEach-Object Line
    Write-Output "VISUAL_PASS $evidencePath"
} finally {
    if(-not $reviewProcess.HasExited){Stop-Process -Id $reviewProcess.Id -ErrorAction SilentlyContinue}
}
