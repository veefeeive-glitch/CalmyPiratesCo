$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/EngineDream-R1-20260908'
if(Test-Path -LiteralPath $dest){throw 'Do not overwrite frozen evidence'}
$gpu=@('ShipR7-Visual-20260908-095007','ShipR7-Visual-20260908-095948')
$net=@('20260908-095106-Ship','20260908-100137-Ship')
foreach($run in $gpu){
    $d=Join-Path $root "Saved/Verification/$run"
    if(-not(Select-String "$d/Visual.log" -Pattern 'CALMY_SHIP Visual PASS' -Quiet)){throw 'Missing GPU pass'}
    if(Select-String "$d/Visual.log" -Pattern 'CALMY_SHIP Visual FAIL|Fatal error:|Assertion failed:|Ensure condition failed:' -Quiet){throw 'GPU error'}
    if(@(Get-ChildItem $d -Filter '*.png').Count -ne 7){throw 'Missing frames'}
}
foreach($run in $net){foreach($role in @('Host','Client','LateJoin')){
    $log=Join-Path $root "Saved/Verification/$run/$role.log"
    if(-not(Select-String $log -Pattern "CALMY_SHIP $role PASS" -Quiet)){throw 'Missing network pass'}
    if(Select-String $log -Pattern "CALMY_SHIP $role FAIL|Fatal error:|Assertion failed:|Ensure condition failed:" -Quiet){throw 'Network error'}
}}
$unchanged=0
foreach($spec in @(@('ShipR7-Integration-20260906','Snapshot/Content/',9),@('ShipMetal-R1-20260906','Delta/Content/Ships/',6))){
    $manifest=Get-Content "$root/Evidence/Gameplay/$($spec[0])/manifest.json" -Raw | ConvertFrom-Json
    foreach($entry in $manifest | Where-Object {$_.path.StartsWith($spec[1]) -and $_.path -notlike '*/Maps/OceanFreighterR7.umap'}){
        if((Get-FileHash -LiteralPath (Join-Path $root $entry.path.Substring($spec[2]))).Hash -ne $entry.sha256){throw "Previous asset changed: $($entry.path)"}
        $unchanged++
    }
}
New-Item -ItemType Directory -Path $dest | Out-Null
function Save-Proof([string]$source,[string]$relative){
    $target=Join-Path $dest $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
Save-Proof 'Saved/Verification/EngineDream-R1-20260908' 'Audit'
Save-Proof 'SourceAssets/References/EngineDream-R1-20260908' 'Concept-not-gameplay'
for($i=0;$i -lt 2;$i++){
    Save-Proof "Saved/Verification/$($gpu[$i])" "Round$($i+1)/GPU"
    Save-Proof "Saved/Verification/$($net[$i])" "Round$($i+1)/Network"
}
Save-Proof 'Evidence/Gameplay/ShipMetal-R1-20260906/GPU/07-engine-camera-only.png' 'Before/07-engine-camera-only.png'
Save-Proof 'Content/Maps/OceanFreighterR7.umap' 'Delta/Content/Maps/OceanFreighterR7.umap'
foreach($name in @('EngineDreamR1','EngineDreamR2')){Save-Proof "Content/Ships/OceanFreighterR7/$name" "Delta/Content/Ships/OceanFreighterR7/$name"}
foreach($name in @('audit_engine_dream_r1.py','audit_engine_dream_blender.py','apply_engine_dream_r1.py','apply_engine_dream_r2.py','freeze_engine_dream_r1.ps1')){Save-Proof "Scripts/$name" "Delta/Scripts/$name"}
Save-Proof 'Docs/Changes/ENGINE_DREAM_LOOP_R1_2026-09-08.md' 'Report.md'
foreach($name in @('EngineDream-R1-Apply.log','EngineDream-R1-ApplyB.log','EngineDream-R2-Apply.log','EngineDream-Params.log')){Save-Proof "Saved/Logs/$name" "Diagnostics/$name"}
$record=[ordered]@{
    revision='EngineDream-R1-20260908';scope='Two-round light/material-only delta, not complete ship or standalone snapshot'
    required_baselines=@('ShipR7-Integration-20260906','ShipMetal-R1-20260906')
    old_assets_hash_verified_unchanged=$unchanged;new_material_assets=5;component_material_overrides=3;local_postprocess_volumes=1
    gpu='Both rounds PASS, 37 real walking waypoints; seven images per round include three staged cameras'
    network='Both rounds Host/Client route PASS; LateJoin scene/spawn only PASS'
    critic='Independent read-only baseline and both candidates; neutral deck landed, light/glare partial'
    remaining=@('Engine floor/underside hotspots','Wheel/gauge glare','Pale pump paint','Multiple close engine-room walking views','Existing bridge washout','Moving ship needs local-space tint')
    automation_rerun=$false;fps_measured=$false;human_acceptance=$false;blender_master_changed=$false;geometry_changed=$false
    map_sha256=(Get-FileHash "$root/Content/Maps/OceanFreighterR7.umap").Hash.ToLower()
    master_sha256=(Get-FileHash "$root/SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend").Hash.ToLower()
    dll_sha256=(Get-FileHash "$root/Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll").Hash.ToLower()
}
$record | ConvertTo-Json -Depth 5 | Set-Content "$dest/acceptance.json" -Encoding utf8
$hashes=@(Get-ChildItem $dest -File -Recurse | ForEach-Object {@{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash $_.FullName).Hash.ToLower()}})
$hashes | ConvertTo-Json -Depth 4 | Set-Content "$dest/manifest.json" -Encoding utf8
foreach($entry in (Get-Content "$dest/manifest.json" -Raw | ConvertFrom-Json)){if((Get-FileHash (Join-Path $dest $entry.path)).Hash -ne $entry.sha256){throw 'Evidence mismatch'}}
"ENGINE_DREAM_EVIDENCE_PASS files=$($hashes.Count) mismatches=0 prior_assets_unchanged=$unchanged"
