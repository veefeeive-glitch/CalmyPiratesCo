# Read-only documentation check. Frozen Evidence/Builds and historical facts are not rewritten.
param([switch]$AllowMissingLocalArtifacts)
$ErrorActionPreference='Stop'
$rootPath=Split-Path -Parent $PSScriptRoot
$register=Get-Content -LiteralPath (Join-Path $rootPath 'Docs/document-register.json') -Raw | ConvertFrom-Json
$missing=[System.Collections.Generic.List[object]]::new()
$historyMissing=[System.Collections.Generic.List[object]]::new()
$localArtifactMissing=[System.Collections.Generic.List[object]]::new()
$documents=@(Get-ChildItem -LiteralPath $rootPath -Filter '*.md' -File)
$documents+=Get-ChildItem -LiteralPath (Join-Path $rootPath 'Docs') -Filter '*.md' -File -Recurse
$documents+=Get-Item -LiteralPath (Join-Path $rootPath 'SourceAssets/Generated/OceanFreighter/WALKTHROUGH_RU.md')
$links=0
foreach($document in $documents) {
    $body=Get-Content -LiteralPath $document.FullName -Raw
    foreach($match in [regex]::Matches($body,'!?\[[^\]\r\n]*\]\((?<target>[^\)\r\n]+)\)')) {
        $target=$match.Groups['target'].Value.Trim().Trim('<','>')
        if($target -match '^(https?://|mailto:|#)') { continue }
        $target=($target -split '#',2)[0]
        $target=[Uri]::UnescapeDataString($target)
        $absolute=if([IO.Path]::IsPathRooted($target)) {$target} else {Join-Path $document.DirectoryName $target}
        ++$links
        if(-not(Test-Path -LiteralPath $absolute)) {
            $entry=@{document=[IO.Path]::GetRelativePath($rootPath,$document.FullName);target=$target}
            $relativeTarget=[IO.Path]::GetRelativePath($rootPath,[IO.Path]::GetFullPath($absolute)).Replace('\','/')
            if($AllowMissingLocalArtifacts -and $relativeTarget -match '^(Evidence|Saved|Builds)(/|$)') {
                $localArtifactMissing.Add($entry)
                continue
            }
            if($document.FullName -like '*\Docs\History\*') {$historyMissing.Add($entry)} else {$missing.Add($entry)}
        }
    }
}
foreach($entry in $register.root_documents) {
    foreach($path in @($entry.original,$entry.current,$entry.history) | Where-Object {$_}) {
        if(-not(Test-Path -LiteralPath (Join-Path $rootPath $path))) { $missing.Add(@{document='register';target=$path}) }
    }
}
if($register.root_documents.Count -ne 29) { throw 'Original 29-document register is incomplete' }
if($missing.Count) { $missing | ConvertTo-Json -Depth 3;throw "DOCS_FAIL active_missing=$($missing.Count)" }
if($historyMissing.Count) { $historyMissing | ConvertTo-Json -Depth 3 }
"DOCS_PASS documents=$($documents.Count) local_links=$links active_missing=0 historical_missing=$($historyMissing.Count) unavailable_local_artifacts=$($localArtifactMissing.Count); artifact contents not verified"
