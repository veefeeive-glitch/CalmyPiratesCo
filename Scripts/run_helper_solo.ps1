param([string]$Editor='E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe')
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$logPath=Join-Path $projectRoot ('Saved/Logs/Helper-Solo-'+(Get-Date -Format 'yyyyMMdd-HHmmss')+'.log')
$probeArgs=@(('"'+(Join-Path $projectRoot 'CalmyPiratesCo.uproject')+'"'),'/Game/Maps/WaterLab',
    '-game','-NullRHI','-NoSound','-unattended','-nop4','-DisablePython','-CalmyHelperProbe=Solo',
    '-ExecCmds="t.MaxFPS 60"',('-abslog="'+$logPath+'"'),
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$probeProcess=Start-Process -FilePath $Editor -ArgumentList $probeArgs -PassThru -WindowStyle Hidden
try {
    if(-not $probeProcess.WaitForExit(180000)) { throw "Solo helper timed out: $logPath" }
    if($probeProcess.ExitCode -ne 0 -or -not (Select-String -LiteralPath $logPath -Pattern 'CALMY_HELPER Solo PASS' -Quiet) -or
        (Select-String -LiteralPath $logPath -Pattern 'CALMY_HELPER Solo FAIL|Fatal error:|Assertion failed:|Ensure condition failed:' -Quiet)) {
        throw "Solo helper failed: $logPath"
    }
    (Select-String -LiteralPath $logPath -Pattern 'CALMY_HELPER Solo PASS').Line
    "SOLO_HELPER_PASS $logPath"
} finally {
    if(-not $probeProcess.HasExited) { Stop-Process -Id $probeProcess.Id -Force }
}
