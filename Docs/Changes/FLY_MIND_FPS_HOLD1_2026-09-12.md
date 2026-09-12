# HOLD 1 FPS — fly mind vs NoBrain

Пользователь спросил, бьёт ли «мозг» мух по FPS. Один парный прогон того же стенда S1: неподвижная камера HOLD 1, Epic Quality 3, 1280×800, Development Editor `-game`, без других Unreal-процессов. В probe `bRoamShip=false`. DLL `D7D175ED…` (think+roam).

`-PerfNoBrain` отключает только нейронный шаг LIF/FlyWire в callback роя; cognition/надписи/parity-код остаются в билде.

| Прогон | FPS | Кадр, мс | p95, мс | Game, мс | Render, мс | GPU, мс | swarm_step, мс |
|---|---:|---:|---:|---:|---:|---:|---:|
| S1 baseline 2026-09-11 (`final-epic`) | 91.85 | 10.887 | 14.786 | 4.032 | 10.868 | 8.572 | 1.492 |
| С мозгом (`fly-mind-epic`) | **80.87** | 12.366 | 17.624 | 7.520 | 12.348 | 9.657 | **1.965** |
| Без нейронного шага (`fly-nobrain-epic`) | 75.48 | 13.248 | 19.732 | 7.611 | 13.228 | 10.262 | **0.445** |

Логи: [mind](../../Saved/Verification/Performance-S1-fly-mind-epic-20260912-155956/Performance.log), [NoBrain](../../Saved/Verification/Performance-S1-fly-nobrain-epic-20260912-160049/Performance.log).

## Вывод

- Нейронный шаг реально дороже: `swarm_step` ~0.45 → ~2.0 мс (+~1.5 мс).
- На FPS это **не главный тормоз**: оба сегодняшних прогона упираются в render/GPU (~12–13 мс), а NoBrain даже чуть медленнее по FPS — шум одного прогона, не «мозг убивает кадр».
- Versus S1 (91.85): ~11 FPS ниже при той же сцене; `game_ms` вырос ~4 → ~7.5 **и с мозгом, и без** — сдвиг от всего стека после S1 (drift/двери, presence, inscriptions, parity, think), не от одного переключателя SNN.

Не гарантия маршрута по кораблю, сети или 200 мух. Полный ship sweep не делался.
