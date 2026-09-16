# Voyage G3b + cargo HOLD 1 — 2026-09-13

## Intent

Start the “real game” loop on OceanFreighterR7: long watch (~18 min), storm/leak/fuel complications, crates stowed in HOLD 1 and delivered to the weather-deck green zone.

## Changes

### Cargo → HOLD 1
- Map actors relocated to `(-1150/-950/-750/-550, -420, 320)`; labels `R7_Hold1_Cargo_*`.
- Backup: `SourceAssets/Backups/Before-CargoHold1-20260913/OceanFreighterR7.umap`.
- Scripts: `Scripts/relocate_cargo_hold1.py`, `Scripts/verify_cargo_hold1.py`, `import_ship_r7_game.py` spawn updated for new maps.
- Verify log: `CARGO_HOLD1_VERIFY_PASS n=4` in `Saved/Logs/CalmyPiratesCo.log`.

### Voyage director G3b
- Watch duration default **1080 s** (`WatchDurationSeconds`).
- Curated manifest: **Standard + Bulky + Fragile + Unstable** (4).
- Scheduled storm windows via `ACalmyOcean::SetStormIntensity` + `ACalmyShipMotion::SetStormBoost`.
- Fuel meter drains over the watch (faster in storm); `ACalmyFuelStation` near engine (`-3550,280,330`); hold **F** to refill.
- Fuel starvation fails the watch.
- Bridge WATCH screen shows fuel/storm; briefing copy updated.

### FlyTravel P0
- Travelling colony cloud hard-clamped to `ColonyRadius` ≤70 cm (spread inflation / collision tunnel).

## Gates

| Check | Result |
|---|---|
| Automation | **54/54** `Automation-20260913-015924.log` |
| Cargo HOLD1 verify | **PASS** |
| Ship / Door / Cargo / Carry / BridgeMenu | **PASS** (`POST-G3B-SMOKE-20260913.txt`) |
| FlyTravel Host/Client/LateJoin | **PASS** `20260913-015700-FlyTravel` (spread Host 228) |

DLL: `C53C5189A21F92174C93130D73434B39D0BFAF0329B3ADF25AE852188A9703E4`.

## Out of scope
Port / mission board, Steam, Chaos flooding, full Blender R7-ART-01/02, packaged promote. Melee `wheel_hover` / Tutorial START / Settings preference — P1 backlog from regression sweep.
