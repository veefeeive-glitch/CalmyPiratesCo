param([switch]$FallWater)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label='PhysicalCrew-Visual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Visual.log'
$arguments=@(('"'+$root+'/CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-unattended','-NoSound','-DisablePython','-windowed','-ResX=1280','-ResY=800','-CalmyCharacterProbe=Visual',"-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($FallWater){$arguments+='-CalmyFallWaterReview'}
$process=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    Write-Output "Evidence: $dir"
    if(-not $process.WaitForExit(150000)){throw 'Physical crew review timed out'}
    if($process.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_CHARACTER Visual PASS' -Quiet) -or (Select-String -LiteralPath $log -Pattern 'CALMY_CHARACTER .* FAIL|Fatal error:|Ensure condition failed:|Assertion failed:' -Quiet)){throw "Physical crew review failed: $dir"}
    $expected=if($FallWater){6}else{5}
    if(@(Get-ChildItem $dir -Filter '*.png').Count -ne $expected){throw 'Missing physical crew screenshots'}
    'PHYSICAL_CREW_REVIEW_PASS (bounded walking/fall fixture, not full ship traversal)'
} finally {if(-not $process.HasExited){Stop-Process -Id $process.Id}}
