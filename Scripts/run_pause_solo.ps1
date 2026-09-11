param([switch]$Visual)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$role=if($Visual){'Visual'}else{'Solo'}
$label='Pause-'+$role+'-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$evidence=Join-Path $projectRoot ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $evidence | Out-Null
$log=Join-Path $evidence ($role+'.log')
$probeArgs=@(('"'+$projectRoot+'\CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-unattended','-NoSound','-DisablePython','-CalmyStartPaused',
    ('-CalmyPauseProbe='+$role),('-CalmyReviewDir='+$label),('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($Visual){$probeArgs+=@('-windowed','-ResX=1280','-ResY=800')}else{$probeArgs+='-NullRHI'}
$proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $probeArgs -WindowStyle Hidden -PassThru
try {
    Write-Output "Evidence: $evidence"
    if(-not $proc.WaitForExit(120000)){throw 'Pause probe timed out.'}
    if($proc.ExitCode -ne 0 -or -not (Select-String -LiteralPath $log -Pattern "CALMY_PAUSE_TEST $role PASS" -Quiet) -or
        (Select-String -LiteralPath $log -Pattern 'CALMY_PAUSE_TEST .* FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)) {
        throw "Pause probe failed: $log"
    }
    if($Visual -and -not(Test-Path (Join-Path $evidence 'pause-menu.png'))){throw 'Missing rendered pause menu.'}
    Write-Output "PAUSE_SOLO_PASS $role $log"
} finally {
    # Only this runner's child process, never a user's game/editor.
    if(-not $proc.HasExited){Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue}
}
