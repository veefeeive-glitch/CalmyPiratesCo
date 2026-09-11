> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# First shared leak and repair — 2026-09-02

Unreal Engine 5.8.2, local Windows Development prototype. Implements the first bounded emergency/repair slice (CC-200/201). Not the full voyage, emergency director, lighting consequence or human playtest.

## Behavior and authority

`LeakTest` is a separate copy of the standard-cargo ShiftTest. After 20 seconds of loading, a five-second warning precedes one leak. Water rises 2 points/s, capped at 100; at 50% all player movement is multiplied by 0.7. Hold F stationary and grounded within 180 cm of the valve, with empty hands and clear visibility. One player repairs at 20 points/s. At 100 repair the leak seals and water drains 10 points/s. Partial work survives interruptions; no irreversible failure or mandatory second worker. One event per round; completion suspends it and a host restart resets its generation and state.

The host owns the phase, water, repair progress, exclusive repairer and time. One replicated snapshot supports late join. Owned-pawn RPCs accept only intent and validate monotonic sequence, heartbeat rate, expected round, target world, player state, range and visibility. Release bypasses rate limiting but still consumes the sequence. A heartbeat lease lasts 0.65 server seconds; invalid/disconnected workers are released on tick and normal Logout/EndPlay. No client submits progress, flood level or success. The RPC contract is documented beside its declaration.

## Changed files

- `Public/Gameplay/CalmyLeak.h`, `Private/Gameplay/CalmyLeak.cpp`: replicated event, valve, repair and water indicator.
- `Public/Gameplay/CalmyCharacter.h`, `Private/Gameplay/CalmyCharacter.cpp`: F input, repair intent RPC/gate, lease heartbeats, flood speed multiplier and disconnect cleanup.
- `Private/Gameplay/CalmyGameMode.cpp`: lower emergency banner and Logout cleanup.
- `Private/Gameplay/CalmyShift.cpp`: explicit reset on host restart.
- `Private/Tests/LeakTests.cpp`: server actor-state/geometry/lifecycle tests.
- `Public/Testing/CalmyLeakProbe.h`, `Private/Testing/CalmyLeakProbe.cpp`: non-shipping three-process scenario, disabled unless explicitly opted in.
- `Content/Maps/LeakTest.umap`, `Scripts/create_leak_test.py`, `Config/DefaultGame.ini`: new map and cook entry through Unreal asset APIs, no binary text editing.
- `Scripts/start_game.ps1`, `run_network_smoke.ps1`, `run_automation.ps1`: map/suite selection, early process-exit detection, minimum eleven tests.
- README, this report, stack status and archive build information.

## Verified

- Editor C++ target compiled and linked successfully. No new project deprecation warnings remain; the installed newer-than-preferred compiler and engine header C4996 warnings persist.
- `Saved/Logs/CreateLeakTest.log`: Unreal Python commandlet exit 0, 0 errors / 0 warnings. The generated map's duplication check reports no errors.
- `Saved/Logs/Automation-20260902-122656.log`: **11/11 project automation tests PASS**, wrapper exit 0. Includes initial delay/warning, water/slowdown, stale/null/range/visibility rejection, exclusive worker, pulse-count independence, release/partial work, cargo/movement/airborne/disconnect rejection, takeover, drain, no event stacking and round reset.
- `Saved/Verification/20260902-121935-Leak`: **Host / Client / LateJoin PASS**, runner exit 0. Actual network movement and gameplay RPCs: warning, flood slowdown, duplicate/out-of-range/cargo repair denial, partial repair pause, client exits while actively repairing, late join sees current state and resumes after disconnect, water drains, host physically delivers three crates, contract completes and restarts with cleared event state. No network fixture teleports, direct stat writes, test-only gameplay RPCs or authority bypasses.
- Carry regression: `Saved/Verification/20260902-122254-Carry`, all three roles PASS, runner exit 0.
- Cargo behavior regression: `Saved/Verification/20260902-122304-Cargo`, all three roles PASS, runner exit 0.
- Shift regression: `Saved/Verification/20260902-122403-Shift`, all three roles PASS, runner exit 0. This older suite retains its two explicitly logged overboard-position fixtures.
- `Saved/Logs/Package-20260902-122458.log`: UAT **BUILD SUCCESSFUL**, exit 0, **PACKAGE_PASS**. New independent archive `Builds/Windows-20260902-122458`.
- All PowerShell helpers parse successfully.
- `Saved/Verification/20260902-122714-Leak`: **packaged Host / Client / LateJoin all PASS**, runner exit 0, all child exits 0. Same repair/disconnect/takeover/full-contract/restart scenario against the archive's inner executable. All three packaged logs have no `Error:` or `Warning:` lines.
- Visual PIE inspection: `Saved/Verification/20260902-122714-Leak/LeakTest-NearValve.png` shows the green idle valve, normal cargo and separate lower emergency banner. The name is a capture label, not a claimed player-position check: GameMode still chose the normal spawn despite the PIE start-transform request. `LeakTest-ActiveFinal.png` records the active event; earlier oversized-banner captures are retained under the editor network run.
- Visual Studio solution/project files regenerated successfully. Unreal reopened on LeakTest; PIE stopped after inspection so the user can begin a fresh drill.

