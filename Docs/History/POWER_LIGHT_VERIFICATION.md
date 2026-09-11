> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Readable emergency power consequence — 2026-09-03

Engine: Unreal Engine 5.8.2. Scope: local Windows Development prototype, CC-202 only.

## Implementation and authority

At 30% authoritative leak water, `FCalmyLeakSnapshot.bPowerDegraded` becomes true. It remains true while leaking or draining at or above 30%, then clears. Contract suspension and a new round explicitly clear it. The server derives this boolean from the existing water/phase state; clients have no power RPC and cannot submit the value.

Each peer presents the replicated state locally with one unbound dim/desaturation grade, a red valve work light and a blue delivery-zone task light. The HUD names the emergency-power state and the two preserved next-action cues. This is a readable greybox consequence, not an electrical grid simulation.

Changed files:

- `Public/Gameplay/CalmyLeak.h`, `Private/Gameplay/CalmyLeak.cpp`: replicated power field and local post-process/task-light presentation.
- `Private/Gameplay/CalmyGameMode.cpp`: emergency-power HUD instruction.
- `Private/Tests/LeakTests.cpp`: normal/warning, threshold, drain, round and contract-end power assertions.
- `Public/Testing/CalmyLeakProbe.h`, `Private/Testing/CalmyLeakProbe.cpp`: Host/Client/LateJoin degraded and recovered state proof.
- `Scripts/measure_power_captures.ps1`: deterministic capture metrics and thresholds.
- README, this report, stack status and new archive build information.

## Evidence

- Editor target build: success with the installed preferred MSVC 14.50.35737 toolchain and Windows SDK 10.0.22621.0.
- `Saved/Logs/Automation-20260903-223023.log`: **11/11 automation tests PASS**, wrapper exit 0.
- `Saved/Verification/20260903-223023-Leak`: **Host / Client / LateJoin PASS**, runner exit 0. Client observes replicated emergency power; LateJoin joins during degraded power; all roles observe recovery, full delivery and new-round cleanup.
- `Saved/Logs/Package-20260903-223349.log`: **Windows Development package PASS**, wrapper exit 0. The timestamped archive is `Builds/Windows-20260903-223349` (650.93 MiB); earlier archives were not overwritten.
- `Saved/Verification/20260903-223729-Leak`: **packaged Host / Client / LateJoin PASS**, runner exit 0 against the archive's inner executable. All three logs contain their expected `CALMY_LEAK ... PASS` marker and no `Error:` or `Warning:` entry.
- `Saved/Verification/20260903-230032-Leak`: **stable-path packaged Host / Client / LateJoin PASS** after installing the permanent `Calmy Pirates Co. Development (Private)` rule. The rule targets `Builds/Windows-Current/CalmyPiratesCo/Binaries/Win64/CalmyPiratesCo.exe`, is inbound/allow on Private only, and no new Windows Security Alert appeared.
- Fixed visual captures: `Saved/Verification/20260903-PowerLight/Normal.png` and `Degraded.png`, both 1280x722 from the same map, spawn and camera.
- `Saved/Verification/20260903-PowerLight/metrics.json`: mean luminance 102.34 normal vs 81.24 degraded, ratio **0.794**. Degraded frame retains 2,815 orange, 147 green, 117,498 cyan and 11,839 bright-neutral sampled pixels, all above automated cue-preservation thresholds. `measure_power_captures.ps1` reports `POWER_CAPTURE_METRICS_PASS`.
- PowerShell helper syntax check: PASS.
- Final editor state: Unreal MCP is listening on loopback port 8000, `/Game/Maps/LeakTest` is open, PIE is stopped and the editor reports `All Saved`.

Exact commands:

```powershell
& 'E:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CalmyPiratesCoEditor Win64 Development 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -WaitMutex
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_automation.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Leak -Port 17783
& 'E:\Game\CalmyPiratesCoUE\Scripts\measure_power_captures.ps1' -Normal 'E:\Game\CalmyPiratesCoUE\Saved\Verification\20260903-PowerLight\Normal.png' -Degraded 'E:\Game\CalmyPiratesCoUE\Saved\Verification\20260903-PowerLight\Degraded.png' -OutputJson 'E:\Game\CalmyPiratesCoUE\Saved\Verification\20260903-PowerLight\metrics.json'
& 'E:\Game\CalmyPiratesCoUE\Scripts\package_windows.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Leak -Port 17787 -PackagedExe 'E:\Game\CalmyPiratesCoUE\Builds\Windows-20260903-223349\CalmyPiratesCo\Binaries\Win64\CalmyPiratesCo.exe'
```

Artifact SHA-256:

- `UnrealEditor-CalmyPiratesCo.dll`: `15ABFAF8E4BE3DFE8A1470030EF7D662C47EA08726C75444D75424C29F2F7E8A`
- packaged inner `CalmyPiratesCo.exe`: `8C91B6E86398E8BD2AA94C93D226AAC5EAC74287121DEFFAAAA8229E66E28D1C`
- map assets are unchanged: CargoLab `63CD1C89727DEB25BABD6B8674075D1C0EF52225E60F2221AE2B68C835CF5E97`; DeckTest `E3667D9C863DCF673B4F6383A1BF46484D689220453555C4F064AC293DC945C6`; ShiftTest `88D1B0E73DCEE3EEAE9D2B37A5848DEBDB52A7A931910FD8E2ADDCB87E98583B`; LeakTest `C19CC4EE8134C52FEE4EB5325DE0483AF80D58E28E1883EF8D4241FC0771D489`.

## Limits and next task

- Pixel counts are technical evidence that key color families remain present, not proof that humans notice or correctly interpret them. Human readability/fun gates remain unpassed.
- The fixed capture includes the editor chrome and HUD identically in both frames; the ratio still measures a controlled comparison, but it is not a photometric game-only benchmark.
- Greybox post-processing and two non-shadow-casting point lights are placeholders. There is no circuit topology, flicker sequence, switchboard, audio alarm or accessibility alternative beyond text.
- Packaging emitted Unreal's non-fatal NTFS journal/cache advisory during cooking. No elevation or machine-wide filesystem change was made; the build and packaged network test completed successfully.
- Windows displayed a firewall access prompt for the timestamped packaged executable after the loopback test. It was deliberately left untouched; the automated proof uses loopback only and completed without a firewall rule change.
- The cyan count includes the sea/background and water indicator; it is not attributed solely to the delivery task light. Visual inspection confirms the valve, character, cargo, rails, water and HUD remain distinct, but that judgment is provisional/human-gated.
- Next smallest task: deterministic host-owned emergency pacing with cooldown/anti-stack rules and a seeded soak (CC-203), followed by the explicit one-host/two-client technical stress task (CC-204).
