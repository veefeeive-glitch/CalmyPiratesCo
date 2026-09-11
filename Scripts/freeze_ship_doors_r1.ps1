$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/ShipDoors-R1-20260908'
if(Test-Path -LiteralPath $dest){throw 'Do not overwrite frozen evidence'}
$runs=@(@('20260908-104600-Door','CALMY_DOOR'),@('20260908-104612-Ship','CALMY_SHIP'),@('20260908-104912-Interaction','CALMY_INTERACTION'))
$passes=@()
foreach($run in $runs){foreach($role in @('Host','Client','LateJoin')){
    $log=Join-Path $root "Saved/Verification/$($run[0])/$role.log"
    $pass=Select-String -LiteralPath $log -Pattern "$($run[1]) $role PASS"
    if(-not $pass -or (Select-String -LiteralPath $log -Pattern "$($run[1]) .* FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:" -Quiet)){throw "Invalid proof $log"}
    $passes+=$pass.Line
}}
$gpu='ShipR7-Visual-20260908-104933'
foreach($pair in @(@("Saved/Verification/$gpu/Visual.log",'CALMY_SHIP Visual PASS'),@('Saved/Verification/Doors-R1/Visual.log','CALMY_DOOR Visual PASS'))){
    $log=Join-Path $root $pair[0]
    if(-not(Select-String -LiteralPath $log -Pattern $pair[1] -Quiet) -or (Select-String -LiteralPath $log -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|CALMY_.* FAIL' -Quiet)){throw "GPU proof failed $log"}
}
$automation='Saved/Logs/Automation-20260908-104923.log'
$results=@(Select-String -LiteralPath (Join-Path $root $automation) -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 27 -or @($results | Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation proof failed'}
if((Get-FileHash "$root/Content/Maps/OceanFreighterR7.umap").Hash -ne '1d7f0eb314393f0a888329b29cb81a7b6eebeea2b873f7f6d373abba19668691'){throw 'Map changed after tests'}
if((Get-FileHash "$root/Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll").Hash -ne '6f29e6edf36755a2e3fbd0e9b53c8384ab526a76b2b167947dce8e376d3ff648'){throw 'DLL changed after tests'}
if((Get-FileHash "$root/SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend").Hash -ne '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'){throw 'Master changed'}
$unchanged=0
foreach($spec in @(@('ShipR7-Integration-20260906','Snapshot/Content/',9),@('ShipMetal-R1-20260906','Delta/Content/Ships/',6),@('EngineDream-R1-20260908','Delta/Content/Ships/',6))){
    foreach($entry in (Get-Content "$root/Evidence/Gameplay/$($spec[0])/manifest.json" -Raw | ConvertFrom-Json) | Where-Object {$_.path.StartsWith($spec[1]) -and $_.path -notlike '*/Maps/OceanFreighterR7.umap'}){
        if((Get-FileHash -LiteralPath (Join-Path $root $entry.path.Substring($spec[2]))).Hash -ne $entry.sha256){throw "Baseline asset changed: $($entry.path)"}
        $unchanged++
    }
}
New-Item -ItemType Directory -Path $dest | Out-Null
function Save-Proof([string]$source,[string]$relative){
    $target=Join-Path $dest $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
Save-Proof 'Source' 'Delta/Source'
Save-Proof 'Content/Maps/OceanFreighterR7.umap' 'Delta/Content/Maps/OceanFreighterR7.umap'
Save-Proof 'Content/Ships/OceanFreighterR7/DoorsR1' 'Delta/Content/Ships/OceanFreighterR7/DoorsR1'
Save-Proof 'SourceAssets/Generated/ShipDoorsR1' 'Delta/SourceAssets/Generated/ShipDoorsR1'
foreach($name in @('audit_r7_doors.py','export_r7_doors.py','import_r7_doors.py','fit_r7_door_swings.py','apply_r7_runtime_swings.py','run_network_smoke.ps1','freeze_ship_doors_r1.ps1')){Save-Proof "Scripts/$name" "Delta/Scripts/$name"}
Save-Proof 'Saved/Verification/Doors-R1' 'DoorAudit-And-Diagnostics'
Save-Proof "Saved/Verification/$gpu" 'GPU'
foreach($run in $runs){Save-Proof "Saved/Verification/$($run[0])" "Network/$($run[0])"}
Save-Proof $automation 'Automation.log'
Save-Proof 'Docs/Changes/SHIP_DOORS_R1_2026-09-08.md' 'Report.md'
Save-Proof 'Docs/Current' 'Delta/Docs/Current'
$record=[ordered]@{
    revision='ShipDoors-R1-20260908';scope='61 passage doors, not cupboards or hatches';required_baselines=@('ShipR7-Integration-20260906','ShipMetal-R1-20260906','EngineDream-R1-20260908')
    automation='27/27';network_pass_markers=$passes;prior_content_unchanged=$unchanged
    door_swing='61/61 closed then reopened to 180 degrees with collision enabled in game'
    rpc='Owning-client E/revision/replay tested on explicit transient fixture; ship doors state replicated'
    gpu='7 ship route/camera views plus 2 close-up door states, inspected';ship_traversal='Host/Client 37 waypoints each; LateJoin scene/spawn only'
    all_61_thresholds_walked=$false;human_acceptance=$false;performance_measured=$false;packaged_build=$false;npc_opens_doors=$false
    master_changed=$false;static_superstructure_changed=$true;map_sha256=(Get-FileHash "$root/Content/Maps/OceanFreighterR7.umap").Hash.ToLower()
    dll_sha256=(Get-FileHash "$root/Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll").Hash.ToLower()
    master_sha256=(Get-FileHash "$root/SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend").Hash.ToLower()
}
$record | ConvertTo-Json -Depth 6 | Set-Content "$dest/acceptance.json" -Encoding utf8
$hashes=@(Get-ChildItem $dest -File -Recurse | ForEach-Object {@{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash $_.FullName).Hash.ToLower()}})
$hashes | ConvertTo-Json -Depth 4 | Set-Content "$dest/manifest.json" -Encoding utf8
foreach($entry in (Get-Content "$dest/manifest.json" -Raw | ConvertFrom-Json)){if((Get-FileHash (Join-Path $dest $entry.path)).Hash -ne $entry.sha256){throw 'Evidence mismatch'}}
"DOORS_EVIDENCE_PASS files=$($hashes.Count) prior_assets_unchanged=$unchanged mismatches=0"
