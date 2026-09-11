#requires -Version 7.0
# Metadata inventory, not a gameplay acceptance tool. No gameplay or asset writes.
param([switch]$Verify)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$out=Join-Path $root 'Docs/Handover/INVENTORY.json'
function Relative([string]$p){[IO.Path]::GetRelativePath($root,$p).Replace('\','/')}
function SafePath([string]$p){
 $full=[IO.Path]::GetFullPath((Join-Path $root $p))
 if(-not $full.StartsWith($root+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw "Inventory path outside project: $p"}
 return $full
}
if($Verify){
 $snapshot=Get-Content -LiteralPath $out -Raw | ConvertFrom-Json
 $bad=@(foreach($f in $snapshot.primary_hashes){
  $p=SafePath $f.path
  if(-not(Test-Path -LiteralPath $p)){'MISSING '+$f.path}
  elseif((Get-FileHash -LiteralPath $p).Hash -ne $f.sha256){'CHANGED '+$f.path}
 })
 if($bad.Count){$bad;throw "HANDOVER_VERIFY_FAIL mismatches=$($bad.Count); review before regenerating"}
 "HANDOVER_VERIFY_PASS primary_files=$($snapshot.primary_hashes.Count); not gameplay PASS";return
}
$files=@(foreach($dir in @('Source','Config','Content','Scripts','SourceAssets','Docs','Evidence')){
 Get-ChildItem -LiteralPath (Join-Path $root $dir) -File -Recurse | Where-Object {$_.FullName -ne $out}
})
$primary=@($files | Where-Object {
 $p=Relative $_.FullName
 $p -match '^(Source|Config|Content|Scripts)/' -or
 $p -match '^SourceAssets/Generated/CrewR[0-9]+[A-Z]?/' -or
 $p -match '^SourceAssets/References/CrewR6/' -or
 $p -eq 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
})
$primary+=Get-Item -LiteralPath (Join-Path $root 'CalmyPiratesCo.uproject'),(Join-Path $root '.gitattributes'),(Join-Path $root '.gitignore'),(Join-Path $root '.vsconfig')
$hashes=@($primary | Sort-Object FullName -Unique | ForEach-Object {
 @{path=(Relative $_.FullName);bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}
})
$revisions=@(Get-ChildItem -LiteralPath (Join-Path $root 'SourceAssets/Generated') -Directory | Where-Object {$_.Name -match '^CrewR[0-9]+[A-Z]?$' -and (Test-Path -LiteralPath (Join-Path $_.FullName 'mesh_manifest.json'))} | Select-Object -ExpandProperty Name)
$models=@(foreach($rev in $revisions){
 $dir=Join-Path $root ('SourceAssets/Generated/'+$rev)
 $m=Get-Content -LiteralPath (Join-Path $dir 'mesh_manifest.json') -Raw | ConvertFrom-Json
 @{revision=$rev;folder=(Relative $dir);triangles=$m.triangles;glb_count=@(Get-ChildItem -LiteralPath $dir -Filter '*.glb').Count;texture_png_count=@(Get-ChildItem -LiteralPath $dir -Filter 'T_*.png').Count;runtime_folder_exists=(Test-Path -LiteralPath (Join-Path $root ('Content/Characters/'+$rev)))}
})
$rig=Get-Content -LiteralPath (Join-Path $root 'Source/CalmyPiratesCo/Private/Gameplay/CalmyCrewPresentation.cpp') -Raw
$runtime=[regex]::Match($rig,'/Game/Characters/([^/]+)/').Groups[1].Value
$fixed=@(Get-ChildItem -LiteralPath (Join-Path $root 'Scripts') -File -Recurse | Where-Object {$_.Extension -in @('.ps1','.py')} | ForEach-Object {
 $matches=@(Select-String -LiteralPath $_.FullName -Pattern '[CE]:[\\/]')
 if($matches.Count){@{path=(Relative $_.FullName);matching_lines=@($matches | Select-Object -ExpandProperty LineNumber)}}
})
$baseline=Join-Path $root 'Evidence/Gameplay/Crew-R6-20260908'
$evidenceEntries=Get-Content -LiteralPath (Join-Path $baseline 'manifest.json') -Raw | ConvertFrom-Json
$evidenceBad=@($evidenceEntries | Where-Object {
 $p=Join-Path $baseline $_.path
 -not(Test-Path -LiteralPath $p) -or (Get-FileHash -LiteralPath $p).Hash -ne $_.sha256
})
if($evidenceBad.Count){throw 'Frozen R6 evidence changed; stop and investigate'}
$frozenSource=Get-Content -LiteralPath (Join-Path $baseline 'source-hashes.json') -Raw | ConvertFrom-Json
$sourceDiff=@($frozenSource | Where-Object {$_.path -like 'Source/*'} | Where-Object {
 $p=Join-Path $root $_.path
 -not(Test-Path -LiteralPath $p) -or (Get-FileHash -LiteralPath $p).Hash -ne $_.sha256
} | Select-Object -ExpandProperty path)
$frozenSourcePaths=@($frozenSource | Where-Object {$_.path -like 'Source/*'} | Select-Object -ExpandProperty path)
$sourceDiff+=@(Get-ChildItem -LiteralPath (Join-Path $root 'Source') -Recurse -File | ForEach-Object {Relative $_.FullName} | Where-Object {$_ -notin $frozenSourcePaths})
$dll=Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll'
$tracked=@(& git -C $root ls-files);$remote=@(& git -C $root remote)
$totals=@($files | Group-Object { (Relative $_.FullName).Split('/')[0] } | ForEach-Object {
 @{folder=$_.Name;files=$_.Count;bytes=($_.Group | Measure-Object Length -Sum).Sum}
})
$result=[ordered]@{
 schema=1;generated_utc=[DateTime]::UtcNow.ToString('o');paths='relative to project root';
 runtime_character_path=$runtime;authoring_candidates=$models;
 git=@{tracked_count=$tracked.Count;remote_names=$remote;has_tracked_baseline=($tracked.Count -gt 0)};
 current_dll_sha256=$(if(Test-Path -LiteralPath $dll){(Get-FileHash -LiteralPath $dll).Hash.ToLower()}else{$null});
 frozen_r6=@{manifest_entries=$evidenceEntries.Count;hash_mismatches=$evidenceBad.Count;source_changed_since_baseline=$sourceDiff};
 scope='Metadata: Source/Config/Content/Scripts/SourceAssets/Docs/Evidence. Excludes caches, Saved, Builds, .git, tools and sibling Unity. Primary hashes are narrower than metadata.';
 folder_totals=$totals;scripts_with_machine_paths=$fixed;primary_hashes=$hashes;
 files=@($files | Sort-Object FullName | ForEach-Object {@{path=(Relative $_.FullName);bytes=$_.Length}})
}
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $out -Encoding utf8
"HANDOVER_INVENTORY_UPDATED files=$($files.Count) primary_hashes=$($hashes.Count) runtime=$runtime source_diffs=$($sourceDiff.Count) frozen_r6_ok=$($evidenceEntries.Count)"
