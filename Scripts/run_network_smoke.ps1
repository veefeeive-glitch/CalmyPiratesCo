param(
    [string]$Editor = 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe',
    [int]$Port = 17777,
    [ValidateSet('Carry','Shift','Cargo','Leak','Interior','Water','WaterInteraction','Character','Helper','Pause','Interaction','Ship','Door','BridgeMenu','Tutorial','FlySwarm','FlyMind','Melee','FlyTravel','Settings','Session')][string]$Suite = 'Carry',
    [ValidateRange(0,1200)][int]$SoakSeconds = 0,
    [ValidateRange(3,4)][int]$Peers = 3,
    [ValidateSet('None','Lag100','Lossy')][string]$NetProfile = 'None',
    [string]$PackagedExe,
    [switch]$FlyPlaced,
    [ValidateSet('WaterLab','OceanFreighterR7')][string]$FlyMap = 'OceanFreighterR7'  # the gameplay colony (FlyR2_Gameplay) lives only in HOLD 1 of the ship
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$project = Join-Path $projectRoot 'CalmyPiratesCo.uproject'
if ($PackagedExe) { $Editor = (Resolve-Path -LiteralPath $PackagedExe).Path }
$evidence = Join-Path $projectRoot ('Saved\Verification\' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + $Suite)
$marker = switch ($Suite) { 'Water' {'CALMY_WATER'} 'Shift' {'CALMY_SHIFT'} 'Cargo' {'CALMY_CARGO'} 'Leak' {'CALMY_LEAK'} 'Interior' {'CALMY_INTERIOR'} default {'CALMY_PROBE'} }
$probeOption = switch ($Suite) { 'Water' {'CalmyWaterProbe'} 'Shift' {'CalmyShiftProbe'} 'Cargo' {'CalmyCargoProbe'} 'Leak' {'CalmyLeakProbe'} 'Interior' {'CalmyInteriorProbe'} default {'CalmyProbe'} }
$testMap = switch ($Suite) { 'Water' {'/Game/Maps/WaterLab?listen'} 'Shift' {'/Game/Maps/ShiftTest?listen'} 'Cargo' {'/Game/Maps/CargoLab?listen'} 'Leak' {'/Game/Maps/LeakTest?listen'} 'Interior' {'/Game/Maps/FreighterDeck?listen'} default {'/Game/Maps/DeckTest?listen'} }
if ($Suite -eq 'Character') { $marker='CALMY_CHARACTER'; $probeOption='CalmyCharacterProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'Helper') { $marker='CALMY_HELPER'; $probeOption='CalmyHelperProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'Pause') { $marker='CALMY_PAUSE_TEST'; $probeOption='CalmyPauseProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'Interaction') { $marker='CALMY_INTERACTION'; $probeOption='CalmyInteractionProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'Ship') { $marker='CALMY_SHIP'; $probeOption='CalmyShipProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'Door') { $marker='CALMY_DOOR'; $probeOption='CalmyDoorProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'BridgeMenu') { $marker='CALMY_BRIDGE_MENU'; $probeOption='CalmyBridgeMenuProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'WaterInteraction') { $marker='CALMY_WATER_INTERACTION'; $probeOption='CalmyWaterInteractionProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'Tutorial') { $marker='CALMY_TUTORIAL_TEST'; $probeOption='CalmyTutorialProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'FlySwarm') { $marker='CALMY_FLY_SWARM_TEST'; $probeOption='CalmyFlySwarmProbe'; $testMap='/Game/Maps/WaterLab?listen' }
if ($Suite -eq 'FlySwarm' -and $FlyPlaced) { $testMap="/Game/Maps/${FlyMap}?listen" }
if ($Suite -eq 'FlyMind') { $marker='CALMY_FLY_MIND_TEST'; $probeOption='CalmyFlyMindProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'Melee') { $marker='CALMY_MELEE_TEST'; $probeOption='CalmyMeleeProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'FlyTravel') { $marker='CALMY_FLY_TRAVEL_TEST'; $probeOption='CalmyFlyTravelProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'Settings') { $marker='CALMY_SETTINGS_TEST'; $probeOption='CalmySettingsProbe'; $testMap='/Game/Maps/OceanFreighterR7?listen' }
if ($Suite -eq 'Session') { $marker='CALMY_SESSION_TEST'; $probeOption='CalmySessionProbe'; $testMap='/Game/Maps/WaterLab?listen'; if ($Peers -lt 4) { $Peers = 4 } }
New-Item -ItemType Directory -Path $evidence | Out-Null
if (Get-NetUDPEndpoint -LocalPort $Port -ErrorAction SilentlyContinue) { throw "UDP port $Port is occupied." }
$processes = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()
$common = @('-game', '-NullRHI', '-NoSound', '-unattended', '-nop4', '-nosplash', '-NoLoadingScreen', '-DisablePython', '-ExecCmds="t.MaxFPS 60"', '-MULTIHOME=127.0.0.1',
    '-ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False')
if ($Suite -eq 'Leak') { $common += '-CalmyEmergencySeed=203' }
if ($Suite -eq 'FlySwarm' -and $FlyPlaced) { $common += '-FlyPlaced' }
if ($Suite -eq 'FlyMind') { $common += ('-FlyMindTestSlot=' + (Split-Path $evidence -Leaf)) }
if ($Suite -eq 'Interaction') { $common += "-CalmyG1Soak=$SoakSeconds" }
if ($Suite -eq 'Session') { $common += "-CalmySessionPeers=$Peers" }
if ($NetProfile -eq 'Lag100') { $common += @('-PktLag=100') }
elseif ($NetProfile -eq 'Lossy') { $common += @('-PktLag=200', '-PktLoss=2', '-PktLagVariance=50') }
function Start-Probe([string]$Role, [string]$Map) {
    $prefixArgs = if ($PackagedExe) { @($Map) } else { @('"' + $project + '"', $Map) }
    $probeArgs = @($prefixArgs) + $common + @("-$probeOption=$Role", "-port=$Port", ('-abslog="' + (Join-Path $evidence "$Role.log") + '"'))
    # Settings writes GameUserSettings/preferences; UE 5.8 re-resolves that ini on every save, so isolate the whole user dir.
    if($PackagedExe -or $Suite -eq 'Settings'){$probeArgs+=('-UserDir="'+(Join-Path $evidence ($Role+'-Profile'))+'/"')}
    $proc = Start-Process -FilePath $Editor -ArgumentList $probeArgs -PassThru -WindowStyle Hidden
    $processes.Add($proc)
    return $proc
}
function Wait-Log([string]$Role, [string]$Pattern, [int]$Timeout = 60) {
    $until = (Get-Date).AddSeconds($Timeout)
    $log = Join-Path $evidence "$Role.log"
    do {
        if ((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern $Pattern -Quiet)) { return }
        if (-not ($processes | Where-Object { -not $_.HasExited })) { throw "All probes exited before $Role / $Pattern. Evidence: $evidence" }
        Start-Sleep -Seconds 1
    } while ((Get-Date) -lt $until)
    throw "Timed out waiting for $Role / $Pattern. Evidence: $evidence"
}
try {
    Write-Output "Evidence: $evidence"
    Write-Output "Peers=$Peers NetProfile=$NetProfile Suite=$Suite"
    $null = Start-Probe 'Host' $testMap
    Wait-Log 'Host' "$marker Host START"
    Write-Output 'Host ready; starting owning client.'
    $null = Start-Probe 'Client' "127.0.0.1:$Port"
    $lateTrigger = switch ($Suite) { 'Water' {'READY'} 'Shift' {'COMPLETE'} 'Cargo' {'READY'} 'Leak' {'READY'} 'Interior' {'INSIDE'} default {'HELD'} }
    if ($Suite -in @('Character','Helper','Pause','Interaction','Ship','Door','BridgeMenu','WaterInteraction','Tutorial','FlySwarm','FlyMind','Melee','FlyTravel','Settings','Session')) { $lateTrigger='READY' }
    Wait-Log 'Client' "$marker Client $lateTrigger" 150
    if ($Peers -ge 4) {
        Write-Output 'Client ready; starting second client and late joiner together.'
        $null = Start-Probe 'Client2' "127.0.0.1:$Port"
        $null = Start-Probe 'LateJoin' "127.0.0.1:$Port"
        Wait-Log 'Client2' "$marker Client2 READY" 120
        Wait-Log 'LateJoin' "$marker LateJoin READY" 120
    } else {
        Write-Output "Client reached $lateTrigger; starting late joiner."
        $null = Start-Probe 'LateJoin' "127.0.0.1:$Port"
    }
    $until = (Get-Date).AddSeconds(100 + $(if($Suite -eq 'Interaction'){$SoakSeconds}elseif($Suite -eq 'Ship'){140}elseif($Suite -eq 'FlyTravel'){60}elseif($Suite -eq 'Session'){40}else{0}))
    while (($processes | Where-Object { -not $_.HasExited }) -and (Get-Date) -lt $until) { Start-Sleep -Seconds 1 }
    $roles = @('Host', 'Client', 'LateJoin')
    if ($Peers -ge 4) { $roles = @('Host', 'Client', 'Client2', 'LateJoin') }
    foreach ($role in $roles) {
        $log = Join-Path $evidence "$role.log"
        $pass = Select-String -LiteralPath $log -Pattern "$marker $role PASS"
        if (-not $pass -or (Select-String -LiteralPath $log -Pattern "$marker .* FAIL|Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:|LogGameFeatures: Error:|LogAssetManager: Error:" -Quiet)) {
            throw "$role failed. Inspect $log"
        }
        $pass.Line | Write-Output
        if($PackagedExe -and (Select-String -LiteralPath $log -Pattern 'SkipPackage: /Game/|Failed to find object.* /Game/' -Quiet)) {
            throw "$role packaged build is missing runtime content. Inspect $log"
        }
    }
    foreach ($proc in $processes) {
        if (-not $proc.HasExited -or $proc.ExitCode -ne 0) { throw "Probe process $($proc.Id) did not exit successfully." }
    }
    if ($Suite -eq 'WaterInteraction') {
        $eventPattern='EVENT serial=(\d+) birth=([\d.]+) amplitude=([\d.]+) x=([-\d.]+) y=([-\d.]+) received_age=([-\d.]+)'
        $serverEvents=@{}
        foreach($line in Get-Content -LiteralPath (Join-Path $evidence 'Host.log')) {
            if($line -match $eventPattern){$serverEvents[$Matches[1]]=($Matches[2..5] -join '|')}
        }
        foreach($role in @('Client','LateJoin')) {
            $shared=0
            foreach($line in Get-Content -LiteralPath (Join-Path $evidence "$role.log")) {
                if($line -match $eventPattern) {
                    if(-not $serverEvents.ContainsKey($Matches[1]) -or $serverEvents[$Matches[1]] -ne ($Matches[2..5] -join '|')) {
                        throw "Water event snapshot mismatch: $role / $line"
                    }
                    $shared++
                }
            }
            if($shared -lt $(if($role -eq 'Client'){4}else{1})){throw "Too few shared water events for $role"}
            Write-Output "WATER_EVENT_SNAPSHOT_MATCH $role events=$shared"
        }
    }
    Write-Output 'NETWORK_SMOKE_PASS'
    if ($Suite -eq 'FlyMind') {
        $restoreProc = Start-Probe 'Restore' '/Game/Maps/OceanFreighterR7'
        $until = (Get-Date).AddSeconds(70)
        while (-not $restoreProc.HasExited -and (Get-Date) -lt $until) { Start-Sleep -Seconds 1 }
        $restoreLog = Join-Path $evidence 'Restore.log'
        if (-not $restoreProc.HasExited -or $restoreProc.ExitCode -ne 0 -or -not (Select-String -LiteralPath $restoreLog -Pattern 'CALMY_FLY_MIND_TEST Restore PASS' -Quiet)) { throw "Fly mind persistence restart failed: $restoreLog" }
        (Select-String -LiteralPath $restoreLog -Pattern 'CALMY_FLY_MIND_TEST Restore PASS').Line
    }
} finally {
    foreach ($proc in $processes) {
        if (-not $proc.HasExited) { Stop-Process -Id $proc.Id -ErrorAction SilentlyContinue }
    }
}
