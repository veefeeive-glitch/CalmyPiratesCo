# FlyWire ← inscriptions — метки бьют в спайки

После общих Dark Souls-меток пользователь продолжил («го»): связать их с **реальным LIF/FlyWire**, а не только с utility cognition.

## Связка

Общий mood меток (`InscriptionCaution` / `InscriptionWelcome`) теперь входит в каждый 1‑мс шаг мозга:

| Канал | Откуда | Куда в графе (как в desktop-fly) |
|---|---|---|
| Loom gain | wary ↑ / welcome ↓ | LC4/LPLC2 × `SensoryGain` |
| Vigilance | wary | слабый двусторонний loom даже без угрозы рядом |
| AirPuff | wary | Sensory JO (индексы 432–447) |
| AscendDrive | welcome | Ascending (350, 392…) |

`SensoryMultiplier` по игроку тоже умножается на colony mark-gain. MaleCNS не подключался.

## Проверки

Финальная DLL: `5D622F47ABAB96366AECB9134DEBC6D4A3C331F0AB79936CEAC5B0890FD1ED80`.

| Проверка | Результат |
|---|---|
| Automation FlyMind + FlySwarm | **PASS** (вкл. `InscriptionsDriveFlyWire`) |
| FlyMind Host/Client/LateJoin + Restore | **PASS** — `Saved/Verification/20260912-153241-FlyMind` |

## Границы

Это модельная инъекция в уже экспортированные сенсорные/ascending пути, не новые синапсы из лаборатории и не феноменальное сознание. Maps/package не менялись.
