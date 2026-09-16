param([string]$Label='',[switch]$IgnoreRails)
# Player-view playability survey of OceanFreighterR7 (UCalmyPlayabilityProbe) + analysis and deck maps. NullRHI, solo, no saved changes.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
if(-not $Label){$Label='ShipPlayability-'+(Get-Date -Format 'yyyyMMdd-HHmmss')}
$dir=Join-Path $root ('Saved/Verification/'+$Label)
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$log=Join-Path $dir 'Playability.log'
$probeArgs=@(('"'+$root+'\CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-NullRHI','-unattended','-NoSound','-nosplash','-DisablePython',
 '-CalmyPlayabilityProbe=Solo',('-CalmyReviewDir='+$Label),('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
 '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($IgnoreRails){$probeArgs+='-CalmyPlayabilityIgnoreRails'}
$proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $probeArgs -WindowStyle Hidden -PassThru
Write-Output "Evidence: $dir"
try {
 if(-not $proc.WaitForExit(1600000)){throw "Playability survey timed out: $dir"}
 $done=Select-String -LiteralPath $log -Pattern 'CALMY_PLAYABILITY (DONE|FAIL)' | Select-Object -Last 1
 if(-not $done){throw "Playability survey did not finish: $dir"}
 Write-Output $done.Line
 if($done.Line -match 'CALMY_PLAYABILITY FAIL'){exit 1}
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue}}
& python (Join-Path $PSScriptRoot 'analyze_ship_playability.py') $dir
if($LASTEXITCODE -ne 0){exit $LASTEXITCODE}
