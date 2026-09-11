# One-time local snapshot of the accepted STATIC integration, not an off-device backup/package.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$dest=Join-Path $root 'Evidence/Gameplay/ShipR7-Integration-20260906'
if(Test-Path -LiteralPath $dest){throw 'Frozen evidence already exists; do not overwrite'}
$ship='Saved/Verification/20260906-181051-Ship'
$water='Saved/Verification/20260906-181348-Water'
$visual='Saved/Verification/ShipR7-Visual-20260906-181041'
$automation='Saved/Logs/Automation-20260906-181244.log'
$expectedDll='B1D65EECDE052118211144910A57B55CC1EBF91185353FA0AB4A34332C3C3016'
$expectedMaster='8B4FE72D31E549649C0B36947A6CB5B906EBBD19B0955D5BD1F76D5F7DA44F93'
if((Get-FileHash -LiteralPath (Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll')).Hash -ne $expectedDll){throw 'DLL changed since verification'}
if((Get-FileHash -LiteralPath (Join-Path $root 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend')).Hash -ne $expectedMaster){throw 'Master changed'}
$passes=[System.Collections.Generic.List[string]]::new()
foreach($suite in @(@($ship,'CALMY_SHIP'),@($water,'CALMY_WATER'))) {
 foreach($role in @('Host','Client','LateJoin')) {
  $log=Join-Path $root ($suite[0]+"/$role.log")
  $pass=Select-String -LiteralPath $log -Pattern ($suite[1]+" $role PASS")
  if(-not $pass -or (Select-String -LiteralPath $log -Pattern ' FAIL |Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet)){throw "Unaccepted log: $log"}
  $passes.Add($pass.Line)
 }
}
$vlog=Join-Path $root ($visual+'/Visual.log')
if(-not(Select-String -LiteralPath $vlog -Pattern 'CALMY_SHIP Visual PASS' -Quiet) -or
 (Select-String -LiteralPath $vlog -Pattern ' FAIL |Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|overflow|missing usage flag Nanite|Invalid material' -Quiet)){throw 'GPU fixture not clean'}
if(@(Get-ChildItem -LiteralPath (Join-Path $root $visual) -Filter '*.png').Count -ne 7){throw 'Missing GPU views'}
$results=@(Select-String -LiteralPath (Join-Path $root $automation) -Pattern 'Test Completed.*Path=\{CalmyPiratesCo\.')
if($results.Count -ne 26 -or @($results | Where-Object {$_.Line -notmatch 'Result=\{Success\}'}).Count){throw 'Automation not 26/26'}
New-Item -ItemType Directory -Path $dest | Out-Null
function Snapshot([string]$Relative) {
 $from=Join-Path $root $Relative;$to=Join-Path $dest ('Snapshot/'+$Relative)
 New-Item -ItemType Directory -Path (Split-Path -Parent $to) -Force | Out-Null
 Copy-Item -LiteralPath $from -Destination $to -Recurse
}
foreach($folder in @('Source','Config','Content')){Snapshot $folder}
foreach($folder in @('SourceAssets/Generated/ShipR7Game','SourceAssets/Generated/ShipR7RenderB')){Snapshot $folder}
Snapshot 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
foreach($f in Get-ChildItem -LiteralPath $PSScriptRoot -File | Where-Object {$_.Extension -in '.py','.ps1'}){Snapshot ('Scripts/'+$f.Name)}
foreach($path in @('CalmyPiratesCo.uproject','AGENTS.md','README.md','Docs/Current','Docs/Roadmaps','Docs/Changes/SHIP_R7_INTEGRATION_2026-09-06.md','Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll','Binaries/Win64/UnrealEditor.modules')){Snapshot $path}
foreach($pair in @(@($ship,'Ship'),@($water,'WaterLab'),@($visual,'Visual'))) {
 Copy-Item -LiteralPath (Join-Path $root $pair[0]) -Destination (Join-Path $dest $pair[1]) -Recurse
}
Copy-Item -LiteralPath (Join-Path $root $automation) -Destination (Join-Path $dest 'Automation.log')
Copy-Item -LiteralPath 'C:/Users/Valerii/AppData/Local/UnrealBuildTool/Log.txt' -Destination (Join-Path $dest 'Build.log')
$diagnostics=Join-Path $dest 'Diagnostics';New-Item -ItemType Directory -Path $diagnostics | Out-Null
Get-ChildItem -LiteralPath (Join-Path $root 'Saved/Logs') -Filter 'ShipR7-*.log' | Copy-Item -Destination $diagnostics
foreach($attempt in @('ShipR7-Visual-20260906-174304','ShipR7-Visual-20260906-174511','ShipR7-Visual-20260906-175521','ShipR7-Visual-20260906-180031','20260906-175531-Ship','20260906-180116-Ship')){
 Copy-Item -LiteralPath (Join-Path $root ('Saved/Verification/'+$attempt)) -Destination (Join-Path $diagnostics $attempt) -Recurse
}
$acceptance=[ordered]@{kind='static_ship_integration_bounded_review';date='2026-09-06';map='/Game/Maps/OceanFreighterR7';dll_sha256=$expectedDll.ToLower();master_sha256=$expectedMaster.ToLower();automation_passed=26;network_pass_lines=@($passes);visual_pass=(Select-String -LiteralPath $vlog -Pattern 'CALMY_SHIP Visual PASS').Line;gpu_images_reviewed=7;full_ship_accepted=$false;sailing_implemented=$false;all_old_routes_repeated=$false;human_acceptance=$false;new_windows_package=$false}
$acceptance | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $dest 'acceptance.json') -Encoding utf8
$entries=@(Get-ChildItem -LiteralPath $dest -File -Recurse | ForEach-Object {[ordered]@{path=[IO.Path]::GetRelativePath($dest,$_.FullName).Replace('\','/');bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower()}})
$entries | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $dest 'manifest.json') -Encoding utf8
$mismatches=0
foreach($entry in $entries){if((Get-FileHash -LiteralPath (Join-Path $dest $entry.path)).Hash.ToLower() -ne $entry.sha256){++$mismatches}}
if($mismatches){throw 'Snapshot verification failed'}
Write-Output "SHIP_R7_EVIDENCE_PASS files=$($entries.Count) mismatches=$mismatches $dest"
