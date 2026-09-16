# Super audit — 2026-09-13

## Scope

Полная проверка `E:\Game\CalmyPiratesCoUE` после [оставшихся исправлений](REMAINING_FIXES_2026-09-13.md). Проверены:

- **сборка:** редактор и игровой таргет без редактора;
- **тесты:** все automation и все 21 сетевых suite, HUD review, визуальный прогон Settings;
- **статическая часть:** RPC-контракты и серверная валидация, editor-only API, OnRep-записи, тикающие акторы, жёстко заданные пути ассетов и карт, конфиги cook/travel, сохранения;
- **репозиторий:** гигиена git (stash, remote, LFS, `__pycache__`), документация.

## Findings and actions

| # | Серьёзность | Находка | Действие |
|---|---|---|---|
| 1 | Высокая (риск потери данных) | Весь цикл Voyage G4 (PortHub.umap, доска миссий, контракты, топливо, аудио, экраны рубки, S1 корабля, 12 отчётов) не закоммичен; у репозитория нет remote. Единственная копия — этот диск | Не коммитилось (нужно решение пользователя). Рекомендуется коммит + внешний remote/бэкап |
| 2 | Средняя | Automation `TutorialProgress` и `VoyageLoopPort` писали в настоящий `Saved/Config/WindowsEditor/GameUserSettings.ini` (значения восстанавливаются, но падение посреди теста оставит сброшенное обучение или чужой контракт) | `Scripts/run_automation.ps1` запускает редактор с временным `-UserDir` в `Saved/Verification/Automation-Profile-*` |
| 3 | Средняя | Smoke Settings: UE 5.8 `SaveSettings` → `ReloadWindowPositions` → `LoadConfigIni` заново вычисляет `GGameUserSettingsIni`, изоляция пробы подменой глобала не работала | `-UserDir` для Settings smoke и визуального прогона; проба без изоляции падает `needs_isolated_UserDir`; чтение с диска через `OverrideFile` |
| 4 | Низкая | Принятый контракт читался из ini целиком: правка файла хостом могла обнулить манифест, убрать шторм/течь/топливо или сократить вахту | `LoadAccepted` доверяет только `ContractId`, условия берутся из каталога; automation проверяет подменённые значения |
| 5 | Средняя (исправлено) | FlyTravel нестабилен с 12–13 сентября: муха застревала в стенке надстройки у X −30.8 м, часть роя отставала до 18.5 м. Причина — перенос «пассажиров» кораблём терял дельты меньше 0.05 за кадр и поворачивал их вокруг начала мира; дополнительно отрезанная муха откатывалась назад вместо шага вдоль следа | [Ship rider carry fix](SHIP_RIDER_CARRY_FIX_2026-09-13.md): `ConsumeRiderDelta`, накопление дельт, поворот вокруг корпуса, шаг вдоль следа; automation `RidersKeepSmallMotion`; FlyTravel ×3 PASS |
| 6 | Низкая | FlySwarm без `-FlyPlaced` (стресс 200 мух) зависал у клиента: runtime-тег `FlyStressTest` не реплицируется. С `-FlyPlaced` smoke по умолчанию шёл в WaterLab, где размещённой колонии `FlyR2_Gameplay` больше нет (она только в HOLD 1 корабля) | Клиент находит стресс-колонию по числу инстансов; `-FlyMap` по умолчанию `OceanFreighterR7`. Оба режима PASS: `20260913-204444-FlySwarm` (200), `20260913-205158-FlySwarm` (32, корабль) |
| 7 | Низкая | Вкладка «Подсказки» не говорила про новое правило толчка по товарищу | Добавлена строка |
| 8 | Косметика (исправлено) | C4305 double→float в `CalmyPreferences.cpp`, `SettingsTests.cpp`, `CalmySettingsProbe.cpp` и ~33 в `CalmySettingsWidget.cpp` | Литералы `f` во всех четырёх файлах; строки интерфейса не менялись |
| 9 | Инфо (разобрано) | 6 stash (`hero-g1…`, `stabilize-park-…`) | Непримененные строки — старые варианты уже реализованного: координаты строк меню 198/250, прежний манифест смены, `MenuUp/MenuDown`, `StormIntensity`, страница CREW SESSION. Всё это в рабочем дереве в новом виде; stash можно удалить после подтверждения владельца |
| 10 | Инфо | `.gitattributes` задаёт только LFS, без `text eol` — git предупреждает LF→CRLF на 28 файлах | Не менялось |
| 11 | Средняя (исправлено) | Три материала дверей (`MAT_Interior_EquipmentBlue/Machinery/Safety`) — инстансы движковых `MI_Default_Opaque_DS` → `M_Default` без флага Nanite; в игре UE подменял их материалом по умолчанию | `Scripts/fix_door_material_nanite_usage.py`: цепочка скопирована в проект (`MI_Gltf_Default_Opaque_DS_Nanite` → `M_Gltf_Default_Nanite` с `bUsedWithNanite`), переопределения opaque/two-sided сохранены, бэкап `SourceAssets/Backups/Before-DoorNaniteUsage-20260913`. Первая версия скрипта ошибочно сохранила флаг в установке движка (`Engine/Plugins/Interchange/Assets/Content/gltf/M_Default.uasset`); флаг возвращён в `False`, но файл пересохранён (91 510 → 91 781 байт). Предупреждений Nanite 0, Door smoke и Door Visual PASS |
| 12 | Инфо | 61 дверь и ещё 12 классов тикают каждый кадр (двери сразу выходят вне движения) | Не менялось; кандидат на оптимизацию |
| 13 | Инфо | Процедурный звук создаёт новый `USoundWaveProcedural` на каждый звук (короткие буферы, GC) | Не менялось |
| 14 | Инфо | Папка `Сделано Cursor/` и правило `.cursor/rules/sdelano-cursor-log.mdc` не отслеживаются | Оставлены как есть |

