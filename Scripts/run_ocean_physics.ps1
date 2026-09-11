param(
    [string]$Model = 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend',
    [string]$ReportName = ('Ocean-' + (Get-Date -Format 'yyyyMMdd-HHmmss')),
    [switch]$TraversalAudit,
    [switch]$CarryAudit,
    [switch]$NpcAudit
)
$ErrorActionPreference='Stop'
if($NpcAudit) { $CarryAudit=$true }
$rootPath=Split-Path -Parent $PSScriptRoot
if($ReportName -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Use a simple report name.' }
$modelPath=(Resolve-Path -LiteralPath (Join-Path $rootPath $Model)).Path
$outPath=Join-Path $rootPath 'Saved/OceanPhysics'
New-Item -ItemType Directory -Path $outPath -Force | Out-Null
$reportPath=Join-Path $outPath ($ReportName+'.json')
if(Test-Path -LiteralPath $reportPath) { throw "Report already exists: $reportPath" }
$lockPath=Join-Path $outPath 'runner.lock'
New-Item -ItemType Directory -Path $lockPath -ErrorAction Stop | Out-Null
try {
    $previousAuditMode=$env:CALMY_TRAVERSAL_AUDIT
    $env:CALMY_TRAVERSAL_AUDIT=if($TraversalAudit -or $CarryAudit) {'1'} else {'0'}
    & 'E:\Tools\blender-5.2.1-windows-x64\blender.exe' --background $modelPath --python-exit-code 1 --python (Join-Path $PSScriptRoot 'export_ocean_physics.py')
    if($LASTEXITCODE -ne 0) { throw 'Collision export failed' }
    $projectPath=Join-Path $rootPath 'CalmyPiratesCo.uproject'
    $carryArgs=@(if($CarryAudit) { '-OceanCarryAudit' })
    if($NpcAudit) { $carryArgs+='-OceanNpcAudit' }
    & 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' $projectPath /Engine/Maps/Entry -game -NullRHI -NoSound -unattended -nop4 -DisablePython -CalmyOceanPhysics @carryArgs "-OceanReport=$ReportName" -benchmark -fps=60 "-abslog=$outPath/$ReportName.log" '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False'
    # Engine shutdown may return 0 even when the probe requested failure.
    # Require a fresh JSON result with the exact input hash and both controls.
    if(-not(Test-Path -LiteralPath $reportPath)) { throw 'Physical probe did not produce a report' }
    $r=Get-Content -Raw -LiteralPath $reportPath | ConvertFrom-Json
    $hash=(Get-FileHash -LiteralPath $modelPath -Algorithm SHA256).Hash.ToLowerInvariant()
    $minimum=if($CarryAudit) { 56 } else { 176 }
    if(($NpcAudit -and -not $r.npc_audit) -or ($CarryAudit -and -not $r.carry_audit) -or $r.source_sha256 -ne $hash -or $r.total -lt $minimum -or $r.passed -ne $r.total -or $r.controls_total -ne 4 -or $r.controls_passed -ne 4) {
        throw "PHYSICAL_FAIL $($r.passed)/$($r.total). Inspect $reportPath"
    }
    Write-Output "PHYSICAL_PASS $($r.passed)/$($r.total), controls=$($r.controls_passed), source=$hash, report=$reportPath"
} finally {
    $env:CALMY_TRAVERSAL_AUDIT=$previousAuditMode
    Remove-Item -LiteralPath $lockPath
}
