# One-shot local evidence snapshot. Never overwrites earlier evidence.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$out=Join-Path $root 'Evidence/Gameplay/Tutorial-Ship-T1-20260910'
if(Test-Path -LiteralPath $out){throw 'T1 evidence already exists; preserve it'}
$dll=Join-Path $root 'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll'
if((Get-FileHash -LiteralPath $dll).Hash -ne '2477C819A3826398F9607A997DC5B4B8F0BAF021AD8B9BB7580B11E2777D5176'){throw 'DLL differs from verified T1'}
$paths=@('Source','Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll','Scripts/start_game.ps1','Scripts/Project.ps1','Scripts/Play-Calmy.cmd','Scripts/run_tutorial_review.ps1','Scripts/run_network_smoke.ps1','Scripts/freeze_tutorial_t1.ps1','Docs/Changes/TUTORIAL_SHIP_T1_2026-09-10.md','Saved/Verification/Tutorial-T1-Build4.log','Saved/Logs/Automation-20260910-103341.log','Saved/Verification/20260910-103320-Tutorial','Saved/Verification/Tutorial-Visual-20260910-103331','Saved/Verification/20260910-103601-Water')
foreach($p in $paths){if(-not(Test-Path -LiteralPath (Join-Path $root $p))){throw "Missing evidence: $p"}}
New-Item -ItemType Directory -Path $out | Out-Null
foreach($p in $paths){
    $dst=Join-Path $out $p
    New-Item -ItemType Directory -Path (Split-Path -Parent $dst) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $p) -Destination $dst -Recurse
}
$files=@(Get-ChildItem -LiteralPath $out -File -Recurse | ForEach-Object {@{path=[IO.Path]::GetRelativePath($out,$_.FullName).Replace('\','/');sha256=(Get-FileHash -LiteralPath $_.FullName).Hash.ToLower();bytes=$_.Length}})
@{iteration='Tutorial-Ship-T1';files=$files;automation='32/32';network=@('Tutorial','Water');gpu_frames=6;packaged=$false} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $out 'manifest.json') -Encoding utf8
foreach($f in $files){if((Get-FileHash -LiteralPath (Join-Path $out $f.path)).Hash -ne $f.sha256){throw "Snapshot mismatch: $($f.path)"}}
"TUTORIAL_T1_FROZEN files=$($files.Count)"
