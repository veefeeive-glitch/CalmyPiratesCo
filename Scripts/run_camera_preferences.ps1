param()
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$g1Dir=Join-Path $projectRoot ('Saved/Verification/G1-Preferences-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
$g1Profile=(Join-Path $g1Dir 'Profile').Replace('\','/')
New-Item -ItemType Directory -Path $g1Profile | Out-Null
# Two real game processes, isolated user profile. Deliberately NOT unattended:
# the production save/load path disables preference writes in unattended tests.
foreach($g1Role in @('PrefsWrite','PrefsRead')) {
    $g1Log=Join-Path $g1Dir ($g1Role+'.log')
    $g1Args=@(('"'+$projectRoot+'\CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-NullRHI','-NoSound','-DisablePython','-nop4',
        ('-UserDir="'+$g1Profile+'"'),('-abslog="'+$g1Log+'"'),("-CalmyPauseProbe=$g1Role"),'-ExecCmds="t.MaxFPS 60"',
        '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
    $g1Proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $g1Args -WindowStyle Hidden -PassThru
    try {
        if(-not $g1Proc.WaitForExit(80000)){throw "Preference $g1Role timed out"}
        if($g1Proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $g1Log -Pattern "CALMY_PAUSE_TEST $g1Role PASS" -Quiet) -or
            (Select-String -LiteralPath $g1Log -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)) {throw "Preference $g1Role failed: $g1Log"}
        Select-String -LiteralPath $g1Log -Pattern 'G1_PREFERENCES' | ForEach-Object {$_.Line}
    } finally {if(-not $g1Proc.HasExited){Stop-Process -Id $g1Proc.Id}}
}
Write-Output "G1_PREFERENCES_PASS $g1Dir"