## Clean areas

- Editor-only API (`SetActorLabel`, `GEditor` и т. п.) вне `WITH_EDITOR` — 0; игровой таргет собирается.
- Все 19 серверных RPC-обработчиков проходят через command gate / `Try*` / `Receive*`; указатели от клиента проверяются `IsValid` в получателе (`TryGrab`, `TryHandoff`, `GetGrabFeedback`). Контракты RPC описаны (групповые комментарии для grab/drop/throw).
- OnRep-обработчики не пишут реплицируемые свойства.
- Все карты из кода и скриптов существуют и есть в `MapsToCook`; `ServerTravel` без seamless пересоздаёт net driver, поэтому новый подсчёт мест в `PreLogin` не видит старых соединений.
- Жёсткие пути ассетов (`SM_Cargo_*`, `CrewR8/SM_*`) существуют.
- Логи последних 14 smoke-прогонов: 0 ensure/assert/Accessed None/Fatal.
- `__pycache__` не отслеживается; документация `DOCS_PASS`; TODO/FIXME в Source — 0.

## Verified

Полный прогон до follow-up правок: automation 56/56; 19/21 smoke PASS (Settings, Melee, Session, Tutorial, Shift, Leak, Ship, Door, Carry, BridgeMenu, Cargo, Interior, Water, WaterInteraction, Character, Helper, Pause, Interaction, FlyMind); FlySwarm (без `-FlyPlaced`) и FlyTravel — FAIL, разобраны выше; HUD review PASS; игровой таргет PASS.

## Follow-up verification

| Check | Result |
|---|---|
| Editor build / game target | PASS / PASS |
| Automation через `Scripts/run_automation.ps1` с `-UserDir` | **56/56** `Saved/Logs/Automation-20260913-203705.log`; `Saved/Config/WindowsEditor/GameUserSettings.ini` не изменился |
| FlySwarm стресс 200 мух | **PASS** `Saved/Verification/20260913-204444-FlySwarm` |
| FlySwarm `-FlyPlaced` (корабль) | **PASS** `20260913-205158-FlySwarm` |
| Tutorial | **PASS** `20260913-204639-Tutorial` |
| Settings | **PASS** `20260913-204802-Settings` |
| FlyTravel ×6 | **FAIL** (см. находку 5): `20260913-203728`, `…203924`, `…204142`, `…205318`, `…205539`, `…205841` |
| Rider carry fix: automation | **57/57** `Saved/Logs/Automation-20260913-230822.log` |
| FlyTravel ×3 после исправления | **PASS** `20260913-230857`, `…231115`, `…231334` |
| Ship, Cargo, Helper, Carry, Melee, FlyMind, Shift, Leak, Tutorial, BridgeMenu, FlySwarm `-FlyPlaced`, Door | **PASS** |
| C4305 в сборке редактора | 0 |
| Материалы дверей: HUD review / Door Visual / Door smoke | **PASS**, предупреждений Nanite 0 `HUD-Review-20260913-233809-nanite-v3`, `Doors-R1/door-*.png` |
| Final DLL SHA256 | `756a54fdc09c9f1bb1783c4d8875943818e9025298f6a0b474d932f02ac5f152` |
| OceanFreighterR7.umap SHA256 | `ee3b80e11b9f519440f960e598a4ca78878477de690ce8453b4c0ffddb5dda64` |
| Master blend SHA256 | `8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93` (не изменён) |
