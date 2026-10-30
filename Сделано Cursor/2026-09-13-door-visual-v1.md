# 2026-09-13 — Двери V1 (watertight)

Все 61 дверь: вместо синих кубов — `SM_WatertightDoor` (панель + штурвал), лучше садятся в закрытом состоянии.

## Сделано
- C++ `ACalmyDoor` → kit-меш; seal +16 см, hinge embed 5 см, floor nudge 4 см
- Скрипт карты `Scripts/upgrade_door_visuals_v1.py` (61 дверь)
- Backup: `SourceAssets/Backups/Before-DoorVisual-V1-20260913/`
- Кадры: `Saved/Verification/Doors-Visual-V1-20260913/door-*-final.png`
- Отчёт: `Docs/Changes/DOOR_VISUAL_V1_2026-09-13.md`

## Проверки
- Automation **54/54**
- Door Host/Client/LateJoin **PASS**
- Door Visual **PASS**

DLL `0F15C432…`

## Не сделано
- Пер-дверные offsets / замена рам Blender
- R7-ART-03 стены коридора
