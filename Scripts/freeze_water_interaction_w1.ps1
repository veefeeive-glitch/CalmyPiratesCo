# One-shot capture of tested W1; refuses replacement of frozen evidence.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/WaterInteraction-W1-20260910'
if(Test-Path -LiteralPath $dest){throw 'Frozen W1 already exists'}
$dll='457ad4052e04ba94999e3dc1b98430b45cef9f4248e505742d26683a87fe96e5'
$material='c7729158fd7dc3ab5fa4a24e741d5f7d1fe95352442bb816f3a69dbcfd0e8338'
if((Get-FileHash (Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll')).Hash -ne $dll){throw 'DLL changed after tests'}
if((Get-FileHash (Join-Path $root 'Content/Materials/M_WaterInteraction_W1.uasset')).Hash -ne $material){throw 'Material changed after GPU'}
$suites=@{WaterInteraction='20260910-095247-WaterInteraction';Water='20260910-095401-Water';Character='20260910-095529-Character';Ship='20260910-095317-Ship'}
$markers=@{WaterInteraction='CALMY_WATER_INTERACTION';Water='CALMY_WATER';Character='CALMY_CHARACTER';Ship='CALMY_SHIP'}
$passes=@()
function CheckLog([string]$relative,[string]$pattern){
    $path=Join-Path $root $relative
    $found=@(Select-String -LiteralPath $path -Pattern $pattern)
    if($found.Count -ne 1){throw "Missing unique PASS: $relative"}
    if(Select-String -LiteralPath $path -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|CALMY_\w+ \w+ FAIL' -Quiet){throw "Failed evidence: $relative"}
    return $found[0].Line
}
foreach($s in $suites.Keys){foreach($role in @('Host','Client','LateJoin')){
    $passes+=CheckLog ('Saved/Verification/'+$suites[$s]+'/'+$role+'.log') ($markers[$s]+' '+$role+' PASS')
}}
$gpu=@{Water=@('WaterInteraction-Visual-20260910-100442','CALMY_WATER_INTERACTION Visual PASS',5);PhysicalCrew=@('PhysicalCrew-Visual-20260910-100656','CALMY_CHARACTER Visual PASS',6)}
foreach($g in $gpu.Keys){
    $entry=$gpu[$g];$passes+=CheckLog ('Saved/Verification/'+$entry[0]+'/Visual.log') $entry[1]
    if(@(Get-ChildItem (Join-Path $root ('Saved/Verification/'+$entry[0])) -Filter '*.png').Count -ne $entry[2]){throw "Missing GPU frames: $g"}
}
$auto='Saved/Logs/Automation-20260910-100553.log'
$results=@(Select-String (Join-Path $root $auto) -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 31 -or @($results|Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation not 31/31'}
New-Item -ItemType Directory -Path $dest | Out-Null
function CopyEvidence([string]$from,[string]$to){
    $target=Join-Path $dest $to
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $from) -Destination $target -Recurse
}
foreach($s in $suites.Keys){CopyEvidence ('Saved/Verification/'+$suites[$s]) ('Network/'+$s)}
foreach($g in $gpu.Keys){CopyEvidence ('Saved/Verification/'+$gpu[$g][0]) ('GPU/'+$g)}
CopyEvidence $auto 'Automation.log'
CopyEvidence 'Saved/Verification/WaterInteraction-W1-Build6.log' 'Build.log'
CopyEvidence 'Source' 'Source'
CopyEvidence 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll' 'Binary/UnrealEditor-CalmyPiratesCo.dll'
CopyEvidence 'Content/Materials/M_WaterInteraction_W1.uasset' 'Asset/M_WaterInteraction_W1.uasset'
CopyEvidence 'Docs/Changes/WATER_INTERACTION_W1_2026-09-10.md' 'Report.md'
foreach($name in @('run_network_smoke.ps1','run_water_interaction_review.ps1','run_physical_crew_review.ps1','audit_water_material.py','create_water_interaction_material.py','tune_water_interaction_material.py','freeze_water_interaction_w1.ps1')){CopyEvidence ('Scripts/'+$name) ('Scripts/'+$name)}
foreach($name in @('WaterInteraction-Visual-20260910-093632/Water1.png','WaterInteraction-Visual-20260910-094140/Water1.png','WaterInteraction-Visual-20260910-094637/Water3.png','WaterInteraction-W1-Build.log','WaterInteraction-MaterialAudit.log','WaterInteraction-MaterialTune.log','WaterInteraction-MaterialTune3.log','WaterInteraction-MaterialTune4.log')){CopyEvidence ('Saved/Verification/'+$name) ('Diagnostics/'+$name)}
$acceptance=@{
    revision='WaterInteraction-W1-20260910';binary_sha256=$dll;material_sha256=$material;automation='31/31';passes=$passes
    complete_request=$false;visual_quality='PARTIAL: softer foam, simple ring/water optics, low splash readability'
    runtime_meshes='CrewR6B';blender_candidate='CrewR8-not-imported';ship='static OceanFreighterR7'
    boundaries=@('approximate radial response field, not full fluid/storm','network tests before cosmetic foam gain .03 to .12; same DLL; automation and GPU repeated afterwards','GPU water views camera-only','37-waypoint route Host/Client only','no dedicated/4-player/impaired-network/long-soak acceptance','no finger grip/skin/active walking/moving hull/new package')
    maps=@(foreach($p in @('Content/Maps/OceanFreighterR7.umap','Content/Maps/WaterLab.umap')){@{path=$p;sha256=(Get-FileHash (Join-Path $root $p)).Hash.ToLower()}})
}
$acceptance|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $dest 'acceptance.json') -Encoding utf8
$manifest=@(Get-ChildItem -LiteralPath $dest -File -Recurse|ForEach-Object {@{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}})
$manifest|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $dest 'manifest.json') -Encoding utf8
foreach($f in $manifest){if((Get-FileHash (Join-Path $dest $f.path)).Hash -ne $f.sha256){throw "Frozen mismatch: $($f.path)"}}
"FROZEN_W1_PASS files=$($manifest.Count) $dest"
