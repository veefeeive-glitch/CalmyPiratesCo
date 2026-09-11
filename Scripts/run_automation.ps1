param([string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$log = Join-Path $projectRoot ('Saved\Logs\Automation-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '.log')
& $Editor (Join-Path $projectRoot 'CalmyPiratesCo.uproject') -unattended -nop4 -NullRHI -NoSound `
    '-ExecCmds=Automation RunTests CalmyPiratesCo' '-TestExit=Automation Test Queue Empty' "-abslog=$log" `
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' | Out-Null
$code = $LASTEXITCODE
$results = @(Select-String -LiteralPath $log -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
$passed = @($results | Where-Object { $_.Line -match 'Result=\{Success\}' })
# This engine can exit 0 even after a failed automation assertion. Inspect project test results explicitly.
if ($code -ne 0 -or $results.Count -lt 20 -or $passed.Count -ne $results.Count) { throw "Automation failed: $($passed.Count)/$($results.Count), exit $code. $log" }
Write-Output "AUTOMATION_PASS $($passed.Count)/$($results.Count) $log"
