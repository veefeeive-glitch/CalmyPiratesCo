param([switch]$InteractionHUD,[switch]$CrewLowPoly,[switch]$GripViews)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label=$(if($CrewLowPoly){'LP4-Visual-'}elseif($InteractionHUD){'G1-Visual-'}else{'R3-Visual-'})+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Visual.log'
$reviewArgs=@(('"'+$root+'\CalmyPiratesCo.uproject"'),'/Game/Maps/WaterLab','-game','-unattended','-NoSound','-DisablePython',
 '-windowed','-ResX=1280','-ResY=800','-CalmyR3Review',('-CalmyReviewDir='+$label),('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"',
 '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($InteractionHUD){$reviewArgs+='-CalmyG1HUD'}
if($CrewLowPoly){$reviewArgs+='-CalmyLP4Views'}
if($GripViews){$reviewArgs+='-CalmyGripViews'}
$proc=Start-Process 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $reviewArgs -WindowStyle Hidden -PassThru
try {
 Write-Output "Evidence: $dir"
 if(-not $proc.WaitForExit(150000)){throw "R3 review timed out: $dir"}
 if($proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'R3_REVIEW PASS' -Quiet) -or
   (Select-String -LiteralPath $log -Pattern 'R3_REVIEW FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)) {throw "R3 review failed: $dir"}
 $expected=if($InteractionHUD){7}else{6}
 if($CrewLowPoly){$expected+=2}
 if($GripViews){$expected+=2}
 if(@(Get-ChildItem -LiteralPath $dir -Filter '*.png').Count -ne $expected){throw "Expected $expected screenshots"}
 Write-Output "R3_REVIEW_PASS $dir (render fixture, not traversal or human acceptance)"
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue}}
