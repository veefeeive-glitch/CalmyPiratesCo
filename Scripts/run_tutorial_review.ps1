param([switch]$NullRHI)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$role=if($NullRHI){'Solo'}else{'Visual'}
$label='Tutorial-'+$role+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Game.log'
$launch=@(('"'+$root+'/CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-unattended','-NoSound','-DisablePython','-windowed','-ResX=1280','-ResY=800',"-CalmyTutorialProbe=$role","-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($NullRHI){$launch+='-NullRHI'}
$proc=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $launch -WindowStyle Hidden -PassThru
try {
    Write-Output "Evidence: $dir"
    $until=(Get-Date).AddSeconds(200)
    while(-not $proc.HasExited -and (Get-Date)-lt $until){Start-Sleep -Seconds 1}
    if(-not $proc.HasExited){throw 'Tutorial review timed out'}
    if($proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern "CALMY_TUTORIAL_TEST $role PASS" -Quiet) -or (Select-String -LiteralPath $log -Pattern 'CALMY_TUTORIAL_TEST .* FAIL|Fatal error:|Ensure condition failed:|Assertion failed:' -Quiet)){throw "Tutorial review failed: $dir"}
    (Select-String -LiteralPath $log -Pattern "CALMY_TUTORIAL_TEST $role PASS").Line
    if(-not $NullRHI -and @(Get-ChildItem $dir -Filter '*.png').Count -lt 5){throw 'Missing tutorial screenshots'}
    'TUTORIAL_REVIEW_PASS'
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id}}
