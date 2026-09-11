$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label='ShipR7-Visual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Visual.log'
$reviewArgs=@(('"'+$root+'\CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-unattended','-NoSound','-DisablePython',
 '-windowed','-ResX=1280','-ResY=800','-CalmyShipProbe=Visual',('-CalmyReviewDir='+$label),('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
 '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $reviewArgs -WindowStyle Hidden -PassThru
try {
 Write-Output "Evidence: $dir"
 if(-not $proc.WaitForExit(300000)){throw "Ship review timed out: $dir"}
 if($proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_SHIP Visual PASS' -Quiet) -or
   (Select-String -LiteralPath $log -Pattern 'CALMY_SHIP Visual FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)) {throw "Ship review failed: $dir"}
 if(@(Get-ChildItem -LiteralPath $dir -Filter '*.png').Count -ne 7){throw 'Expected 7 screenshots'}
 (Select-String -LiteralPath $log -Pattern 'CALMY_SHIP Visual PASS').Line | Write-Output
 Write-Output "SHIP_REVIEW_PASS $dir (bounded real-pawn route + camera fixtures, not full ship acceptance)"
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue}}
