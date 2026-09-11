param([int]$Width=1280,[int]$Height=800)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$label='Settings-S1-'+(Get-Date -Format 'yyyyMMdd-HHmmss')
$evidencePath=Join-Path $projectRoot ('Saved/Verification/'+$label)
New-Item -ItemType Directory -Path $evidencePath | Out-Null
$log=Join-Path $evidencePath 'Visual.log'
$argsList=@(('"'+$projectRoot+'/CalmyPiratesCo.uproject"'),'/Game/Maps/OceanFreighterR7','-game','-unattended','-DisablePython','-windowed',"-ResX=$Width","-ResY=$Height",'-CalmySettingsProbe=Visual',"-CalmyReviewDir=$label",('-abslog="'+$log+'"'),'-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
$testProcess=Start-Process 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsList -WindowStyle Hidden -PassThru
Write-Output "Evidence: $evidencePath"
try {
    if(-not $testProcess.WaitForExit(160000)){throw 'Settings review timeout'}
    if($testProcess.ExitCode -ne 0 -or -not(Select-String -LiteralPath $log -Pattern 'CALMY_SETTINGS_TEST Visual PASS' -Quiet)){throw "Settings review failed: $evidencePath"}
    if(@(Get-ChildItem -LiteralPath $evidencePath -Filter '*.png').Count -ne 7){throw 'Missing settings screenshots'}
    (Select-String -LiteralPath $log -Pattern 'CALMY_SETTINGS_TEST Visual PASS').Line
    'SETTINGS_REVIEW_PASS'
} finally {if(-not $testProcess.HasExited){Stop-Process -Id $testProcess.Id -ErrorAction SilentlyContinue}}
