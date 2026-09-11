param([Parameter(Mandatory=$true)][string]$PackagedExe)
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$gameExe=(Resolve-Path -LiteralPath $PackagedExe).Path
$reviewRoot=Join-Path $projectRoot ('Saved/Verification/Package-Share-Review-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $reviewRoot | Out-Null
Write-Output "Evidence: $reviewRoot"
foreach($scenario in @('Tutorial','Settings')) {
    $scenarioRoot=Join-Path $reviewRoot $scenario
    New-Item -ItemType Directory -Path $scenarioRoot | Out-Null
    $gameLog=Join-Path $scenarioRoot 'Game.log'
    $map=if($scenario -eq 'Tutorial'){'/Game/Maps/WaterLab'}else{'/Game/Maps/OceanFreighterR7'}
    $marker=if($scenario -eq 'Tutorial'){'CALMY_TUTORIAL_TEST Visual PASS'}else{'CALMY_SETTINGS_TEST Visual PASS'}
    $launchArgs=@($map,'-game','-unattended','-windowed','-ResX=1280','-ResY=800','-MULTIHOME=127.0.0.1',
        ('-UserDir="'+$scenarioRoot+'/Profile/"'),('-abslog="'+$gameLog+'"'),
        ('-Calmy'+$scenario+'Probe=Visual'),'-CalmyReviewDir=Screenshots','-ExecCmds="t.MaxFPS 60"')
    $gameProcess=Start-Process -FilePath $gameExe -ArgumentList $launchArgs -WindowStyle Hidden -PassThru
    try {
        if(-not $gameProcess.WaitForExit(240000)){throw "$scenario packaged review timed out"}
        if($gameProcess.ExitCode -ne 0 -or -not(Select-String -LiteralPath $gameLog -Pattern $marker -Quiet) -or
            (Select-String -LiteralPath $gameLog -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Failed to load package|SkipPackage: /Game/|Failed to find object.* /Game/' -Quiet)) {
            throw "$scenario packaged review failed: $gameLog"
        }
        $screenshots=@(Get-ChildItem -LiteralPath $scenarioRoot -Recurse -Filter '*.png')
        $minimum=if($scenario -eq 'Tutorial'){5}else{7}
        if($screenshots.Count -lt $minimum){throw "$scenario screenshots missing"}
        (Select-String -LiteralPath $gameLog -Pattern $marker).Line
        "PACKAGED_REVIEW_PASS $scenario screenshots=$($screenshots.Count)"
    } finally {
        if(-not $gameProcess.HasExited){Stop-Process -Id $gameProcess.Id -ErrorAction SilentlyContinue}
    }
}
