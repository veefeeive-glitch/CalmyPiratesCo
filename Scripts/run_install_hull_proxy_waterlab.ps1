param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root 'CalmyPiratesCo.uproject'
$script = Join-Path $PSScriptRoot 'install_hull_proxy_waterlab.py'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$logDir = Join-Path $root "Saved/Verification"
New-Item -ItemType Directory -Force -Path $logDir | Out-Null
$log = Join-Path $logDir "$stamp-HullProxy-Install.log"
$argsList = @(
    "`"$project`"",
    '-run=pythonscript',
    "-script=`"$script`"",
    '-unattended', '-nop4', '-nosplash', '-NullRHI'
)
$proc = Start-Process -FilePath $Editor -ArgumentList $argsList -PassThru -WindowStyle Hidden `
    -RedirectStandardOutput $log -RedirectStandardError $log
Wait-Process -Id $proc.Id -Timeout 600
if ($proc.ExitCode -ne 0) { throw "Hull proxy install failed exit=$($proc.ExitCode) log=$log" }
if (-not (Select-String -LiteralPath $log -Pattern 'HULL_PROXY_INSTALL' -Quiet)) {
    # Unreal often logs to Saved/Logs instead of redirected stdout.
    $editorLog = Get-ChildItem (Join-Path $root 'Saved/Logs') -Filter 'CalmyPiratesCo*.log' |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $editorLog -or -not (Select-String -LiteralPath $editorLog.FullName -Pattern 'HULL_PROXY_INSTALL' -Quiet)) {
        throw "HULL_PROXY_INSTALL marker missing. log=$log"
    }
    Copy-Item $editorLog.FullName $log -Force
}
"HULL_PROXY_INSTALL_OK log=$log"
