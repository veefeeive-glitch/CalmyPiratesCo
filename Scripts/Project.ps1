#requires -Version 7.0
param(
 [ValidateSet('Check','Build','Automation','Play','Models')][string]$Action='Check',
 [string]$EngineRoot=$env:CALMY_UE_ROOT,[string]$BlenderExe=$env:CALMY_BLENDER_EXE,
 [ValidateSet('WaterLab','OceanFreighterR7')][string]$MapName='WaterLab',
 [switch]$StartPaused,[switch]$DryRun
)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'CalmyPiratesCo.uproject'
function RequireFile([string]$p){if(-not $p -or -not(Test-Path -LiteralPath $p -PathType Leaf)){throw "Required file missing: $p"}}
RequireFile $project
function RequireEngine {
 if(-not $EngineRoot){throw 'Pass -EngineRoot or set CALMY_UE_ROOT'}
 $p=Join-Path $EngineRoot 'Engine/Build/Build.version';RequireFile $p
 $v=Get-Content -LiteralPath $p -Raw | ConvertFrom-Json
 if($v.MajorVersion -ne 5 -or $v.MinorVersion -ne 8 -or $v.PatchVersion -ne 2){throw 'Expected UE 5.8.2; no automatic migration'}
}
if($Action -eq 'Check'){
 RequireEngine;RequireFile (Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe')
 RequireFile (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat');RequireFile $BlenderExe
 "ENV_PATHS_OK project=$root UE=5.8.2 Blender=$BlenderExe (paths only; not compiler/GPU/MCP validation)";return
}
if($Action -eq 'Models'){
 RequireFile $BlenderExe;$model=Join-Path $root 'SourceAssets/Generated/CrewR8/Crew_R8.blend';RequireFile $model
 if($DryRun){"DRY_RUN Models exe=$BlenderExe file=$model";return}
 Start-Process -FilePath $BlenderExe -ArgumentList ('"'+$model+'"') -WindowStyle Normal | Out-Null;return
}
RequireEngine
if($Action -eq 'Build'){
 $exe=Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat';RequireFile $exe
 $buildArgs=@('CalmyPiratesCoEditor','Win64','Development',('-Project='+$project),'-WaitMutex','-NoHotReloadFromIDE','-gather')
 if($DryRun){"DRY_RUN Build exe=$exe args=$($buildArgs -join ' ')";return}
 & $exe @buildArgs
 if($LASTEXITCODE -ne 0){throw "Build failed: $LASTEXITCODE"};return
}
if($Action -eq 'Automation'){
 $exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe';RequireFile $exe
 if($DryRun){"DRY_RUN Automation script=$PSScriptRoot/run_automation.ps1 editor=$exe";return}
 & (Join-Path $PSScriptRoot 'run_automation.ps1') -Editor $exe;return
}
$exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe';RequireFile $exe
$playArgs=@(('"'+$project+'"'),('/Game/Maps/'+$MapName),'-game','-DisablePython',
 '-MULTIHOME=127.0.0.1',
 '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if($StartPaused){$playArgs+='-CalmyStartPaused'}
if($DryRun){"DRY_RUN Play exe=$exe args=$($playArgs -join ' ')";return}
Start-Process -FilePath $exe -ArgumentList $playArgs -WindowStyle Normal | Out-Null
