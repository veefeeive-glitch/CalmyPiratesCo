# One-shot immutable evidence archive. Refuses overwrite and incomplete final suites.
$ErrorActionPreference='Stop'
$projectRoot=Split-Path -Parent $PSScriptRoot
$archivePath=Join-Path $projectRoot 'Evidence/Gameplay/CrewHelper-R2-20260906'
if(Test-Path -LiteralPath $archivePath) { throw "Archive already exists: $archivePath" }
$modelPath=Join-Path $projectRoot 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
$modelHash=(Get-FileHash -LiteralPath $modelPath -Algorithm SHA256).Hash.ToLowerInvariant()
if($modelHash -ne '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93') { throw 'Unexpected ship revision' }
$npcPath=Join-Path $projectRoot 'Saved/OceanPhysics/R7-NPC-20260906-03.json'
$walkPath=Join-Path $projectRoot 'Saved/OceanPhysics/R7-Walk-CrewR2-20260906.json'
$npc=Get-Content -LiteralPath $npcPath -Raw | ConvertFrom-Json
$walk=Get-Content -LiteralPath $walkPath -Raw | ConvertFrom-Json
$baseline=Get-Content -LiteralPath (Join-Path $projectRoot 'Evidence/ShipTraversal/R7-Cargo-20260906/physical-report.json') -Raw | ConvertFrom-Json
if(-not $npc.npc_audit -or $npc.passed -ne 56 -or $npc.total -ne 56 -or $npc.controls_passed -ne 4 -or $npc.source_sha256 -ne $modelHash) { throw 'NPC acceptance failed' }
$differences=@(Compare-Object @($baseline.walks | ForEach-Object {$_.name+':'+$_.pass}) @($walk.walks | ForEach-Object {$_.name+':'+$_.pass}))
if($walk.source_sha256 -ne $modelHash -or $walk.passed -ne 293 -or $walk.total -ne 336 -or $differences.Count) { throw 'Walking baseline differs; inspect rather than silently accept' }
$geometry=Get-Content -LiteralPath (Join-Path $projectRoot 'Evidence/ShipTraversal/R7-Cargo-20260906/geometry-routes.json') -Raw | ConvertFrom-Json
$required=[System.Collections.Generic.HashSet[string]]::new()
foreach($route in $geometry.routes) { [void]$required.Add($route.name+'_forward');[void]$required.Add($route.name+'_reverse') }
foreach($route in $walk.walks) {
    if($route.name -match '^(AUDIT_exterior_tower_to_roof_|AUDIT_accommodation_to_weather_|AUDIT_usable_|AUDIT_mess_to_bridge|AUDIT_weather_to_stern_engine_|AUDIT_weather_to_hold_|AUDIT_upper_exit_|AUDIT_bridge_watch)') { [void]$required.Add($route.name) }
}
if($required.Count -ne 260 -or @($walk.walks | Where-Object {$required.Contains($_.name) -and $_.pass}).Count -ne 260) {throw 'Required walking intent failed'}
$network=@()
foreach($suite in @('Helper','Character','Cargo','Carry','Water','Shift','Leak','Interior')) {
    $dir=Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved/Verification') -Directory |
        Where-Object {$_.Name -match ('^20260906-\d{6}-'+$suite+'$')} | Sort-Object Name -Descending | Select-Object -First 1
    if(-not $dir) { throw "Missing suite $suite" }
    $marker=switch($suite) {'Carry' {'CALMY_PROBE'} default {'CALMY_'+$suite.ToUpperInvariant()}}
    $lines=@()
    foreach($role in @('Host','Client','LateJoin')) {
        $log=Join-Path $dir.FullName ($role+'.log')
        $pass=Select-String -LiteralPath $log -Pattern ($marker+' '+$role+' PASS')
        if(-not $pass -or (Select-String -LiteralPath $log -Pattern 'CALMY_\w+ \w+ FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)) { throw "Incomplete or failed $suite/$role" }
        $lines+=$pass.Line
    }
    $network+=@{suite=$suite;path=$dir.FullName;passed_roles=3;lines=$lines}
}
$automation=Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved/Logs') -Filter 'Automation-20260906-*.log' | Sort-Object Name -Descending | Select-Object -First 1
$testLines=@(Select-String -LiteralPath $automation.FullName -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($testLines.Count -ne 20 -or @($testLines | Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count) {throw '20/20 automation required'}
$solo=Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved/Logs') -Filter 'Helper-Solo-20260906-*.log' | Sort-Object Name -Descending | Select-Object -First 1
if(-not $solo -or -not (Select-String -LiteralPath $solo.FullName -Pattern 'CALMY_HELPER Solo PASS' -Quiet)) { throw 'Missing final solo acceptance' }
$visualLog=Join-Path $projectRoot 'Saved/Deckhand-R2-Visual-02.log'
if(-not (Select-String -LiteralPath $visualLog -Pattern 'CALMY_CHARACTER Visual PASS' -Quiet)) { throw 'Missing visual acceptance' }

New-Item -ItemType Directory -Path $archivePath | Out-Null
function Copy-Evidence([string]$From,[string]$Relative) {
    $target=Join-Path $archivePath $Relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $From -Destination $target
}
foreach($folder in @('Source','Config','Scripts')) {
    Get-ChildItem -LiteralPath (Join-Path $projectRoot $folder) -File -Recurse |
        Where-Object {$_.Extension -in @('.h','.cpp','.cs','.ini','.py','.ps1','.json')} | ForEach-Object {
            Copy-Evidence $_.FullName ('source-snapshot/'+[IO.Path]::GetRelativePath($projectRoot,$_.FullName))
        }
}
foreach($file in @('CalmyPiratesCo.uproject','AGENTS.md','CREW_HELPER_R2_HANDOVER.md','GAME_ROADMAP.md','SHIP_ROADMAP.md','OCEAN_FREIGHTER_TRAVERSAL_AUDIT.md','README.md','Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll')) {
    Copy-Evidence (Join-Path $projectRoot $file) ('source-snapshot/'+$file)
}
foreach($item in $network) {
    foreach($role in @('Host','Client','LateJoin')) { Copy-Evidence (Join-Path $item.path ($role+'.log')) ('network/'+$item.suite+'/'+$role+'.log') }
}
Copy-Evidence $automation.FullName 'automation.log'
Copy-Evidence $solo.FullName 'solo.log'
Copy-Evidence $npcPath 'npc-ship-report.json'
Copy-Evidence $walkPath 'walking-report.json'
Copy-Evidence (Join-Path $projectRoot 'Saved/OceanPhysics/R7-NPC-20260906-03.log') 'npc-ship.log'
Copy-Evidence (Join-Path $projectRoot 'Saved/OceanPhysics/R7-Walk-CrewR2-20260906.log') 'walking.log'
Copy-Evidence $visualLog 'visual.log'
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Saved/Verification/Deckhand-R2-20260906-02') -Filter '*.png' | ForEach-Object { Copy-Evidence $_.FullName ('views/'+$_.Name) }
foreach($label in @('20260906-035817-Helper','20260906-041028-Character','20260906-041838-Character','20260906-044805-Cargo','20260906-045157-Cargo')) {
    Get-ChildItem -LiteralPath (Join-Path $projectRoot ('Saved/Verification/'+$label)) -Filter '*.log' | ForEach-Object { Copy-Evidence $_.FullName ('diagnostic-failures/'+$label+'/'+$_.Name) }
}
Copy-Evidence (Join-Path $projectRoot 'Saved/OceanPhysics/R7-NPC-20260906-01.json') 'diagnostic-failures/npc-initial-42-of-56.json'
Copy-Evidence (Join-Path $projectRoot 'Saved/Deckhand-R2-Visual-01.log') 'diagnostic-failures/visual-initial.log'
$assetHashes=@(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Content/Characters/Deckhand') -File -Recurse | ForEach-Object {
    @{path=[IO.Path]::GetRelativePath($projectRoot,$_.FullName);sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
$acceptance=@{
    status='SCOPED_GAMEPLAY_PASS_WITH_OPEN_PRODUCTION_GATES';created_utc=[DateTime]::UtcNow.ToString('o');engine='5.8.2';
    map='/Game/Maps/WaterLab';ship_imported=$false;ship_sha256=$modelHash;ship_baseline_archive='Evidence/ShipTraversal/R7-Cargo-20260906';
    source_note='Final source and Editor DLL captured. GPU views and ship reports precede isolated restart/test-harness changes; body, water, NPC route code unchanged.';
    automation_passed=20;automation_total=20;network=$network;solo_heavy_delivery=$true;
    npc_ship=@{passed=56;total=56;controls=4;scope='13 routes, both directions, Standard/Bulky; leader waits for NPC; isolated initial overlap, physical collision once separated'};
    walking=@{passed=293;total=336;required_passed=260;required_total=260;unchanged_failures=43;outcome_changes=$differences};
    visual=@{reviewed_images=5;maximum_joint_gap_cm=.17;gate_cm=8;final_animation_accepted=$false};assets=$assetHashes;
    not_claimed=@('full_solo_contract','shared_two_handle_physics','dynamic_ship_water','new_packaged_build','human_fun_gate','all_ship_spaces')
}
$acceptance | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $archivePath 'acceptance.json') -Encoding utf8
$files=@(Get-ChildItem -LiteralPath $archivePath -Recurse -File | ForEach-Object {
    @{path=[IO.Path]::GetRelativePath($archivePath,$_.FullName);bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
@{algorithm='SHA256';files=$files} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $archivePath 'evidence-manifest.json') -Encoding utf8
$bad=@($files | Where-Object {(Get-FileHash -LiteralPath (Join-Path $archivePath $_.path) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $_.sha256})
if($bad.Count) {throw 'Archive hash verification failed'}
"ARCHIVE_PASS files=$($files.Count) mismatches=0 path=$archivePath"
