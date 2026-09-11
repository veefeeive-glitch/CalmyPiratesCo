# Grip R2a — kind-specific handles + moving wrist — 2026-09-11

Узкий первый срез Grip R2. Не пальцы/skin, не mesh sockets в Content, не второй живой игрок, не двери/встречный поток.

## Изменения

- `FCalmyCargoGrip` (`Public/Core/CargoGrip.h`): отдельные локальные точки для Standard / Bulky / Unstable / Fragile, смещённые к R3 fittings из `Scripts/create_crew_cargo_r3.py`, но в **actor centimetres** с одним применением масштаба коллизии (как Grip/Health R1).
- Первая попытка через `CargoVisual` mesh-local дала static wrist ~12 см (недостижимо); сохранена как диагностика `LP4-Visual-20260911-222015`, критерий не ослабляли.
- `ACalmyCargo::GetGripPoint` берёт kind-specific locals вместо одного proxy `(±50, ±30, 12)`.
- GPU GripViews: после статичного кадра 2.5 с yaw ±40° у держателя; порог moving wrist **5 см** (static по-прежнему **3 см**).
- Automation: `CalmyPiratesCo.Cargo.HandleGrips` (различие near/far/left/right и высоты Standard vs Fragile).

## Проверки

| Проверка | Результат |
|---|---|
| Build Editor Development | PASS |
| Automation | **43/43** `Saved/Logs/Automation-20260911-222321.log` |
| Helper Host/Client/LateJoin | PASS `Saved/Verification/20260911-222440-Helper` |
| GPU LP4 + GripViews | PASS `Saved/Verification/LP4-Visual-20260911-222341` (10 PNG) |

Метрики grip (Standard crate fixture):

- static player **1.169** см, joints 0.005
- moving player **1.179** см
- with helper: player **1.375** / helper **0.762** см

DLL SHA256: `A935E36DB99F84CC9D41FAE0A154AA41AC6A2394CF41A789CCE19F1D0A24DE42`

## Ограничения

- Руки у боковых граней рядом с ручками, не обхват пальцами и не точные socket на меше.
- Moving probe — только yaw держателя на месте, не ходьба/двери/bulky поток.
- Ship/Water/Character network на этой DLL не повторялись (Helper покрывает carry).
- Пальцы/skin, import sockets, второй игрок, package — следующие срезы.

Карты/Blender masters/Content cargo meshes не редактировались. Frozen Evidence GripHealth-R1 не трогали.