Final artifact hashes (SHA256):

- LeakTest map: `C19CC4EE8134C52FEE4EB5325DE0483AF80D58E28E1883EF8D4241FC0771D489`.
- Editor module: `3253B94E7AE59656182581980DAE4FFE039D100E53667723EE414C82E922E840`.
- Packaged inner executable: `A9CBBA4F0F18E0DDBB666A4C8D830FA2380719354AEE41E2818BFFC1A191F431`.
- Archive size approximately 651 MiB. Previous verified archives remain intact.

Editor `-game` network logs have existing editor widget-factory registration warnings; no gameplay error/ensure/fatal was accepted by the runner. Do not call these editor logs warning-free.

## Investigation history

The initial isolated actor test failed its takeover branch because its temporary world had no floor: the non-local second pawn was in Falling mode. Diagnostics recorded controller=1, controlled=1, held=0, falling=1 and no visibility blocker. A physical floor and explicit airborne/grounded assertions corrected the fixture; gameplay validation was not weakened. The first network rehearsal was stopped before completion after discovering its crate-drop route could obstruct the valve; the corrected route puts cargo aside. That interrupted run is not accepted evidence.

One editor build attempt began before the normal editor shutdown had finished and was refused by Live Coding. Re-running after process exit succeeded. No user editor was force-killed; only exact owned rehearsal processes were stopped.

## Reproduce

Close the editor normally and wait for it to exit before C++ builds.

```powershell
& 'E:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CalmyPiratesCoEditor Win64 Development 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -WaitMutex
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_automation.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Leak -Port 17783
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Carry -Port 17777
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Cargo -Port 17781
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Shift -Port 17779
& 'E:\Game\CalmyPiratesCoUE\Scripts\package_windows.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\start_game.ps1' -Mode Solo -MapName LeakTest
```

For a packaged network run, add `-PackagedExe` with the archive's inner `CalmyPiratesCo/Binaries/Win64/CalmyPiratesCo.exe`, not its bootstrap launcher.

## Boundaries and next smallest task

- This is an opt-in loading-phase drill with a gameplay water meter and flat blue indicator, not physical water simulation, sinking, traversal physics or voyage progression.
- Original DeckTest, ShiftTest and CargoLab map hashes are unchanged; default remains solo-friendly ShiftTest. LeakTest uses standard crates, so a combined special-cargo + emergency contract is not claimed tested.
- F callbacks/RPCs are exercised by the probe; physical keyboard hold, input-focus loss, gamepad and accessibility still need an explicit manual input check. Automated network runs disable audio; this leak increment has visual warnings only.
- Heartbeat expiry is implemented; deliberate network loss/jitter/latency soak is not yet tested. Three processes briefly overlap for late join, which is not the full CC-204 three-player stress gate.
- No human fun/readability/co-op gate is passed. The full mixed-cargo human shift remains unvalidated.
- Next smallest slice: readable degraded-power/light feedback tied to this existing emergency (CC-202), with fixed normal/degraded captures and replicated-state proof, followed by deterministic pacing/soak (CC-203).
- No Unity source, firewall/security permission, paid service, remote deployment or repository history was changed. Earlier builds remain intact.
