# Ship drift G2 + doors closed — 12 сентября 2026

По запросу: все двери стартуют закрытыми; OceanFreighterR7 слегка дрейфует/качается от того же водного поля W1 (swell + disturbances + flow), без Chaos на полном меше R7.

## Двери

- C++: `bStartOpen=false`, `State=Closed`, `Angle=0` в [`CalmyDoor.h`](../../Source/CalmyPiratesCo/Public/Gameplay/CalmyDoor.h).
- Карта: [`Scripts/install_ship_drift_doors.py`](../../Scripts/install_ship_drift_doors.py) выставил `start_open=false` на всех **61** `ACalmyDoor`, backup в `SourceAssets/Backups/Before-ShipDrift-Doors-20260912`.
- Door probe: RPC-fixture явно стартует открытой (тест закрывает→открывает).
- Map SHA256: `a1acd33b3d517885d2900da313851c4d75a196e5f36b7f166acbec4b5f2d01aa`.

## Дрейф (G2 proxy)

Новый [`ACalmyShipMotion`](../../Source/CalmyPiratesCo/Public/Gameplay/CalmyShipMotion.h):

- Кинематический ride по **тем же** `HeightAt` / `VerticalVelocityAt` / `FlowAt`, что и cargo buoyancy (включая W1 disturbances).
- Лимиты: heave ≤70 см, heel ≤3.5°, drift ≤55 см/с, soft home spring, yaw зафиксирован.
- Репликация actor movement (server authority).
- Dry-hull: `WorldToDryHullLocal` переводит запросы в rest-frame через ship transform ([`CalmyOcean`](../../Source/CalmyPiratesCo/Private/Gameplay/CalmyOcean.cpp)); authored `DryHullOrigin` не мутируется.
- Визуал/коллизия: 72 StaticMesh/Door attached KeepWorld под `ShipMotion_R7`.
- World-space riders: `CarryWorldRiders` двигает flies (`ApplyShipMotion`), resting cargo, helpers без movement base; персонажи — `bIgnoreBaseRotation` + based movement по movable decks.
- Не делалось: Chaos на 1M+ tri R7, шторм, затопление через двери, рулевое управление.

`ApplyBuoyancy(..., bIgnoreDryHull)` оставлен для будущего physics-proxy; текущий ride кинематический по тому же полю.

## Проверки

DLL: `EB86E392231AE983D3CF2A46FAB3E4F680ED7750B3E903372867AE8B317276F6`.

| Проверка | Результат |
|---|---|
| Automation `CalmyPiratesCo` | **44/44 PASS** — `Saved/Verification/20260912-004913-ShipDrift-Auto.log` |
| Door Host/Client/LateJoin | **PASS** — `20260912-003959-Door` |
| Ship Host/Client/LateJoin | **PASS** — `20260912-004153-Ship` (37 waypoints, dry_checks, max_joint≈6.2 см) |
| Water Host/Client/LateJoin | **PASS** — `20260912-004355-Water` |
| WaterInteraction Host/Client/LateJoin | **PASS** — `20260912-004527-WaterInteraction` |
| FlyTravel Host/Client/LateJoin | **PASS** — `20260912-004646-FlyTravel` (`left_hold=1` при закрытых дверях) |

Install log: `20260912-003852-ShipDrift-Doors-Install.log` (`doors_closed=61 attached=72`).

## Границы

Package/Windows ZIP не обновлялся. Амплитуда дрейфа намеренно умеренная для проходимости Ship/FlyTravel. Полный physics proxy hull и boarding из воды — следующие G2 шаги. Арт R7-ART и grips вне этого прохода.
