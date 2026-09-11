param()
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label='WaterInteraction-Visual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Visual.log'
$arguments=@(('"'+$root+'/CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-unattended','-NoSound','-DisablePython','-windowed','-ResX=1280','-ResY=800','-CalmyWaterInteractionProbe=Visual',"-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$process=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    Write-Output "Evidence: $dir"
    if(-not $process.WaitForExit(150000)){throw 'Water interaction review timed out'}
    if($process.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_WATER_INTERACTION Visual PASS' -Quiet) -or (Select-String -LiteralPath $log -Pattern 'CALMY_WATER_INTERACTION .* FAIL|Fatal error:|Ensure condition failed:|Assertion failed:' -Quiet)){throw "Water interaction review failed: $dir"}
    if(@(Get-ChildItem $dir -Filter '*.png').Count -ne 5){throw 'Missing water screenshots'}
    'WATER_INTERACTION_REVIEW_CAPTURED (camera-only fixture; visual judgement is separate)'
} finally {if(-not $process.HasExited){Stop-Process -Id $process.Id}}
