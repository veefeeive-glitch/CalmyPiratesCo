# Regression sweep — 2026-09-13

Build: Development Editor up-to-date before suite. Automation **54/54** (`Saved/Logs/Automation-20260913-005420.log`).

## Network matrix (NullRHI Host/Client/LateJoin)

Evidence master: `Saved/Verification/REGRESSION-SWEEP-20260913-005458.txt`.

| Suite | Result | Notes |
|---|---|---|
| Helper, Character, Cargo, Carry, Water, Shift, Leak, Interior | **PASS** | |
| Ship, Door, WaterInteraction, BridgeMenu, Session, FlyMind | **PASS** | |
| Tutorial | **FAIL** | Host never reached `CALMY_TUTORIAL_TEST Host START` (60s timeout). Retry isolated: same. P1 load/hang — not voyage softlock. |
| Settings | **FAIL** | Host/Client `applied_controls_graphics_or_saved_preferences`. Retry same. P1 settings probe / preference apply. |
| FlyTravel | **FAIL** | Host `fly_crossed_collision` + spread>350 (up to ~975). Retry same on pre-fix DLL. |
| Melee | **FAIL** | Client `wheel_hover_failed`. LateJoin PASS. P1 wheel hover flake. |

Isolated retry: `Saved/Verification/REGRESSION-RETRY-20260913.txt` — 4/4 still FAIL.

## P0 actions

- **FlyTravel cohesion:** clamp travelling cloud to `ColonyRadius` (≤70 cm) in `FlySwarmMob.cpp` so individuals cannot inflate spread / tunnel bulkheads. Rebuild + re-smoke required after this report’s companion fix.

## Not fixed in this sweep (backlog)

- Tutorial Host START timeout on WaterLab.
- Settings preference apply assertion under NullRHI.
- Melee wheel hover under NullRHI.

Packaged promote **not** run. Traversal audit 293/336 known FAIL not re-run.
