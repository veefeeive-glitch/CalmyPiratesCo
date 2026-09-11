param([Parameter(Mandatory=$true)][string]$ShipGpu,[Parameter(Mandatory=$true)][string]$CrewGpu,[Parameter(Mandatory=$true)][string]$FinalShip)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/Nightwatch-R1-20260908'
if(Test-Path -LiteralPath $dest){throw 'Frozen evidence exists; never overwrite'}
foreach($value in @($ShipGpu,$CrewGpu,$FinalShip)){if($value -notmatch '^[a-zA-Z0-9-]+$'){throw 'Expected a verification directory basename'}}
$runs=@(@('20260908-113854-Character','CALMY_CHARACTER'),@('20260908-115216-Helper','CALMY_HELPER'),@('20260908-115313-Door','CALMY_DOOR'),@('20260908-115226-Ship','CALMY_SHIP'),@('20260908-115429-Water','CALMY_WATER'),@($FinalShip,'CALMY_SHIP'))
$passes=@()
foreach($run in $runs){foreach($role in @('Host','Client','LateJoin')){
    $log=Join-Path $root "Saved/Verification/$($run[0])/$role.log"
    $pass=Select-String -LiteralPath $log -Pattern "$($run[1]) $role PASS"
    if(-not $pass -or (Select-String -LiteralPath $log -Pattern "$($run[1]) .* FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:" -Quiet)){throw "Invalid proof $log"}
    $passes+=$pass.Line
}}
$auto='Saved/Logs/Automation-20260908-115600.log'
$results=@(Select-String -LiteralPath (Join-Path $root $auto) -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 27 -or @($results | Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation proof failed'}
$solo='Saved/Logs/Helper-Solo-20260908-115505.log'
if(-not(Select-String -LiteralPath (Join-Path $root $solo) -Pattern 'CALMY_HELPER Solo PASS' -Quiet)){throw 'Solo helper failed'}
foreach($name in @($ShipGpu,$CrewGpu)){
    $log=Join-Path $root "Saved/Verification/$name/Visual.log"
    if(Select-String -LiteralPath $log -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|CALMY_.* FAIL|\[VSM\].*overflow' -Quiet){throw "GPU defect: $name"}
}
if(-not(Select-String -LiteralPath "$root/Saved/Verification/$ShipGpu/Visual.log" -Pattern 'CALMY_SHIP Visual PASS' -Quiet)){throw 'Ship GPU no PASS'}
if(-not(Select-String -LiteralPath "$root/Saved/Verification/$CrewGpu/Visual.log" -Pattern 'R3_REVIEW PASS views=9' -Quiet)){throw 'Crew GPU no PASS'}
if(@(Get-ChildItem "$root/Saved/Verification/$ShipGpu" -Filter '*.png').Count -ne 7 -or @(Get-ChildItem "$root/Saved/Verification/$CrewGpu" -Filter '*.png').Count -ne 9){throw 'Missing screenshots'}
$maps=Get-Content "$root/Saved/Verification/Nightwatch-R1-20260908/final-map-hashes.json" -Raw | ConvertFrom-Json
foreach($p in $maps.PSObject.Properties){if((Get-FileHash "$root/Content/Maps/$($p.Name).umap").Hash -ne $p.Value){throw 'Map changed after final correction'}}
$dll=(Get-FileHash "$root/Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll").Hash.ToLower()
if($dll -ne '4af0c8e437de2fd89903365797fa55e165f5fb9aa1600a401e31e4efbc9a7b46'){throw 'DLL changed'}
if((Get-FileHash "$root/SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend").Hash -ne '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'){throw 'Ship master changed'}
$unchanged=0
foreach($spec in @(@('ShipR7-Integration-20260906','Snapshot/Content/',9),@('ShipMetal-R1-20260906','Delta/Content/Ships/',6),@('EngineDream-R1-20260908','Delta/Content/Ships/',6),@('ShipDoors-R1-20260908','Delta/Content/Ships/',6))){
    foreach($entry in (Get-Content "$root/Evidence/Gameplay/$($spec[0])/manifest.json" -Raw | ConvertFrom-Json) | Where-Object {$_.path.StartsWith($spec[1]) -and $_.path -notlike '*/Maps/OceanFreighterR7.umap' -and $_.path -notlike '*/Maps/WaterLab.umap'}){
        if((Get-FileHash -LiteralPath (Join-Path $root $entry.path.Substring($spec[2]))).Hash -ne $entry.sha256){throw "Prior content changed $($entry.path)"};$unchanged++
    }
}
New-Item -ItemType Directory -Path $dest | Out-Null
function Save-Proof([string]$source,[string]$relative){
    $target=Join-Path $dest $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
Save-Proof 'Source' 'Delta/Source'
foreach($map in @('WaterLab','OceanFreighterR7')){Save-Proof "Content/Maps/$map.umap" "Delta/Content/Maps/$map.umap"}
foreach($folder in @('Content/Characters/CrewNightwatchR5','Content/Atmosphere/NightwatchR1','SourceAssets/Generated/CrewNightwatchR5','SourceAssets/Generated/NightwatchAudioR1','SourceAssets/References/Nightwatch-R1-20260908')){Save-Proof $folder "Delta/$folder"}
foreach($name in @('create_crew_nightwatch_r5.py','import_crew_nightwatch_r5.py','create_nightwatch_audio.py','apply_nightwatch_r1.py','refine_nightwatch_r1.py','fix_nightwatch_light_overlap.py','fix_nightwatch_sun_disc.py','freeze_nightwatch_r1.ps1','run_ship_r7_review.ps1','run_crew_cargo_r3_review.ps1','run_network_smoke.ps1','run_automation.ps1','run_helper_solo.ps1')){Save-Proof "Scripts/$name" "Delta/Scripts/$name"}
Save-Proof "Saved/Verification/$ShipGpu" 'GPU-Final/Ship'
Save-Proof "Saved/Verification/$CrewGpu" 'GPU-Final/Crew'
foreach($run in $runs){Save-Proof "Saved/Verification/$($run[0])" "Network/$($run[0])"}
Save-Proof $auto 'Automation.log';Save-Proof $solo 'HelperSolo.log'
Save-Proof 'Saved/Verification/Nightwatch-R1-20260908' 'Audit-And-Diagnostics'
foreach($trial in @('ShipR7-Visual-20260908-113550','ShipR7-Visual-20260908-115205','ShipR7-Visual-20260908-120126','LP4-Visual-20260908-113843','LP4-Visual-20260908-115520')){Save-Proof "Saved/Verification/$trial" "Earlier-Trials/$trial"}
Save-Proof 'SourceAssets/Generated/CrewNightwatchR5-Round1/R5_Lineup.png' 'Earlier-Trials/Model-Round1.png'
Save-Proof 'Docs/Changes/NIGHTWATCH_R1_2026-09-08.md' 'Report.md'
Save-Proof 'Docs/Current' 'Delta/Docs/Current'
$record=[ordered]@{revision='Nightwatch-R1-20260908';required_baselines=@('ShipR7-Integration-20260906','ShipMetal-R1-20260906','EngineDream-R1-20260908','ShipDoors-R1-20260908');automation='27/27';network_pass_markers=$passes;solo_helper=$true;prior_content_unchanged=$unchanged;map_sha256=$maps;dll_sha256=$dll;character_triangles=@{Deckhand=3474;Misha=3592};ship_geometry_changed=$false;gpu_ship=$ShipGpu;gpu_crew=$CrewGpu;ship_route='37 waypoints Host/Client; late join scene only';audio_listened=$false;human_acceptance=$false;all_ship_routes=$false;performance_measured=$false;packaged_build=$false;remaining='Runtime shoulders angular, dark swimming hands, lamp hotspots, help-sign contrast, full audiovisual human review'}
$record | ConvertTo-Json -Depth 6 | Set-Content "$dest/acceptance.json" -Encoding utf8
$hashes=@(Get-ChildItem $dest -File -Recurse | ForEach-Object {@{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash $_.FullName).Hash.ToLower()}})
$hashes | ConvertTo-Json -Depth 4 | Set-Content "$dest/manifest.json" -Encoding utf8
foreach($entry in (Get-Content "$dest/manifest.json" -Raw | ConvertFrom-Json)){if((Get-FileHash (Join-Path $dest $entry.path)).Hash -ne $entry.sha256){throw 'Evidence mismatch'}}
"NIGHTWATCH_EVIDENCE_PASS files=$($hashes.Count) prior_assets_unchanged=$unchanged mismatches=0"
