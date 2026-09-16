# Replay training host-only — 2026-09-12

## Problem

Repo check 12 сентября: `ACalmyPlayerController::ServerReplayTutorial` принимал запрос от любого клиента без проверки роли, фазы и частоты и вызывал `ServerTravel` на WaterLab. Любой подключённый игрок мог перебросить всю сессию (до 4 человек) на обучение, в том числе посреди вахты и повторно.

## Implemented

- Server RPC `ServerReplayTutorial` удалён: у клиента нет входа, через который можно сменить карту сессии.
- `FCalmySession::CanRequestSessionTravel(NetMode, bLocalController)` — политика: только локальный контроллер в `NM_Standalone` или `NM_ListenServer`.
- `RequestReplayTutorial` использует политику; у клиента ничего не происходит, меню не закрывается.
- Меню рубки у клиента показывает строку как `REPLAY TRAINING (HOST)`.
- Automation `CalmyPiratesCo.Core.SessionLanHelpers`: solo/listen host разрешены; удалённый контроллер на хосте, клиент, dedicated server — запрещены.
- `UCalmyBridgeMenuProbe`: каждый клиент (Client/LateJoin) после resume жмёт replay; через 4 с проверяется, что карта OceanFreighterR7, forced replay не выставлен, меню закрыто. Хост при этом должен дойти до PASS на той же карте.
- Попутно: `Scripts/__pycache__` (32 `.pyc`) убран из индекса Git, в `.gitignore` добавлены `__pycache__/` и `*.pyc`.

## RPC contract

Сетевого вызова нет. Caller — только local host/solo; receiver — он же; duplicate — повторный `ServerTravel` хоста как раньше; late join / disconnect — не затрагиваются, клиент не может инициировать travel.

## Verified (this machine)

| Check | Result |
|---|---|
| Editor build | PASS |
| Automation | **53/53** `Saved/Logs/Automation-replayfix-20260912-211758.log` |
| BridgeMenu Host/Client/LateJoin с правкой | **NETWORK_SMOKE_PASS** `Saved/Verification/20260912-212311-BridgeMenu`: Client и LateJoin `client_replay_ignored`, Host PASS |
| BridgeMenu с правкой, другие прогоны | FAIL Host `menu_teleported_pawn` на шаге 5, до нового шага: `20260912-211818-BridgeMenu`, `20260912-212237-BridgeMenu` |
| BridgeMenu на HEAD без правки | PASS `Saved/Verification/20260912-212020-BridgeMenu` |
| BridgeMenu после исправления пробы | **5/5 NETWORK_SMOKE_PASS** `20260912-215504`, `-215622`, `-215741`, `-215859`, `-220017`; Host `REBASE crew=1->2` в 4 из 5, сдвиг при входе клиента 4.3–10.1 см |
| DLL SHA256 | `A4DE1FF0E36395D1863C28DA8F700BDC7FA0C61318641DB06A1E4A8395004A9E` (после исправления пробы) |

## Probe flake — исправлено

`menu_teleported_pawn` — гонка в самой пробе: smoke запускает клиента после `Host START`, и если клиент входит примерно за 1.1 с до проверки шага 5, его пешка спавнится в рубке и сдвигает хоста больше чем на 10 см. В обоих провалах вход был за ~1.1 с до проверки, в успешных — за 0.2–0.7 с. Код хоста на этом участке не менялся. Исправление в `UCalmyBridgeMenuProbe`: при открытии меню запоминается число игроков (`GameState->PlayerArray`); если к проверке шага 5 оно изменилось, проба пишет `REBASE crew=… moved=…`, берёт новую точку отсчёта и ждёт ещё 2 с (не более 5 раз). Без входов проверка «сдвиг ≤ 10 см» прежняя. В сообщение о провале добавлены `moved` и `crew`. Запуск клиентов в `run_network_smoke.ps1` не менялся: при старте после `Host READY` LateJoin не успевает до выхода хоста на 65 с.

## Not done

- `SetActorLabel` в `CalmyVoyageDirector.cpp` (ломает сборку игры без редактора) — не трогали по решению пользователя.
- `Update-HandoverInventory.ps1` не перезапущен: требует PowerShell 7, на этом ПК его нет.
- Не fun gate; сборка игры/ZIP не делались.
