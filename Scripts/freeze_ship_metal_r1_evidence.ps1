$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root 'Evidence/Gameplay/ShipMetal-R1-20260906'
if (Test-Path -LiteralPath $dest) { throw 'Frozen evidence already exists' }
$visual = Join-Path $root 'Saved/Verification/ShipR7-Visual-20260906-210311'
$network = Join-Path $root 'Saved/Verification/20260906-210603-Ship'
if (-not (Select-String -Path "$visual/Visual.log" -Pattern 'CALMY_SHIP Visual PASS' -Quiet)) { throw 'GPU pass missing' }
foreach ($role in @('Host','Client','LateJoin')) {
    if (-not (Get-ChildItem $network -Filter '*.log' | Select-String -Pattern "CALMY_SHIP $role PASS")) { throw "$role pass missing" }
}
if (@(Get-ChildItem $visual -Filter '*.png').Count -ne 7) { throw 'Expected seven frames' }
$baseline = Get-Content "$root/Evidence/Gameplay/ShipR7-Integration-20260906/manifest.json" -Raw | ConvertFrom-Json
$old = @($baseline | Where-Object { $_.path.StartsWith('Snapshot/Content/') -and $_.path -ne 'Snapshot/Content/Maps/OceanFreighterR7.umap' })
foreach ($entry in $old) {
    if ((Get-FileHash -LiteralPath (Join-Path $root $entry.path.Substring(9))).Hash -ne $entry.sha256) { throw "Old content changed: $($entry.path)" }
}
New-Item -ItemType Directory -Path $dest | Out-Null
function Copy-Proof([string]$source, [string]$relative) {
    $target = Join-Path $dest $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $source) -Destination $target -Recurse
}
Copy-Proof 'Saved/Verification/ShipR7-Visual-20260906-210311' 'GPU'
Copy-Proof 'Saved/Verification/20260906-210603-Ship' 'Network'
Copy-Proof 'Saved/Verification/MetalR1-materials.json' 'assignments.json'
Copy-Proof 'Saved/Logs/Ship-MetalR1B-Apply-20260906.log' 'Apply.log'
Copy-Proof 'Content/Maps/OceanFreighterR7.umap' 'Delta/Content/Maps/OceanFreighterR7.umap'
Copy-Proof 'Content/Ships/OceanFreighterR7/MetalR1B' 'Delta/Content/Ships/OceanFreighterR7/MetalR1B'
foreach ($name in @('fix_ship_working_metal_r1.py','resume_ship_working_metal_r1.py','audit_ship_metal_parent.py','freeze_ship_metal_r1_evidence.ps1')) {
    Copy-Proof "Scripts/$name" "Delta/Scripts/$name"
}
Copy-Proof 'Docs/Changes/SHIP_METAL_R1_2026-09-06.md' 'Report.md'
$acceptance = [ordered]@{
    revision = 'ShipMetal-R1-20260906'
    baseline_required = 'Evidence/Gameplay/ShipR7-Integration-20260906'
    scope = 'Material-only delta; not a standalone project snapshot'
    material_instances = 40
    new_material_assets = 42
    old_content_checked = $old.Count
    old_content_changed_excluding_map = 0
    gpu = 'PASS: 37-waypoint real walking route, seven reviewed frames; three CameraActor views separate'
    network = 'Host and Client route PASS; LateJoin scene and grounded spawn only PASS'
    automation_rerun = $false
    human_acceptance = $false
    blender_master_changed = $false
    remaining = @('Overexposed lights/screens','Full interior lighting and Surface Cache audit','Preserve UE material overrides on future reimport','Full ship routes and human acceptance')
    map_sha256 = (Get-FileHash "$root/Content/Maps/OceanFreighterR7.umap").Hash.ToLower()
    master_sha256 = (Get-FileHash "$root/SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend").Hash.ToLower()
    dll_sha256 = (Get-FileHash "$root/Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll").Hash.ToLower()
}
$acceptance | ConvertTo-Json -Depth 5 | Set-Content "$dest/acceptance.json" -Encoding utf8
$manifest = @(Get-ChildItem $dest -File -Recurse | ForEach-Object {
    @{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/'); bytes=$_.Length; sha256=(Get-FileHash $_.FullName).Hash.ToLower()}
})
$manifest | ConvertTo-Json -Depth 4 | Set-Content "$dest/manifest.json" -Encoding utf8
foreach ($entry in (Get-Content "$dest/manifest.json" -Raw | ConvertFrom-Json)) {
    if ((Get-FileHash (Join-Path $dest $entry.path)).Hash -ne $entry.sha256) { throw 'Evidence hash mismatch' }
}
"METAL_EVIDENCE_PASS files=$($manifest.Count) mismatches=0 old_content_unchanged=$($old.Count)"
