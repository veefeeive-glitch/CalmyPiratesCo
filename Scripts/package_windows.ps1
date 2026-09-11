param([string]$Engine = 'E:\Games\UE_5.8')
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'CalmyPiratesCo.uproject'
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$archive = Join-Path $projectRoot "Builds\Windows-$stamp"
if (Test-Path -LiteralPath $archive) { throw "Archive already exists: $archive" }
$log = Join-Path $projectRoot "Saved\Logs\Package-$stamp.log"
New-Item -ItemType Directory -Path $archive | Out-Null
$uat = Join-Path $Engine 'Engine\Build\BatchFiles\RunUAT.bat'
& $uat BuildCookRun "-project=$project" -target=CalmyPiratesCo -platform=Win64 -clientconfig=Development `
    -installed -nop4 -utf8output -unattended -nocompileeditor -skipbuildeditor `
    -build -cook -stage -package -pak -iostore -archive "-archivedirectory=$archive" -prereqs -nodebuginfo `
    '-AdditionalCookerOptions=-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False' 2>&1 | Tee-Object -FilePath $log
if ($LASTEXITCODE -ne 0) { throw "Packaging failed ($LASTEXITCODE). Log: $log" }
$exe = Join-Path $archive 'CalmyPiratesCo.exe'
if (-not (Test-Path -LiteralPath $exe)) { $exe = Join-Path $archive 'Windows\CalmyPiratesCo.exe' }
if (-not (Test-Path -LiteralPath $exe)) { throw "Expected packaged executable missing: $exe" }

# Keep immutable timestamped archives, but also promote the last successful build
# to one stable path. Windows Firewall keys application rules by executable path,
# so launching this stable copy avoids a new prompt for every timestamped archive.
$buildsRoot = [IO.Path]::GetFullPath((Join-Path $projectRoot 'Builds'))
$current = [IO.Path]::GetFullPath((Join-Path $buildsRoot 'Windows-Current'))
$previous = [IO.Path]::GetFullPath((Join-Path $buildsRoot 'Windows-Current.previous'))
foreach ($candidate in @($current, $previous)) {
    if (-not $candidate.StartsWith($buildsRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to update a current-build path outside $buildsRoot"
    }
}
if (Test-Path -LiteralPath $previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
if (Test-Path -LiteralPath $current) { Move-Item -LiteralPath $current -Destination $previous }
try {
    Copy-Item -LiteralPath $archive -Destination $current -Recurse
    $currentExe = Join-Path $current 'CalmyPiratesCo.exe'
    if (-not (Test-Path -LiteralPath $currentExe)) { $currentExe = Join-Path $current 'Windows\CalmyPiratesCo.exe' }
    if (-not (Test-Path -LiteralPath $currentExe)) { throw "Promoted executable missing: $currentExe" }
    if (Test-Path -LiteralPath $previous) { Remove-Item -LiteralPath $previous -Recurse -Force }
} catch {
    if (Test-Path -LiteralPath $current) { Remove-Item -LiteralPath $current -Recurse -Force }
    if (Test-Path -LiteralPath $previous) { Move-Item -LiteralPath $previous -Destination $current }
    throw
}
Write-Output "PACKAGE_ARCHIVE_PASS $exe"
Write-Output "PACKAGE_CURRENT_PASS $currentExe"
