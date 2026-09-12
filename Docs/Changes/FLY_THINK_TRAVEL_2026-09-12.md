# Fly think + ship roam

Пользователь попросил, чтобы рой **«думал»** и путешествовал по кораблю.

## Поведение

- Путешествие R6 сохранено (`bRoamShip` + поиск пути).
- Перед/после выбора маршрута колония говорит travel-thoughts (`DescribeTravelThought`): «I am thinking…» / «I have a path…».
- HUD: `SHIP RESIDENT | ACTIVITY · THINKING` пока идёт поиск (`bTravelPlanning`).
- Цели: прежняя длина роума ~900 см; мягкий nudge от меток (away wary / toward welcome) и лёгкий уход от недавно посещённых клеток — без жёсткого Directed (он раздувал spread).

## Проверки

Финальная DLL: `D7D175ED235BF0A6DC66FE476E7EC1A3F6B416BFC37A987C58F9C4138B1467C0`.

| Проверка | Результат |
|---|---|
| FlyTravel Host/Client/LateJoin | **PASS** — `Saved/Verification/20260912-155421-FlyTravel` (spread=85) |
| FlyMind Host/Client/LateJoin + Restore | **PASS** — `Saved/Verification/20260912-155638-FlyMind` |

Ранний FAIL той же ветки: spread>350 от агрессивного scoring/Directed — откатан к soft nudge.
