# Tutorial progress save + menu replay — 2026-09-11

После прохождения обучения факт сохраняется локально; следующий обычный запуск сразу ведёт на корабль. Из меню рубки можно пройти обучение снова.

## Изменения

- `[Calmy.Progress] TutorialCompleted` в `GameUserSettings.ini` через `UCalmyPreferences` (не CPF_Config draft настроек — сброс графики не стирает прогресс).
- При шаге `Depart` (сервер и OnRep у клиентов) вызывается `ACalmyTutorial::MarkCompletedLocally`.
- Обычный запуск WaterLab при сохранённом прогрессе: `ShouldAutoBoard` → relative `ServerTravel` на OceanFreighterR7. Probes / `-CalmyLab` не затрагиваются.
- Меню терминала: **REPLAY TRAINING / ОБУЧЕНИЕ СНОВА** → `BeginForcedReplay` + travel на WaterLab (Server RPC для клиента). После повторного прохождения флаг force снимается, completion остаётся.

## Проверки

| Проверка | Результат |
|---|---|
| Build | PASS |
| Automation | **44/44** `Saved/Logs/Automation-20260911-230652.log` |
| Tutorial Solo NullRHI | PASS `Tutorial-Solo-20260911-230726` |
| BridgeMenu Visual | PASS `BridgeMenu-Visual-20260911-230805` |

DLL SHA256: `0D6DBDE58F18A43A0A37749BCE3BB480F484F6399E522232C6B1DE4E6EB3D9EE`

BridgeMenu Host/Client/LateJoin: PASS `Saved/Verification/20260911-230854-BridgeMenu`.

## Ограничения

- Это локальный прогресс обучения, не save всей смены/корабля/сессии.
- Повтор обучения переносит всю сессию на WaterLab (host travel).
- Human acceptance и новый package не заявлены.
