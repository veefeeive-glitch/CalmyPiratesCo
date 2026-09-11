$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label='FlyMind-R4-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Game.log'
$argsList=@(('"'+$root+'/CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-unattended','-NoSound','-DisablePython','-windowed','-ResX=1280','-ResY=800',
    '-CalmyFlyMindProbe=Visual',"-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$proc=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsList -WindowStyle Hidden -PassThru
try {
    "Evidence: $dir"
    $until=(Get-Date).AddSeconds(190)
    while(-not $proc.HasExited -and (Get-Date)-lt $until){Start-Sleep -Seconds 1}
    if(-not $proc.HasExited -or $proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_FLY_MIND_TEST Visual PASS' -Quiet)){throw "Mind review failed: $dir"}
    if(-not(Test-Path (Join-Path $dir '01-memory-dialogue.png'))){throw 'Missing dialogue screenshot'}
    if(Select-String -LiteralPath $log -Pattern 'Failed to compile Material|Fatal error:|Ensure condition failed:|Assertion failed:' -Quiet){throw 'Mind GPU review logged rendering/runtime errors'}
    (Select-String -LiteralPath $log -Pattern 'CALMY_FLY_MIND_TEST Visual PASS').Line
    'FLY_MIND_REVIEW_PASS'
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id}}
