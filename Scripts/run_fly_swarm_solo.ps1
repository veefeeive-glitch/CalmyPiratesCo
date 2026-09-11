param(
    [ValidateRange(20,30)][int]$Hz = 30,
    [ValidateRange(10,144)][int]$RenderFPS = 20
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$evidence = Join-Path $projectRoot ('Saved/Verification/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + "-FlySwarm-Solo-$Hz-$RenderFPS")
New-Item -ItemType Directory -Path $evidence | Out-Null
$log = Join-Path $evidence 'Solo.log'
$argsList = @(('"' + (Join-Path $projectRoot 'CalmyPiratesCo.uproject') + '"'), '/Game/Maps/WaterLab',
    '-game', '-NullRHI', '-NoSound', '-unattended', '-nop4', '-DisablePython', '-CalmyFlySwarmProbe=Solo', "-FlyHz=$Hz",
    ('-ExecCmds="t.MaxFPS ' + $RenderFPS + '"'), ('-abslog="' + $log + '"'),
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$proc = Start-Process -FilePath 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsList -PassThru -WindowStyle Hidden
try {
    $until = (Get-Date).AddSeconds(160)
    while (-not $proc.HasExited -and (Get-Date) -lt $until) { Start-Sleep -Seconds 1 }
    $pass = Select-String -LiteralPath $log -Pattern 'CALMY_FLY_SWARM_TEST Solo PASS'
    if (-not $proc.HasExited -or $proc.ExitCode -ne 0 -or -not $pass -or
        (Select-String -LiteralPath $log -Pattern 'CALMY_FLY_SWARM_TEST Solo FAIL|Fatal error:|Assertion failed:|Ensure condition failed:' -Quiet)) {
        throw "Fly swarm solo failed: $log"
    }
    $pass.Line
    "FLY_SWARM_SOLO_PASS Hz=$Hz render_cap=$RenderFPS $log"
} finally {
    if (-not $proc.HasExited) { Stop-Process -Id $proc.Id }
}
