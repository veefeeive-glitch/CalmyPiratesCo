# Remaining fixes after the Cursor review — 2026-09-13

## Scope

Исправление известных проблем из [Cursor review fixes + R7 service spaces S1](CURSOR_REVIEW_FIX_AND_SHIP_S1_2026-09-13.md): smoke Settings и Melee, звук роя у клиентов, лимит в четыре игрока при одновременном входе, урон по своим, RPC-контракты вахты, тёмный цепной ящик и бледная табличка офиса.

## Fixed

| Проблема | Причина | Исправление |
|---|---|---|
| Smoke Melee: `wheel_hover_failed` | Колесо действий стало восьмисекторным, проба наводилась вправо (сектор 2) и ждала сектор 1 | Проба наводится в верхний правый сектор «be wary» (и мышью в GPU-режиме) |
| Smoke Settings: `applied_controls_graphics_or_saved_preferences` | В UE 5.8 `UGameUserSettings::SaveSettings` вызывает `ReloadWindowPositions` → `LoadConfigIni`, который заново вычисляет `GGameUserSettingsIni`. Подмена глобального пути в пробе сбрасывалась на настоящий `Saved/Config/WindowsEditor/GameUserSettings.ini`, и проверка читала его. Сохранение в игре исправно | Smoke и визуальный прогон Settings запускаются с временным `-UserDir` в папке доказательств; без него проба сразу падает (`needs_isolated_UserDir`). Проверка читает сохранённый файл с диска (`FLoadConfigParams::OverrideFile`) и пишет диагностику при провале |
| Клиенты не слышат удар по рою и ответ роя | Звук играл только на сервере | Сервер ставит `SwatAudioAt` / `VoiceAudioAt` по серверным часам; клиент в OnRep играет звук только для свежей метки (≤1.5 с), поэтому подключившиеся позже молчат. Smoke Melee требует `client_swat_audio` и `client_voice_audio` |
| Лобби могло принять пятого игрока | `PreLogin` считал только `GetNumPlayers()`, клиенты в загрузке не учитывались | `FCalmySession::SeatsTakenAtPreLogin`: локальные игроки хоста + открытые клиентские соединения, кроме входящего, но не меньше `GetNumPlayers()`; automation `SessionLanHelpers` |
| Удары по своим выбивали игрока, телепортировали и снимали $125 | Любой удар игрока наносил 6 урона | `FCalmyMeleePolicy`: игрок по игроку — толчок без урона (вспышка и счётчик удара остаются), игрок по Мише — 6, Миша в ответ — 12; automation `ProvocationAndTargeting` |
| Неполные RPC-контракты `ServerBeginVoyage`, `ServerRestartVoyage`, `ServerInscribeSwarm` | — | Описаны вызывающий, проверки, дубликаты, поздний вход и отключение |
| Цепной ящик: тёмно, цепь из брусков | Эмиссивная лампа не освещала помещение; звенья были боксами | Звенья — овальные кольца (26 в каждом колодце, 4 свисающих); установщик добавляет три точечных света (цепной ящик, аварийный генератор, провизионная), копируя параметры `Nightwatch_Service_Fill`, прикреплены к `ShipMotion_R7` |
| Бледная табличка «SHIP'S OFFICE» | Белые буквы на светлой переборке | Тёмная табличка и светлые буквы крупнее |

## Verified (this machine)

| Check | Result |
|---|---|
| Editor build / game target (без редактора) | PASS / PASS |
| Automation | **56/56** `Saved/Logs/Automation-fix2-20260913-195746.log` |
| Settings Host/Client/LateJoin + Visual | **PASS** `Saved/Verification/20260913-195811-Settings`, `Settings-S1-20260913-203224` |
| Melee (client_swat_audio=1, client_voice_audio=1) | **PASS** `20260913-200029-Melee` |
| Session (4 места) | **PASS** `20260913-200109-Session` |
| Tutorial, Shift, Leak, Ship, Door, Carry, BridgeMenu | **PASS** `20260913-2001xx…2011xx` |
| Cargo, Interior, Water, WaterInteraction, Character, Helper, Pause, Interaction, FlyMind | **PASS** |
| HUD review, кадры S1 со светом | **PASS** `HUD-Review-20260913-195746-fix2` (12 кадров) |
| OceanFreighterR7.umap SHA256 | `ee3b80e11b9f519440f960e598a4ca78878477de690ce8453b4c0ffddb5dda64` |
| Master blend SHA256 | `8b4fe72d…` не изменён |

FlySwarm и FlyTravel разобраны в [супер-аудите](SUPER_AUDIT_2026-09-13.md).

## Not done / known

- Traversal audit R6 (293/336) не перезапускался: это исследовательский Blender-аудит старой модели, маршрут R7 проверяет smoke Ship.
- Пакетная сборка (ZIP) не делалась по решению пользователя; сборка игрового таргета без редактора проверяет только компиляцию.
- Диагностические логи Tutorial probe (`DIAG`, `DROP_EVENT`, `Host CARGO`) оставлены.
