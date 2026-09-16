# Door visual V1 — watertight leaves — 2026-09-13

По запросу: двери выглядели ущербно (синие engine-cube) и плохо садились в закрытом состоянии. Все **61** проходные `ACalmyDoor` на OceanFreighterR7 переведены на kit-меш `SM_WatertightDoor` (панель + inset + штурвал), с посадкой в раму.

## Изменения

- `ACalmyDoor`: полотно = `/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor`; кубические ручки скрыты (штурвал уже на меше).
- Закрытая посадка: oversize **+16 см** по ширине/высоте, floor nudge **4 см**, hinge embed **5 см** в косяк; толщина визуала ~8 см. Коллизия/геймплей Width/Height прежние.
- Материалы kit: `MAT_Interior_EquipmentBlue` / Machinery / Safety (сброс старых blue-cube overrides).
- Карта: `Scripts/upgrade_door_visuals_v1.py` + `run_upgrade_door_visuals_v1.ps1`; backup `SourceAssets/Backups/Before-DoorVisual-V1-20260913/`.
- Map SHA256 после embed: `b15a4221e5c545c91b9f09e28002ffc998779505cb6127787285d573767c5a68`.

## Проверки

| Проверка | Результат |
|---|---|
| Automation | **54/54** |
| Door Host/Client/LateJoin | **PASS** (`20260913-022010-Door`, затем smoke после embed) |
| Door Visual Solo | **PASS** — кадры `Saved/Verification/Doors-Visual-V1-20260913/door-*-final.png` |

DLL: `0F15C43231E79AC5B2BFBDCE8CE5605AF17A6413C57DBED098C998905BEE95C4`

## Границы

- Не новый Blender master / не замена рам superstructure DoorsR1.
- Не звуки, замки, герметизация затопления.
- Отдельные проёмы могут ещё давать тонкий зазор (геометрия рам R7 разная); дальнейший dream-loop / per-door offsets — backlog.
- R7-ART-03 (плоские стены коридора) не закрыт этим проходом.

## Повтор

```powershell
& ./Scripts/run_upgrade_door_visuals_v1.ps1
& ./Scripts/run_network_smoke.ps1 -Suite Door -Port 17930
# Visual: -game OceanFreighterR7 -CalmyDoorProbe=Visual
```
