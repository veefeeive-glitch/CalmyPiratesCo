param([int]$Width=1280,[int]$Height=800)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$label='BridgeMenu-Visual-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$dir=Join-Path $root ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $dir | Out-Null
$log=Join-Path $dir 'Visual.log'
$argsList=@(('"'+$root+'/CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-unattended','-NoSound','-DisablePython','-windowed',"-ResX=$Width","-ResY=$Height",'-CalmyBridgeMenuProbe=Visual',"-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ExecCmds="t.MaxFPS 60"','-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$proc=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsList -WindowStyle Hidden -PassThru
try {
    Write-Output "Evidence: $dir"
    if(-not $proc.WaitForExit(150000)){throw 'Bridge menu review timed out'}
    if($proc.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_BRIDGE_MENU Visual PASS' -Quiet) -or (Select-String -LiteralPath $log -Pattern 'CALMY_BRIDGE_MENU .* FAIL|Fatal error:|Ensure condition failed:|Assertion failed:' -Quiet)){throw "Bridge review failed: $dir"}
    if(@(Get-ChildItem $dir -Filter '*.png').Count -ne 3){throw 'Missing bridge screenshots'}
    (Select-String -LiteralPath $log -Pattern 'CALMY_BRIDGE_MENU Visual PASS').Line
    'BRIDGE_MENU_REVIEW_PASS'
} finally {if(-not $proc.HasExited){Stop-Process -Id $proc.Id}}
