$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/Crew-R6-20260908'
if(Test-Path -LiteralPath $dest){throw 'Frozen destination already exists'}
$suites=@{
 Character='20260908-155905-Character'; Helper='20260908-160526-Helper';
 Ship='20260908-160537-Ship'; Water='20260908-161328-Water'; Pause='20260908-161339-Pause'
}
$markers=@{Character='CALMY_CHARACTER';Helper='CALMY_HELPER';Ship='CALMY_SHIP';Water='CALMY_WATER';Pause='CALMY_PAUSE_TEST'}
$passLines=@()
foreach($suite in $suites.Keys){
 foreach($role in @('Host','Client','LateJoin')){
  $log=Join-Path $root ('Saved/Verification/'+$suites[$suite]+'/'+$role+'.log')
  $found=@(Select-String -LiteralPath $log -Pattern ($markers[$suite]+' '+$role+' PASS'))
  if($found.Count -ne 1){throw "Missing unique PASS: $suite $role"}
  if(Select-String -LiteralPath $log -Pattern 'Fatal error|Assertion failed:|Ensure condition failed:|CALMY_\w+ \w+ FAIL' -Quiet){throw "Failed run: $log"}
  $passLines+=$found[0].Line
 }
}
$auto=Join-Path $root 'Saved/Logs/Automation-20260908-155854.log'
$results=@(Select-String -LiteralPath $auto -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 28 -or @($results | Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation does not pass'}
$maps=@{
 'Content/Maps/OceanFreighterR7.umap'='9804dcd7af9d9742006b34951cad355ce1caf89269a01cb366e40e1786c8f03a';
 'Content/Maps/WaterLab.umap'='0122f4aca4ec281214979814e024fd3fb5a430856e284eed778d1a9f911ca180'
}
foreach($path in $maps.Keys){if((Get-FileHash -LiteralPath (Join-Path $root $path)).Hash -ne $maps[$path]){throw "Map changed: $path"}}
$dll=Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll'
if((Get-FileHash -LiteralPath $dll).Hash -ne 'ce3b270d3b58f16e6efd2b6c4fb842f2667319a97afda3147735fed32fcb7ef3'){throw 'Binary changed after tests'}
$baselines=@('ShipR7-Integration-20260906','ShipMetal-R1-20260906','EngineDream-R1-20260908','ShipDoors-R1-20260908','Nightwatch-R1-20260908')
$preserved=@()
foreach($baseline in $baselines){
 $dir=Join-Path $root ('Evidence/Gameplay/'+$baseline)
 $entries=Get-Content -LiteralPath (Join-Path $dir 'manifest.json') -Raw | ConvertFrom-Json
 foreach($entry in $entries){if((Get-FileHash -LiteralPath (Join-Path $dir $entry.path)).Hash -ne $entry.sha256){throw "Changed frozen evidence $baseline/$($entry.path)"}}
 $preserved+=@{name=$baseline;entries=@($entries).Count;unchanged=$true}
}
New-Item -ItemType Directory -Path $dest | Out-Null
function CopyEvidence($source,$relative){
 $target=Join-Path $dest $relative
 New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
 Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
foreach($suite in $suites.Keys){CopyEvidence ('Saved/Verification/'+$suites[$suite]) ('Network/'+$suite)}
CopyEvidence 'Saved/Verification/LP4-Visual-20260908-155714' 'GPU-Final/Crew'
CopyEvidence 'Saved/Verification/ShipR7-Visual-20260908-160739' 'GPU-Final/Ship'
CopyEvidence 'Saved/Verification/LP4-Visual-20260908-153917' 'Diagnostics/GPU-Round1'
CopyEvidence 'Saved/Verification/CrewR6-20260908/Import-Round1.log' 'Diagnostics/Import-Round1-exit3.log'
CopyEvidence 'Saved/Verification/CrewR6-20260908/Import-Final.log' 'Diagnostics/Import-Final-exit3.log'
CopyEvidence 'Saved/Verification/CrewR6-20260908/Build-Final.log' 'Build.log'
CopyEvidence 'Saved/Logs/Automation-20260908-155854.log' 'Automation.log'
CopyEvidence 'Saved/Logs/Helper-Solo-20260908-160628.log' 'HelperSolo.log'
CopyEvidence 'SourceAssets/Generated/CrewR6' 'Blender-Round1'
CopyEvidence 'SourceAssets/Generated/CrewR6B' 'Blender-Final'
CopyEvidence 'SourceAssets/References/CrewR6' 'Concept-not-gameplay'
CopyEvidence 'Content/Characters/CrewR6B' 'RuntimeAssets/CrewR6B'
CopyEvidence 'Source' 'Source'
CopyEvidence 'Docs/Changes/CREW_R6_2026-09-08.md' 'Report.md'
CopyEvidence 'Scripts/create_crew_r6.py' 'Scripts/create_crew_r6.py'
CopyEvidence 'Scripts/import_crew_r6.py' 'Scripts/import_crew_r6.py'
CopyEvidence 'Scripts/freeze_crew_r6_evidence.ps1' 'Scripts/freeze_crew_r6_evidence.ps1'
CopyEvidence 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll' 'Binary/UnrealEditor-CalmyPiratesCo.dll'
foreach($path in $maps.Keys){CopyEvidence $path ('RuntimeAssets/Maps/'+(Split-Path -Leaf $path))}
$snapshotPaths=@('Source','Content/Characters/CrewR6B','SourceAssets/Generated/CrewR6B')
$sourceHashes=@(foreach($path in $snapshotPaths){Get-ChildItem -LiteralPath (Join-Path $root $path) -File -Recurse | ForEach-Object {
 @{path=$_.FullName.Substring($root.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}
}})
$sourceHashes | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $dest 'source-hashes.json') -Encoding UTF8
$acceptance=@{
 revision='Crew-R6B-20260908';automation='28/28';binary_sha256=(Get-FileHash -LiteralPath $dll).Hash.ToLower();
 network_pass_lines=$passLines;maps_unchanged=$maps;preserved_baselines=$preserved;
 triangles=@{Deckhand=4802;Misha=4882};max_ship_joint_cm=7.812;ship_joint_limit_cm=8;
 import_processes='Both saved 20 assets, then shutdown SemanticSearch crash exit3. Not clean import PASS.';
 runtime_reload='Clean automation and GPU runs passed against /Game/Characters/CrewR6B.';
 inherited_unfinished='BridgeMenu draft is preserved, not accepted; Pause tests cover WaterLab panel only.';
 human_acceptance=$false;packaged_build=$false;fps_measured=$false;
 remaining=@('Rigid shoulder/elbow seams; skinned surface required','Dark fronts/trousers/NPC text','Convincing finger wrap not proven','Thin Ship joint margin; long soak needed','Clean editor import shutdown unresolved')
}
$acceptance | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $dest 'acceptance.json') -Encoding UTF8
$manifest=@(Get-ChildItem -LiteralPath $dest -File -Recurse | Sort-Object FullName | ForEach-Object {
 @{path=$_.FullName.Substring($dest.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}
})
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $dest 'manifest.json') -Encoding UTF8
"CREW_R6_FROZEN $dest files=$($manifest.Count) network_roles=$($passLines.Count)"
