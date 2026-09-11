> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Cargo behavior increment — 2026-09-02

Engine: Unreal Engine 5.8.2. Scope: local Windows Development, not Shipping.

## Implemented

One server-owned holder/reservation remains authoritative. Added replicated cargo kind and condition snapshots; bulky proximity assistance / movement penalty / throw denial / separation drop; unstable depletion, rest recovery and Q handoff; fragile collision damage, tint and synthesized impact cue; damaged-delivery contract counter. Overboard reset preserves condition; new-round reset clears it. No client can submit integrity, stability or delivery totals.

The new handoff RPC documents ownership, range/world/visibility, lifecycle/empty-hands checks, sequence/rate replay protection, per-cargo cooldown, late join and disconnect behavior. It transitions reservation ownership only after validation and publishes the new holder once; it is not a second ownership system.

## Files

- `Source/CalmyPiratesCo/Public/Gameplay/CalmyCargo.h` and `Private/Gameplay/CalmyCargo.cpp`: kinds, condition, helper detection, transfer, damage and presentation.
- `Public/Gameplay/CalmyCharacter.h` and `Private/Gameplay/CalmyCharacter.cpp`: Q intent/RPC, bulky movement speed and held collision sweep radius.
- `Public/Gameplay/CalmyShift.h`, `Private/Gameplay/CalmyShift.cpp`, `Private/Gameplay/CalmyGameMode.cpp`: damaged-delivery count and contextual HUD.
- `Private/Tests/CargoConditionTests.cpp`: authoritative behavior and contract tests.
- `Public/Testing/CalmyCargoProbe.h`, `Private/Testing/CalmyCargoProbe.cpp`: non-shipping real multi-process regression.
- `Content/Maps/CargoLab.umap`, `Scripts/create_cargo_lab.py`, `Config/DefaultGame.ini`: new opt-in co-op map and cooking entry; old maps preserved.
- `Scripts/run_network_smoke.ps1`, `run_automation.ps1`, `start_game.ps1`: Cargo suite, minimum ten automation tests, map selection.

## Verified

- C++ Editor target compiled and linked successfully. The first link attempt was blocked by the prior turn's unattended map-check process, not a compilation error. That exact owned test process was stopped, then linking passed; no user editor was force-killed.
- `Saved/Logs/Automation-20260902-115711.log`: **10/10 automation tests pass**, wrapper exit 0 and `AUTOMATION_PASS`.
- `Saved/Logs/CreateCargoLab.log`: map generated through Unreal APIs, 0 errors / 0 warnings, process exit 0. Map duplication check also reports 0 errors / 0 warnings.
- `Saved/Verification/20260902-115745-Cargo`: **Host / Client / LateJoin all PASS**, runner exit 0. Covers solo bulky rejection, real slowed carry, throw denial, actual helper movement away and forced drop, unstable handoff / duplicate rejection / immediate reverse cooldown / forced drop / rest, real fragile throw collision and damaged delivery. Late join resolves kind and condition history snapshots. No test-only teleports, stat assignment or authority shortcuts were used in this network probe.
- `Saved/Verification/20260902-115912-Carry`: original carry/throw/impact/recovery/disconnect regression **Host / Client / LateJoin PASS**, runner exit 0.
- `Saved/Verification/20260902-115922-Shift`: original three-crate delivery, overboard reset and host-only repeat-shift regression **Host / Client / LateJoin PASS**, runner exit 0.
- `Saved/Logs/Package-20260902-115853.log`: **BUILD SUCCESSFUL**, UAT and wrapper exit 0, **PACKAGE_PASS**. New independent archive: `Builds/Windows-20260902-115853`; earlier verified archive remains unchanged.
- `Saved/Verification/20260902-120132-Cargo`: **packaged executable Host / Client / LateJoin PASS**, runner exit 0. All three final packaged logs have no `Error:` or `Warning:` lines. This validates the cooked map and new gameplay outside the editor.
- `Saved/Verification/20260902-115745-Cargo/CargoLab-Gameplay.png`: actual PIE rendering with a wider yellow bulky crate, blue unstable crate, turquoise fragile crate and the contextual condition HUD. The image was captured through Unreal's viewport API. Further keyboard/audio inspection was not performed because a Windows Firewall permission prompt overlaid the desktop; no security permissions were changed.
- Unreal was reopened on CargoLab and PIE was stopped after visual inspection, ready for editing. Blender and Visual Studio were retained. Tests did not close the user's old packaged solo window.

Final hashes:

- CargoLab map: `63CD1C89727DEB25BABD6B8674075D1C0EF52225E60F2221AE2B68C835CF5E97`.
- Editor module: `00E2017C9C9C032BD0A9DFD24D9C1415C751D8631CF9BE092A3D1784EEAC8FFB`.
- Packaged inner executable: `093472C8BD852BDDCB35B2EA608D940BAD507564477A4756AFA5D5D0119B41DC`.
- DeckTest and ShiftTest hashes still match the previous verification exactly: neither existing map was overwritten.

## Reproduce

Close the editor normally before rebuilding C++. Do not leave an unattended editor/map-check process holding the DLL.

```powershell
& 'E:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CalmyPiratesCoEditor Win64 Development 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -WaitMutex
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_automation.ps1'
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Cargo -Port 17781
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Carry -Port 17777
& 'E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1' -Suite Shift -Port 17779
& 'E:\Game\CalmyPiratesCoUE\Scripts\package_windows.ps1'
```

## Boundaries

- No human fun-gate claim. Two-human evaluation, full special-cargo contract playthrough, four-player soak, lossy-network behavior, accessibility and audible/perceptual mix evaluation remain unverified.
- Bulky assistance is proximity-based and single-holder, not a physical two-grip rig. Greybox art and simple physics replication remain.
- The impact cue is procedural placeholder audio, not final sound design. Automated network runs disable audio and do not prove audible playback quality.
- Old ShiftTest remains the default solo-playable map. CargoLab intentionally requires another player for bulky cargo; no hidden AI helper or solo bypass was added.
- Next smallest gameplay task: complete and instrument a full mixed-cargo shift with two human players, then add one recoverable voyage emergency/repair interaction.
