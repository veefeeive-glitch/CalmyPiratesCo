# 2026-09-13 — Voyage Loop G4

## Что сделано
- Playability: menu hover-safe Begin Watch; Door V2 seating; live HOLD1 harness (debug teleport).
- PortHub map + MissionBoard (3 контракта) + allowlisted travel PortHub↔R7.
- VoyageDirector читает контракт; Outcome → RETURN TO PORT; route markers/lights на R7.
- Docs: `Docs/Changes/VOYAGE_LOOP_G4_2026-09-13.md`.

## Проверки
- Automation **55/55**
- Smoke Door/BridgeMenu/Ship/Cargo/Carry/FlyTravel **PASS**
- Live HOLD1: `teleported_to_hold1` + `teleported_to_green_zone`
- DLL `50876B80…`

## Не сделано
- Fun gate / Steam / Chaos / Shipping promote / walk без debug teleport.
