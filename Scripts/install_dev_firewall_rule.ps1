param([switch]$Elevated)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$exe = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Builds\Windows-Current\CalmyPiratesCo\Binaries\Win64\CalmyPiratesCo.exe'))
$expectedRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Builds\Windows-Current'))
$ruleName = 'Calmy Pirates Co. Development (Private)'

if (-not $exe.StartsWith($expectedRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to configure an executable outside $expectedRoot"
}
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Stable packaged executable is missing. Run package_windows.ps1 first: $exe"
}

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = [Security.Principal.WindowsPrincipal]::new($identity)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    if ($Elevated) { throw 'The elevated firewall helper is not running as Administrator.' }
    $powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', ('"' + $PSCommandPath + '"'), '-Elevated')
    try {
        $process = Start-Process -FilePath $powershell -ArgumentList $arguments -Verb RunAs -Wait -PassThru
    } catch {
        throw 'The one-time Windows administrator confirmation was cancelled. No firewall rule was changed.'
    }
    if ($process.ExitCode -ne 0) { throw "Firewall setup was cancelled or failed (exit $($process.ExitCode))." }
    Write-Output "FIREWALL_RULE_PASS $ruleName"
    exit 0
}

$rules = @(Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue)
if ($rules.Count -eq 0) {
    New-NetFirewallRule -DisplayName $ruleName -Description 'Stable Unreal development build; inbound access only on Private networks.' `
        -Direction Inbound -Action Allow -Enabled True -Profile Private -Program $exe -Protocol Any | Out-Null
} else {
    foreach ($rule in $rules) {
        Set-NetFirewallRule -InputObject $rule -Direction Inbound -Action Allow -Enabled True -Profile Private | Out-Null
        Get-NetFirewallApplicationFilter -AssociatedNetFirewallRule $rule | Set-NetFirewallApplicationFilter -Program $exe | Out-Null
    }
}

$verified = @(Get-NetFirewallRule -DisplayName $ruleName -ErrorAction Stop | Where-Object {
    $_.Enabled.ToString() -eq 'True' -and
    $_.Direction.ToString() -eq 'Inbound' -and
    $_.Action.ToString() -eq 'Allow' -and
    $_.Profile.ToString() -match '(^|,\s*)Private($|,)'
})
if ($verified.Count -eq 0) { throw "Firewall rule verification failed: $ruleName" }
Write-Output "FIREWALL_RULE_PASS $ruleName`nPROGRAM $exe`nPROFILE Private`nPUBLIC_ACCESS False"
