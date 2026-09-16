# Voyage Loop G4 — 2026-09-13

Порт → миссия → погрузка → море → порт. Playability gate + PortHub + mission board + G3b wire + return.

## Сделано

### Фаза 0 — Playability gate
- **0a Menu:** hover не крадёт `Selected` после Open / после стрелок (`bHoverArmed`, 10 px arm). Enter всегда по keyboard-selected. Evidence: live harness menu shot с BEGIN WATCH выделенным.
- **0b Doors V2:** `SealOversizeCm=24`, `FloorNudgeCm=-3`, `HingeEmbedCm=8`, thickness 10. Door network smoke PASS.
- **0c Live HOLD1:** `Scripts/run_live_hold1_route.ps1` + `Scripts/play_harness.py`; `-CalmyLiveHold1` teleport HOLD1 → после grab → green zone. Markers `CALMY_LIVE_HOLD1 teleported_to_hold1` / `teleported_to_green_zone`.

### Фаза 1 — PortHub
- Карта `/Game/Maps/PortHub` (pier + warehouse + MissionBoard + P0–P3).
- Tutorial destination / auto-board → **PortHub** (R7 остаётся для ship smoke).
- Allowlist `FCalmySession::IsAllowlistedVoyageTravel`: PortHub ↔ OceanFreighterR7 only.

### Фаза 2 — Mission board
- `ACalmyMissionBoard` + `FCalmyVoyageContract` / GConfig slot `Calmy.VoyageContract`.
- Catalog: short / standard_g3b / heavy. Host accept → save → ServerTravel R7.
- Up/Down/Enter/E near board.

### Фаза 3 — Wire G3b + return
- `ACalmyVoyageDirector` читает accepted contract (duration, curated counts, storm/leak/fuel flags).
- Outcome primary: **RETURN TO PORT** → PortHub + clear contract. R / Shift restart = NEW WATCH на месте.
- Automation: `CalmyPiratesCo.Gameplay.VoyageLoopPort` + VoyageCycle.

### Фаза 4 — Route readability
- Runtime markers + point lights HOLD1 ↔ green zone approach (`EnsureHoldRouteAids`).

## Проверки
| Check | Result |
|---|---|
| Automation | **55/55** (`VoyageLoopPort` + prior) |
| Door / BridgeMenu / Ship / Cargo / Carry / FlyTravel | **PASS** (`Saved/Verification/G4-SMOKE-20260913.txt`) |
| Live HOLD1 harness | hold=True green=True |
| PortHub map | `PORTHUB_MAP_PASS` |

DLL SHA256: `50876B809AB9B85C96ABF401125076435B22BF6FCD3DEE6F31242FF70AB98628`

## Не сделано / backlog
- Steam invites, Chaos flood R7, Blender R7-ART reimport, Shipping package promote, human fun gate.
- Full unaided walk HOLD1→green without `-CalmyLiveHold1`.
- Tutorial/Settings/Melee P1 backlog без изменений.

## Ключевые пути
- `Source/.../CalmyBridgeTerminal.*`, `CalmyDoor.cpp`, `CalmyVoyageContract.*`, `CalmyMissionBoard.*`, `CalmyVoyageDirector.*`, `CalmySession.*`, `CalmyGameMode.*`, `CalmyTutorial.h`
- `Content/Maps/PortHub.umap`
- `Scripts/create_porthub_map.py`, `run_live_hold1_route.ps1`, `play_harness.py`
