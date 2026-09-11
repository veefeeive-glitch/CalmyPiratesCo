# One-shot evidence capture for the verified R2 build. Never replaces an archive.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/PhysicalCrewBridge-R2-20260910'
if(Test-Path -LiteralPath $dest){throw 'Frozen destination already exists'}
$expectedDll='dec6d435f3e56eba61fbf58320543e9525357eaa049278d0ace5ed419bb82ed5'
$dll=Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll'
if((Get-FileHash -LiteralPath $dll).Hash -ne $expectedDll){throw 'Binary changed after the R2 tests'}
$suites=@{
 Character='20260910-012546-Character';BridgeMenu='20260910-012546-BridgeMenu';
 Ship='20260910-012845-Ship';Helper='20260910-012846-Helper';Water='20260910-012846-Water';Pause='20260910-012847-Pause'
}
$markers=@{Character='CALMY_CHARACTER';BridgeMenu='CALMY_BRIDGE_MENU';Ship='CALMY_SHIP';Helper='CALMY_HELPER';Water='CALMY_WATER';Pause='CALMY_PAUSE_TEST'}
$passes=@()
function CheckLog([string]$path,[string]$pattern){
 $full=Join-Path $root $path
 $found=@(Select-String -LiteralPath $full -Pattern $pattern)
 if($found.Count -ne 1){throw "Missing unique PASS: $path"}
 if(Select-String -LiteralPath $full -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|CALMY_\w+ \w+ FAIL|R3_REVIEW FAIL' -Quiet){throw "Failed evidence: $path"}
 return $found[0].Line
}
foreach($suite in $suites.Keys){foreach($role in @('Host','Client','LateJoin')){
 $passes+=CheckLog ('Saved/Verification/'+$suites[$suite]+'/'+$role+'.log') ($markers[$suite]+' '+$role+' PASS')
}}
$gpu=@{
 PhysicalCrew=@('PhysicalCrew-Visual-20260910-012418','CALMY_CHARACTER Visual PASS',6)
 Bridge=@('BridgeMenu-Visual-20260910-012545','CALMY_BRIDGE_MENU Visual PASS',3)
 Grip=@('LP4-Visual-20260910-013505','R3_REVIEW PASS',10)
}
foreach($group in $gpu.Keys){
 $entry=$gpu[$group];$passes+=CheckLog ('Saved/Verification/'+$entry[0]+'/Visual.log') $entry[1]
 if(@(Get-ChildItem -LiteralPath (Join-Path $root ('Saved/Verification/'+$entry[0])) -Filter '*.png').Count -ne $entry[2]){throw "Missing GPU frames: $group"}
}
$auto='Saved/Logs/Automation-20260910-012546.log'
$solo='Saved/Logs/Helper-Solo-20260910-013516.log'
$passes+=CheckLog $solo 'CALMY_HELPER Solo PASS'
$results=@(Select-String -LiteralPath (Join-Path $root $auto) -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 30 -or @($results|Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation not 30/30'}
New-Item -ItemType Directory -Path $dest | Out-Null
function CopyEvidence([string]$source,[string]$relative){
 $target=Join-Path $dest $relative
 New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
 Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
foreach($suite in $suites.Keys){CopyEvidence ('Saved/Verification/'+$suites[$suite]) ('Network/'+$suite)}
foreach($group in $gpu.Keys){CopyEvidence ('Saved/Verification/'+$gpu[$group][0]) ('GPU/'+$group)}
CopyEvidence $auto 'Automation.log'
CopyEvidence $solo 'HelperSolo.log'
CopyEvidence 'Saved/Verification/PhysicalCrew-R2-Build.log' 'Build.log'
CopyEvidence 'Source' 'Source'
CopyEvidence 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll' 'Binary/UnrealEditor-CalmyPiratesCo.dll'
CopyEvidence 'Docs/Changes/PHYSICAL_CREW_BRIDGE_R2_2026-09-10.md' 'Report.md'
foreach($name in @('Project.ps1','start_game.ps1','run_physical_crew_review.ps1','run_bridge_menu_review.ps1','freeze_physical_crew_r2.ps1')){CopyEvidence ('Scripts/'+$name) ('Scripts/'+$name)}
foreach($name in @('20260910-004259-Character/Host.log','PhysicalCrew-Visual-20260910-011634/Visual.log','PhysicalCrew-Visual-20260910-012045/Visual.log','BridgeMenu-Visual-20260910-004533/01-terminal.png','20260910-010411-BridgeMenu/Client.log')){
 CopyEvidence ('Saved/Verification/'+$name) ('Diagnostics/'+$name)
}
$acceptance=@{
 revision='PhysicalCrewBridge-R2-20260910';binary_sha256=$expectedDll;automation='30/30';passes=$passes
 gameplay_default='OceanFreighterR7';editor_startup='WaterLab';runtime_meshes='CrewR6B';blender_candidate='CrewR8-not-imported'
 complete_request=$false;unimplemented=@('articulated finger grip','full active locomotion','physical storm','moving full ship','fresh packaged build')
 boundaries=@('GPU water fall has explicit fixture placement','late join during active fall not tested','full four-player and impaired network not tested','ship route is 37 waypoints, late join does not run route','no human comfort/fun acceptance')
 maps=@(foreach($path in @('Content/Maps/OceanFreighterR7.umap','Content/Maps/WaterLab.umap')){@{path=$path;sha256=(Get-FileHash -LiteralPath (Join-Path $root $path)).Hash.ToLower()}})
 # Do not copy DefaultEngine.ini: unrelated private plugin configuration lives there.
 config_sha256=(Get-FileHash -LiteralPath (Join-Path $root 'Config/DefaultEngine.ini')).Hash.ToLower()
}
$acceptance|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dest 'acceptance.json') -Encoding utf8
$manifest=@(Get-ChildItem -LiteralPath $dest -File -Recurse|ForEach-Object {@{path=$_.FullName.Substring($dest.Length+1).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}})
$manifest|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $dest 'manifest.json') -Encoding utf8
foreach($entry in $manifest){if((Get-FileHash -LiteralPath (Join-Path $dest $entry.path)).Hash -ne $entry.sha256){throw "Frozen verification failed: $($entry.path)"}}
"FROZEN_R2_PASS files=$($manifest.Count) $dest"
