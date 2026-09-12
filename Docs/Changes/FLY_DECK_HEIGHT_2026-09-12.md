# Fly deck height — отдых на палубе, полёт ~1 м

Пользователь попросил, чтобы мухи при отдыхе/ходьбе сидели у земли, а в полёте держались примерно на метр над палубой, и чтобы «сознание» опиралось на уже вложенный скан мозга (нейроны и связи).

## Высота

- `FlightHeightCm = 100`, `WalkClearanceCm = 4` на `AFlySwarmMob`.
- `FindDeckBelow` — visibility-trace вниз на walkable deck (normal.Z ≥ 0.65).
- **Rest:** колония мягко садится на `FloorZ + WalkClearanceCm`; особи pin’ятся к той же палубе (sample от лидера, чтобы не нырять в нижний ярус через решётку). Шаг Z ограничен (~6 см) и только при `CanFlySegment`.
- **Flight:** цели пути берут deck-relative `AirZ` (на home-палубе не ниже authored spawn `HomePosition.Z`, чтобы коридоры HOLD оставались достижимы). Per-tick hard Z-snap убран: он пробивал тонкие проёмы и валил FlyTravel (`fly_crossed_collision` / `leader_crossed_collision` / раздутый spread).
- Моторный облачный слой R6 сохранён: общий TravelVelocity + formation; pin высоты в полёте не навязывается каждой особи отдельно.

## Коннектом / «сознание» (честный статус)

Уже было и не менялось этой итерацией:

- Встроенный FlyWire circuit **668 нейронов / 18 968 рёбер** (`FlyConnectomeData.inl` ← `SourceAssets/ThirdParty/DesktopFly/circuit.json`).
- На каждую муху — отдельный LIF SNN по этому графу (моторные rates).
- `FFlyCognition` — отдельный колониальный слой нужд/памяти/занятий поверх SNN.

Это **реальная симуляция цепи и утилитарной когниции**, не доказанное феноменальное сознание и не «разблокировка» нового мозга. Новый полный connectome / STDP / свободный диалог не добавлялись.

## Проверки

Финальная DLL: `6AB74606A652B4C3F5E5CA9E1EF890C46D398237E638AFE1D15659DAD43B2AC9`.

| Проверка | Результат |
|---|---|
| FlyTravel Host/Client/LateJoin | **PASS** — `Saved/Verification/20260912-000018-FlyTravel` (`left_hold=1`, walked≈11003, spread=230, rest=1) |
| FlyMind Host/Client/LateJoin + Restore | **PASS** — `Saved/Verification/20260912-000306-FlyMind` |
| Automation `CalmyPiratesCo.Fly*` | **7/7 PASS** вкл. `FlySwarm.Connectome` — `Saved/Verification/20260912-000427-FlyAltitude-Auto.log` |

Ранние неудачи той же ветки (не приняты): `20260911-232955`…`235530-FlyTravel` — collision/spread от агрессивного altitude pin; логи оставлены.

## Границы

Maps/mesh/package не менялись. Packaged build не обновлялся. Высота — deck-relative bias целей + rest settle, не гарантия ровно 100 см в каждом отсеке с низкой потолочной геометрией. GPU visual review этого прогона не снимался.
