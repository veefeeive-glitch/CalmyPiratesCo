# Ship G2 storm / proxy / helm — 12 сентября 2026

Потоковая задача Sea/Ship G2: штормовой отклик, отдельный physics proxy, based-movement boarding, helm intent — поверх W1 + существующего `ACalmyShipMotion`. Characters / voyage GameMode / net UI не трогались (кроме минимального join-member restore и `Sockets` в Build.cs, чтобы параллельный G4 Session линковался).

## Реализовано

### Ocean storm scale
- `ACalmyOcean::StormIntensity` (0..1, replicated) + `StormWaveGain` / `StormFlowGain`.
- Analytic swell (`HeightAt` / `VerticalVelocityAt` / mesh / flow) масштабируется; W1 disturbances без умножения.
- `StormScale()` — чистая математика для automation.

### ShipMotion storm + helm + boarding
- Лимиты heave/heel/drift растут от `SeaStrength` packet **и** `StormBoost` (0..1, replicated).
- `SetHelmInput(throttle, rudder)` — серверный surge вдоль rest-forward + ограниченный yaw (`MaxHelmYawDeg`).
- `BoardingRamp` — walkable StaticMesh ramp (starboard midships) для based-movement с воды на палубу; без правок Character.
- По-прежнему кинематический ride R7 по полю W1; не Chaos на полном меше.

### Hull proxy
- Новый `ACalmyHullProxy`: Chaos box, mass 1200 kg, COM ниже палубы, `ApplyBuoyancy(..., bIgnoreDryHull=true)`, walkable `Deck`.
- Install script: `Scripts/install_hull_proxy_waterlab.py` / `run_install_hull_proxy_waterlab.ps1` (на момент отчёта install SKIP — занятый Unreal другими потоками).

## Проверки

DLL SHA256: `0128BC980D14E8ED9A121EEE0BD2F02C4D6671D8D2F68D5750F10726F9BF0DB4`

| Проверка | Результат |
|---|---|
| Development Editor build | **Succeeded** (`Saved/Verification/20260912-G2-Build5.log`) |
| Automation `CalmyPiratesCo` | **52/52 PASS** — `Saved/Logs/Automation-G2-20260912-1831*.log` |
| `CalmyPiratesCo.Ship.HullProxyDefaults` | PASS |
| `CalmyPiratesCo.Ship.MotionLimitsAndHelm` | PASS |
| `CalmyPiratesCo.Water.StormScale` | PASS |
| Water Waves / DryHull / Disturbance | PASS |

Editor Play / WaterLab proxy spawn / Ship Host–Client smoke в этой сессии не повторялись: параллельные агенты держали Unreal занятым. Boarding ramp появляется с CDO на существующем `ShipMotion_R7` при следующем Play.

## Границы

- Не полный fluid solver / Chaos R7 / затопление через двери.
- Helm — C++ API (`SetHelmInput` / `SetStormBoost`), не bridge console binding и не Character input.
- Boarding ramp — геометрический шаг для based movement; не climb system и не «спасение с воды» в Character.
- Human fun gate не заявляется.
- Concurrent interference: G1/G3/G4/Stabilize на том же worker несколько раз откатывали Ocean/ShipMotion mid-edit; финальные исходники зафиксированы в дереве + snapshot `internal/g2-sea-ship-sources` агент-store.

## Следующий безопасный шаг

1. Когда Editor свободен: `Scripts/run_install_hull_proxy_waterlab.ps1`, затем Play WaterLab — стоять на proxy при `StormBoost`.
2. Play OceanFreighterR7: `SetStormBoost(0.6)`, `SetHelmInput(0.4, 0.2)`, проверить ramp/based movement.
3. Повторить Ship/Water Host–Client smoke на новой DLL.
