# Crew R8 import attempt — 2026-09-11

Фаза 4 очереди. Цель: импорт в отдельный `/Game/Characters/CrewR8`, переключение runtime, проверки. R6B Content не удалялся.

## Done

1. **Import** `Scripts/import_crew_r8.py` via full Editor `-ExecutePythonScript`.
   - Log: `Saved/Logs/Import-CrewR8-20260911-184200.log`
   - `CREW_R8_IMPORT_PASS meshes=20 maps_changed=0`
   - Editor exit **3** — known `SemanticSearch.dll` AV on quit (same as R6). Assets on disk: 20 SM + 20 materials under `Content/Characters/CrewR8`.
2. Temporary C++ path switch CrewR6B → CrewR8 in `CalmyCrewPresentation.cpp` / `CrewLowPolyTests.cpp`.
3. Build Succeeded. Automation **42/42** with R8 paths.
4. Network: **Helper / Water / Ship PASS** with R8 meshes.
5. Character Host: first fail `bodycam_invalid` (stale probe FOV 102 vs preferences FOV). Probe updated to preference-aligned FOV (kept).
6. Character Host after FOV fix: **FAIL** `timeout stage=1` — stuck `Recovering`, `RecoveryCount=0` ~100s (`20260911-185152-Character`). `CanFinishRecovery` requires torso within 5 cm of hardcoded `CrewRig::Bind` and <12° — R8 proportions/bind mismatch with R6B pivots.

## Revert (careful)

Runtime paths restored to **CrewR6B**. `Content/Characters/CrewR8` retained as imported candidate. Rebuild + Character retest documented below after this report.

## Not done (deferred)

- Bind/pivot retarget for R8 or recovery tolerance for new mesh sizes
- Character Host/Client/LateJoin PASS on R8
- GPU review of R8
- Grips / fingers / skin
- Water floating proxy
- New Windows package

## Evidence

| Item | Path / result |
|---|---|
| Import log | `Saved/Logs/Import-CrewR8-20260911-184200.log` |
| Character fail (FOV) | `Saved/Verification/20260911-184450-Character` |
| Character fail (recovery) | `Saved/Verification/20260911-185152-Character` |
| Helper/Water/Ship PASS (R8) | `Saved/Verification/20260911-184505-Helper`, `...-184600-Water`, `...-184732-Ship` |

DLL during R8 switch builds: `FA9B9C87…` then `8B7CEBAB…` (probe FOV fix). Final DLL after R6B revert + Character retest PASS: `b746b5313878d54eb3f6abefe8a7cb786734bcfb4e9d6f3f35b7bbb9d56174f9`. Character evidence after revert: latest `Saved/Verification/*-Character` with NETWORK_SMOKE_PASS.
