param([int]$Parallel=12,[string]$Routes='',[string]$Label='')
# In-game walkability + unguarded-edge audit of OceanFreighterR7 (UCalmyTraversalProbe). NullRHI, solo, no saved changes.
# -Routes <json>: walk those routes (e.g. playability_routes.py output) instead of the historical R6/R7 set.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label=if($Label){$Label}else{'ShipTraversal-UE-'+(Get-Date -Format 'yyyyMMdd-HHmmss')}
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$log=Join-Path $dir 'Traversal.log'
$probeArgs=@(('"'+$root+'\CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-NullRHI','-unattended','-NoSound','-nosplash','-DisablePython',
 '-CalmyTraversalProbe=Solo',('-CalmyReviewDir='+$label),"-CalmyTraversalParallel=$Parallel",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
 '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($Routes){$probeArgs+=('-CalmyTraversalRoutes="'+(Resolve-Path $Routes)+'"');$probeArgs+='-CalmyTraversalChanges=none'}
$proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $probeArgs -WindowStyle Hidden -PassThru
Write-Output "Evidence: $dir"
try {
 if(-not $proc.WaitForExit(1900000)){throw "Traversal audit timed out: $dir"}
 $done=Select-String -LiteralPath $log -Pattern 'CALMY_TRAVERSAL (DONE|FAIL)' | Select-Object -Last 1
 if(-not $done){throw "Traversal audit did not finish: $dir"}
 Write-Output $done.Line
 if($done.Line -match 'CALMY_TRAVERSAL FAIL'){exit 1}
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue}}
