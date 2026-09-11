> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# First Unreal gameplay slice — verification

This first section records the earlier carry slice. See the appended **Repeatable cargo shift** section for the newer build; historical hashes below are not the current module hashes.

Date: 2026-09-02. Engine: 5.8.2. Scope: Windows development/editor, local loopback.

## Verified

- C++ editor target builds successfully with Visual Studio Community 2026 / MSVC 14.51.
- Eight automation tests pass, including the original six policies, command replay/rate gate, and authoritative actor invariants.
- A real listen host and owning client complete movement, grab, carry-follow, throw, collision knockdown, recovery, and disconnect release.
- An additional real late-joining client sees the already-held cargo, resolves the holder, and sees its held transform.
- A duplicate client grab, a host's contested grab, and an out-of-range grab are rejected.
- Map validation reports zero errors and zero warnings.
- Visual PIE check: deck, character, cargo and HUD render; actual E keyboard input changes HUD to HELD, actual left mouse input launches cargo and clears the held state.

## Evidence

- Automation: `Saved/Logs/GameplayTests-R2.log` — eight completed successes, process exit 0.
- Network: `Saved/Verification/20260902-103045/Host.log`, `Client.log`, `LateJoin.log` — all three PASS, process exit 0, runner `NETWORK_SMOKE_PASS`.
- Creation: `Saved/Logs/CreateDeckTest.log` — map and five materials saved through Unreal APIs. The first creation command exited 1 due to missing GameFeatureData Asset Manager configuration; that configuration was subsequently added. The map itself saved successfully and was loaded by the later successful network run.
- First failed attempts remain in `Saved/Logs/GameplayTests.log` and `Saved/Verification/20260902-102748` for diagnosis. They are not counted as passes.
- Visual evidence: `Saved/Verification/20260902-103045/DeckTest-Gameplay.png` (actual PIE with HUD) and `DeckTest-Overview.png` (editor overview). `DeckTest-PIE.png` is an editor-camera capture taken during PIE, not the player's camera.
- Final map SHA256: `E3667D9C863DCF673B4F6383A1BF46484D689220453555C4F064AC293DC945C6`.
- Tested editor module SHA256: `ECFCA210C4BA6FC472E20E49E3DF29107E3EC2B8861A742F16E0B36B135A8374`.

## Boundaries and remaining work

- Simplified visual knockdown; no ragdoll, final animation, sound, or production UI.
- No human fun-gate claim. No packaged shipping build, 4-player soak, Internet session/matchmaking, latency/loss test, or gamepad validation yet.
- Standard physics replication is a prototype baseline, not proof of acceptable behavior on a high-latency network.
- Unreal startup emits two `LogAutomationTest: Condition failed` lines before project tests; these also occur on normal editor startup and are distinct from the eight passing project test results.
- Compiler warns that MSVC 14.51 is newer than the engine's preferred 14.50. Build and tests pass; do not silently change the toolchain.
- Unity source was read, not modified. Its Git ownership mismatch remains unresolved; no global trust bypass was added.
- No commit, remote repository, or remote backup was created.

## Reproduce

Close the editor normally before compiling. Use the commands in `README.md`, then run `Scripts/run_network_smoke.ps1`. New network evidence goes to a timestamped directory. `Scripts/start_dev.ps1` restores the local editor/MCP environment after a reboot; it does not create an OS startup task.

## Repeatable cargo shift — 2026-09-02

Implemented: separate ShiftTest map, server-owned physical delivery zone and exact manifest, completed shift / host-only generation-checked restart, replicated counters and delivered flags, safe player/held-cargo/thrown-cargo return, deterministic collision-free player return slots, HUD objective and rescue feedback. DeckTest and Unity content were not modified.

- Editor C++ rebuild succeeds on the existing toolchain.
- `Saved/Logs/Automation-20260902-113315.log`: **9/9 project tests pass**, confirmed by `Scripts/run_automation.ps1` result `AUTOMATION_PASS`.
- The new `ResetAndContract` test checks occupied spawn fallback, cleared carry links, exact reset counts, knockdown recovery, settle time, duplicate delivery (including an object reset), full-zone bounds, moving cargo rejection, unique-manifest failure, authorized restart and stale round rejection.
- `Saved/Verification/20260902-112848-Shift`: actual Host, Client and LateJoin all PASS; runner exits 0. Client physically moves/carries/drops all three crates. Server-position fixtures explicitly force one held-player fall and one already-thrown crate fall; these are test fixtures, not evidence of human-controlled falls. Late join observes the completed contract and rescue count; remote restart is rejected; host restart clears the contract exactly once.
- `Saved/Verification/20260902-113134-Carry`: old Host/Client/LateJoin movement, contention/range, throw impact, lifecycle, duplicate rejection and disconnect-release regression all PASS on the newer module.
- `Saved/Logs/CreateShiftTest.log`: Unreal API map generation succeeds, commandlet exit 0, 0 errors / 0 warnings.
- `Saved/Logs/ShiftTests-R1.log` is a failed earlier unit run: its transient test controller had no local-player marker. The fixture was corrected; production authorization was not weakened. R2 and the wrapper run pass. The earlier failing engine run still exited 0, which is why the new wrapper checks test results explicitly.
- Editor-only target restrictions were added to ModelContextProtocol, AllToolsets, PythonScriptPlugin and EditorScriptingUtilities. The runtime game must not include development-control plugins.

