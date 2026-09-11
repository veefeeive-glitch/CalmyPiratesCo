> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Deterministic emergency director — 2026-09-03

Engine: Unreal Engine 5.8.2. Scope: local Windows Development prototype, CC-203.

## Implementation and authority

The LeakTest emergency is now paced by a deterministic host-owned policy. The host selects one positive session seed (or accepts `-CalmyEmergencySeed=<n>` for reproducible development), derives timing from seed + round + event serial, and publishes only current facts. The first warning starts 18–24 seconds into a round. After a repaired leak drains completely, a 45–60 second cooldown must expire before a second warning. No more than two events can start per contract.

`FCalmyLeakSnapshot` now replicates `DirectorSeed`, `EventSerial`, `NextEventSeconds` and `CooldownSeconds` with the existing phase/water/repair/power snapshot. Clients never advance these clocks and have no RPC for director state. Existing repair intent remains the only relevant client RPC, with the previously documented ownership, sequence/rate, round, world, distance, line-of-sight and pawn-state validation. Disconnect releases the current repair lease; late join consumes the current snapshot rather than replaying events.

Every event start clears prior water, repairer, repair amount and lease. Contract completion suspends the director and clears countdowns. Host restart advances the round and creates a new deterministic schedule.

Changed files:

- `Public/Core/GameplayPolicies.h`, `Private/Core/GameplayPolicies.cpp`: deterministic delay/cooldown sampler, event cap and anti-stack admission policy.
- `Public/Gameplay/CalmyLeak.h`, `Private/Gameplay/CalmyLeak.cpp`: replicated director facts and server-owned multi-event lifecycle.
- `Private/Tests/GameplayPoliciesTests.cpp`, `Private/Tests/LeakTests.cpp`: seeded soak, timing bounds, cooldown, cap, non-stacking and clean second-event reset checks.
- `Public/Testing/CalmyLeakProbe.h`, `Private/Testing/CalmyLeakProbe.cpp`: Host/Client/LateJoin seed, serial, active late-join and new-round schedule proof.
- `Scripts/run_network_smoke.ps1`: deterministic seed 203 for the Leak suite.
- README, stack status and build information.

## Evidence

- Editor target build: PASS with MSVC 14.50.35737 and Windows SDK 10.0.22621.0.
- `Saved/Logs/Automation-20260903-232752.log`: **12/12 PASS**, including `CalmyPiratesCo.Gameplay.EmergencyDirectorSeededSoak`.
- Seeded soak facts: 64 seeds × 16 rounds × 600 seconds = **1,024 schedules / 170.67 simulated hours**; 2,048 starts; maximum concurrent emergencies 1; every schedule reached but did not exceed the two-event cap; every sampled initial delay and cooldown stayed inside its declared range.
- `Saved/Verification/20260903-231600-Leak`: editor Host/Client/LateJoin PASS with seed 203. Editor-only DataStorage UI factory warnings occur during headless editor startup; there are no project `Error:`, fatal, assertion or ensure markers.
- `Saved/Logs/Package-20260903-232817.log`: Windows Development package PASS, wrapper exit 0.
- `Saved/Verification/20260903-233041-Leak`: **packaged Host/Client/LateJoin PASS** from `Builds/Windows-Current`; all three logs contain the expected director markers and no `Error:`, `Warning:`, fatal, assertion or ensure markers.
- Firewall stability: the existing Private-only rule continued to match the refreshed stable EXE; no new Windows Security Alert appeared.

Exact commands:

```powershell
& 'E:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CalmyPiratesCoEditor Win64 Development 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -WaitMutex
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_automation.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Leak -Port 17793
& 'E:\Game\CalmyPiratesCoUE\Scripts\package_windows.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Leak -Port 17799 -PackagedExe 'E:\Game\CalmyPiratesCoUE\Builds\Windows-Current\CalmyPiratesCo\Binaries\Win64\CalmyPiratesCo.exe'
```

Artifact facts:

- Archive: `Builds/Windows-20260903-232817`, 682,550,186 bytes before `BUILD_INFO.md` (650.93 MiB).
- Editor DLL SHA-256: `4DF5CA576C1879DF26174CAAEDFDACFF6025E9AFF2D19A3DACD73647383960AE`.
- Archive and stable inner EXE SHA-256: `63A32C5A11582E5B7C402C21034BEA40899B759109BC917C3BC16664E0B0AA82`.
- Map hashes are unchanged from CC-202: CargoLab `63CD1C89727DEB25BABD6B8674075D1C0EF52225E60F2221AE2B68C835CF5E97`; DeckTest `E3667D9C863DCF673B4F6383A1BF46484D689220453555C4F064AC293DC945C6`; ShiftTest `88D1B0E73DCEE3EEAE9D2B37A5848DEBDB52A7A931910FD8E2ADDCB87E98583B`; LeakTest `C19CC4EE8134C52FEE4EB5325DE0483AF80D58E28E1883EF8D4241FC0771D489`.

## Limits and next task

- The soak validates deterministic scheduling invariants with a fixed 20-second synthetic active duration. It is not a 170-hour real-time engine soak and does not model human repair speed, cargo decisions or network impairment.
- Only the leak event exists. The policy prevents overlap and reserves a small cap for future event types, but it is not yet a weighted multi-event content system.
- Timing values and two-event cap are provisional tuning. Human readability, workload, comedy and fun gates remain unpassed.
- The packaged three-process smoke validates one host, one main client and one reconnecting/late client on loopback. It is not the explicit sustained one-host/two-client stress task.
- Next smallest task: CC-204 one-host/two-client technical stress with convergence, disconnect/reconnect and bounded-duration soak evidence.