The new contract is a small repeatable loading exercise, not the full 12–20 minute voyage design. No human fun gate, three cargo behaviors, emergency/repair loop, lobby/matchmaking, four-player soak, lossy-network quality, audio or final art is claimed.

Packaging procedure reference: [Epic's BuildCookRun overview](https://dev.epicgames.com/documentation/unreal-engine/build-operations-cooking-packaging-deploying-and-running-projects-in-unreal-engine).

### Final packaged verification and hand-off

- Recommended archive: `Builds/Windows-20260902-114027`, approximately 651 MiB before generated runtime logs. Launch `CalmyPiratesCo.exe` at its root. Configuration: Windows x64 **Development**, not Shipping.
- `Saved/Logs/Package-20260902-114027.log`: BuildCookRun **BUILD SUCCESSFUL**, UAT exit 0; wrapper **PACKAGE_PASS**, exit 0. Includes the VC++ prerequisite installers, which were packaged, not run.
- `Saved/Verification/20260902-114138-Shift`: **packaged executable** Host/Client/LateJoin all PASS, runner exit 0. These three final probe logs contain no `Error:` or `Warning:` lines, no ensures, and no game-feature startup error. This run also verifies the newly compiled probe stage logging and cooked materials/map dependencies.
- `Saved/Logs/ShiftMapCheck.log`: explicit `MAP CHECK` reports **0 errors, 0 warnings**. Correction from the next development turn: the unattended process did not actually exit on QUIT and retained the editor DLL; it was identified by its exact test command line and stopped before the next build. Its process exit is not counted as a pass. The known engine-startup self-test messages remain separate from map results.
- Visual PIE evidence: `Saved/Verification/20260902-112848-Shift/ShiftTest-Overview.png` and `ShiftTest-Gameplay.png`. Deck, three crates, open gangway, equipment block, green delivery zone and the 0/3 contract HUD render.
- The final packaged game was also launched visibly with real D3D12/SM6 rendering. Its deck and HUD rendered; a Windows Firewall permission dialog overlays the game. Further GUI input was not attempted because Computer Use does not allow acting on system security permissions. User must dismiss the prompt; no firewall rules were changed.
- Real-rendering log: `Builds/Windows-20260902-114027/CalmyPiratesCo/Saved/Logs/CalmyPiratesCo.log`. No gameplay startup error; renderer warns about NVIDIA-driver TSR 16-bit support and `r.MotionVectorSimulation` thread safety. No driver/system change was performed.
- Unreal Editor, Blender and Visual Studio were left open; the standalone game is open too. Official Unreal MCP listens only on `127.0.0.1:8000`, Blender on `127.0.0.1:9876`. Blender's stale initial connector was retried successfully with a read-only scene query.
- The GameFeatures runtime plugin is explicitly enabled (not the agent GameFeaturesToolset), preserving the shared asset-type rule for both editor-run games and cooked games. The temporary `bIsEditorOnly` rule experiment was reverted because editor `-game` also needs the rule. No packaged MCP/toolset server is active.

Final hashes:

- DeckTest (unchanged): `E3667D9C863DCF673B4F6383A1BF46484D689220453555C4F064AC293DC945C6`.
- ShiftTest: `88D1B0E73DCEE3EEAE9D2B37A5848DEBDB52A7A931910FD8E2ADDCB87E98583B`.
- Editor module: `C51462FD0BDA19ED4572DC4EAF6A569C13E7D3146E3267AEA7C2B88ABC525879`.
- Final packaged inner executable: `276A11E4E3072360D0CB5C844CEC7323B37F9861AB42C5030118F68E73B3C2C0`.

Earlier attempts were retained for diagnosis, not recommended for play: archive `Windows-20260902-113144` had a missing GameFeatures-class ensure despite successful cooking; its first wrapper also assumed an extra Windows subfolder. Probe `20260902-113609-Shift` exposed a PowerShell single-item array concatenation error, subsequently fixed. Probe `20260902-113704-Shift` passed gameplay but retained the startup ensure. Archive attempt `Windows-20260902-113844` failed cooking because it tried to claim the open editor's MCP port. Probe `20260902-113912-Carry` passed gameplay during the editor-only rule experiment but logged its configuration error. The final scripts detect those ensures/asset-manager errors and disable the cook process's MCP auto-start. Only the final archive and final packaged probe are the accepted hand-off.
