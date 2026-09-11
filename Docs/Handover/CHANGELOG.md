# Журнал продолжения проекта

## 2026-09-11 — Crew R8 import (PARTIAL)

[Отчёт](../Changes/CREW_R8_IMPORT_2026-09-11.md): 20 мешей в Content/CrewR8; SemanticSearch crash на quit. Runtime R8: Helper/Water/Ship PASS, Character recovery FAIL (bind R6B). Revert на R6B; Character PASS; FOV probe исправлен под preferences. Grips/water/package отложены.

## 2026-09-11 — Automation smoke S1

[Отчёт](../Changes/AUTOMATION_SMOKE_S1_2026-09-11.md): ENV Check вручную (pwsh7 нет); AUTOMATION_PASS 42/42 на DLL S1 `dd75bb31…`. Package/network smoke не запускались.

## 2026-09-11 — Phased ops: remote SKIP + ops hardening

[Отчёт](../Changes/PHASED_OPS_REMOTE_SKIP_OPS_2026-09-11.md): remote пропущен (нет URL). JPG/JPEG в LFS (30 файлов renormalize). Unity Archive `safe.directory` добавлен. Supermemory MCP auth OK.

## 2026-09-11 — Off-device backup B1

[Отчёт](../Changes/BACKUP_OFFDEVICE_B1_2026-09-11.md): копия на `D:\Backups\CalmyPiratesCo-2026-09-11` (проект без Intermediate/DDC/.vs + Releases). ZIP SHA256 и git HEAD совпали; Evidence 3793. Robocopy FAILED=0. Remote не создавался.

## 2026-09-11 — Git LFS baseline B1

[Отчёт](../Changes/GIT_BASELINE_B1_2026-09-11.md): по запросу пользователя создан локальный первый коммит `7bb0048b431705a61b74406882677373d01a55d7` (2880 tracked, 1826 LFS). В `.gitignore` добавлены `Evidence/` и `.codex/`. Remote/push нет. Gameplay и ZIP не менялись. Следующее — off-device backup Evidence/Releases; remote только по явному разрешению.

## 2026-09-11 — Repo check / doc fix

[Отчёт](../Changes/REPO_CHECK_FIX_2026-09-11.md): после сортировки F1 пользователь попросил проверить репозиторий и сохранить результат. ZIP P1 и DLL S1 совпали с ожидаемыми хешами; docs links PASS; Git UE пустой (0 commits/remote). Исправлен устаревший README; удалён пустой служебный `References/desktop-fly/.git` (реальный desktop-fly git не трогали). Gameplay/package не менялись. Следующее — только по явному запросу: Git+LFS baseline или off-device backup; перед коммитом учесть, что Evidence не в `.gitignore`.

## 2026-09-11 — File organization F1

[Отчёт](../Changes/FILE_ORGANIZATION_F1_2026-09-11.md): по просьбе пользователя отсортирована вся E:/Game. Созданы Releases/Archive/References; перенесены ZIP/SHA256, старые Unity/Studio и инструкции, старые пути сохранены скрытыми ссылками. Windows отказала в переносе desktop-fly: начатый частичный перенос восстановлен, Git fsck прошёл, доступ обеспечен junction из References. Текущий UE-проект, его зависимости, frozen evidence и ярлык не перемещались. SHA256 ZIP прежний, игровые версии P1/S1/R6 не менялись. Добавлена карта папок/README, журнал действий, проверки путей/launcher/инвентаря. Ничего не удалялось; при переносе на другой диск нужно проверить абсолютные links.

## 2026-09-11 — Windows share P1

[Отчёт](../Changes/WINDOWS_SHARE_P1_2026-09-11.md): пользователь попросил отдельный ZIP игры для друга. Собрана Windows x64 Development-версия S1/R6; исправлены пропущенные при cook динамические модели CrewR6B/CargoR3 и материал BridgeMenu, добавлен корабль в MapsToCook. В архиве EXE, ресурсы, Visual C++ redistributable, русская инструкция, Play-Ship.cmd и notices. Проверки выполняются из распакованного ZIP с отдельными профилями; точные результаты/хеши/ранняя ошибка упаковки в отчёте. C++ gameplay/карты/модели прежние; Windows-Current не заменён, ярлык editor-game сохранён. Следующее — отправка пользователем и запуск на компьютере друга; интернет/Steam и human acceptance не заявлены.

## 2026-09-11 — Settings / performance S1

[Отчёт](../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md): пользователь сообщил о лагах на корабле у мух и попросил полные настройки. Диагностика выявила CPU-пересчёт океана; кеширование фаз и ранний выход для пустых возмущений дали 20.68 → 91.85 FPS в одной и той же HOLD 1 сцене, без отключения SNN. Добавлены пять вкладок Slate (экран/графика/звук/клавиши/камера/HUD), локальное применение/сохранение, remapping, миграция старой камеры и 15-секундное подтверждение видеорежима. Launcher сохраняет выбранные экран/FPS. 42/42 automation, Settings GPU/network и WaterInteraction/Melee/BridgeMenu регрессии; точные версии и повторные проверки в отчёте. Ранние build/NullRHI/Slate-capture ошибки сохранены. Source/DLL/scripts новые; карты/меши/Blender-кандидаты/package прежние. Следующее — обычная игра и оценка FPS/комфорта пользователем, полный performance sweep отдельно.

## 2026-09-11 — Fly travel R6

[Отчёт](../Changes/FLY_TRAVEL_R6_2026-09-11.md): пользователь попросил один самостоятельный моб из мух, свободно живущий на корабле. Общая когнитивная модель теперь двигает весь рой из HOLD 1; добавлены ограниченный инкрементальный поиск, память посещений, collision центра/особей, след для возврата и сетевой actor movement. Сохранены SNN/память R4 и интерфейс/удары R5. Build2 / 41 из 41 automation / FlyTravel сеть PASS, остальные результаты в отчёте. Первый host завершился раньше позднего клиента; исправлены тестовый срок жизни и стягивание мух в одну точку. Source/DLL/scripts новые, maps/модели прежние; нового пакета нет. Дальше — обычная игра, оценка темпа/заметности роя; не заявлены все помещения и новые предметные занятия.

## 2026-09-10 — Interaction R5

[Отчёт](../Changes/INTERACTION_R5_2026-09-10.md): пользователь попросил действия вместо отдельных клавиш, удар, ответ Миши после 3 попаданий, разлёт мух и колесо E. Добавлены контекст ЛКМ, серверный aimed hit/HP/cooldown/sequence gate, per-attacker provocation и Defending, bounded swat/память угрозы, Canvas колесо с input lease и без click-through. J/K/L/O удалены. Сохранены ошибки первого RenderCore link и параллельного GPU hover; быстрый разворот NPC и ударная рука улучшены через плавный поворот/IK. Финальные build5/40 из 40 automation/Melee и FlyMind сети/Restore/отдельный GPU мыши PASS; carry/helper/bridge menu PASS на build4. Новые Source/DLL, maps/assets прежние; packaged build старый. Frozen `Evidence/Gameplay/Interaction-R5-20260910`. Следующее — обычный запуск/человеческая оценка, баланс и дальнейшая полировка; физический контакт кулака/новая модель/звук не заявлены.

## 2026-09-10 — Fly habitat R3 / cognitive model R4

[Отчёт](../Changes/FLY_MIND_R4_2026-09-10.md): 32 мухи перенесены в HOLD 1, WaterLab дубликат удалён, backups сохранены. Пользователь уточнил, что «сознание» означает реальную вычисляемую модель с памятью, обучением, занятиями и общением. Добавлен общий когнитивный слой, person-specific learning, ограниченная память/эпизоды, автономный выбор занятий, J/K/L/O диалог и серверный SaveGame. Субъективное сознание не установлено, свободного языка/озвучки нет. Source/DLL/maps новые, арт R2 прежний, packaged build старый. Build3 / 38 из 38 automation / Host/Client/LateJoin / новый процесс Restore / GPU обычного HUD PASS. Сохранены неудачи CoreOnline linking и конфликт `;` с DebugCamera; исправлены зависимость/клавиша O и неверный visual disk marker. Frozen `Evidence/Gameplay/Fly-Mind-R4-20260910`. Следующее — оценка в обычной игре; богатое планирование/свободное общение остаются отдельными этапами.

## 2026-09-10 — Fly in game R2

[Отчёт](../Changes/FLY_IN_GAME_R2_2026-09-10.md): пользователь попросил добавить муху в игру. Создан оригинальный mesh 1840 tris, свои материалы и wing WPO; по 32 особи сохранены перед стартом WaterLab и над приборами рубки OceanFreighterR7. Сохранены backups обеих исходных карт. Финальные build/import exit 0/35 из 35 automation/GPU/Host–Client–LateJoin на обеих картах PASS. Исправлены экспорт COLOR_0, Sine input и тестовый resume из solo menu; прежние неудачи в evidence. Новые Source/DLL/maps/assets, старый package не обновлён. Collision/урон/звук не добавлены. Следующее: обычный пользовательский запуск и при необходимости игровой эффект роя.

## 2026-09-10 — Fly Swarm R1

[Отчёт, экспорт и настройка](../Changes/FLY_SWARM_2026-09-10.md). Новый `AFlySwarmMob`, исходный граф desktop-fly закреплён по commit/SHA256, CSR shared singleton, независимые компактные LIF-состояния с точной 4-мс inhibition history, 20–30-Гц timer и 1-мс substeps, ParallelFor, HISM и server-authoritative poses. Сборка/35 из 35 automation/Host–Client–LateJoin PASS; неудачный первый сетевой run и исходная ошибка компиляции сохранены. Карты/ассеты прежние, placeholder fly mesh; collision/урон/арт/GPU acceptance/new package не выполнены. Новый source/DLL поверх Tutorial T1. Notices MIT и CC BY-NC 4.0 сохранены. Следующий шаг — назначить mesh и выбрать место роя на уровне.

Живой журнал. Новые записи добавлять сверху, не удалять прежние неудачи. Детальные исторические отчёты остаются в Docs/Changes. [Шаблон](Templates/CHANGE_ENTRY.md).

## 2026-09-10 — Tutorial → Ship T1

[Отчёт](../Changes/TUTORIAL_SHIP_T1_2026-09-10.md). WaterLab стала обычным началом игры: три общих серверных задания без таймера, физическая доставка одного Standard-ящика, отсчёт и переход всей команды на OceanFreighterR7. Новые HUD инструкции/направление цели, default map и launchers; direct ship entry сохранён. Проверены solo GPU, Host/Client/LateJoin, 32/32 automation. Первый сетевой travel потерял listen; исправлен и повторно проверен. Модели/карты/вода не перестраивались, пакет не обновлялся, постоянного сохранения урока нет.

## 2026-09-10 — Water Interaction W1

[Отчёт](../Changes/WATER_INTERACTION_W1_2026-09-10.md). Серверные удары/след у поверхности формируют общее локальное поле для mesh displacement, сил плавучести и скорости воды пловца. Реплицируется ограниченный снимок живых волн с исходным временем. Отдельный component/material брызг, исправлены отсутствующий emissive и смешивание прозрачных эффектов с SingleLayerWater; смягчены яркие полосы. Новый стенд четырёх типов груза, проверки вершин/сил и peer snapshots. Это response approximation, не полный fluid solver, шторм или подвижный R7. Пальцы/skin/active walking не сделаны. Карты/модели старые, новый только W1 material; приложения/внешние ассеты не скачивались. Финальные проверки и незавершённое — в отчёте; неудачные серии сохранены.

## 2026-09-10 — Physical Crew / Bridge R2, часть запроса из четырёх пунктов

[Отчёт](../Changes/PHYSICAL_CREW_BRIDGE_R2_2026-09-10.md). Серверный физический торс при падении, силы/моменты подъёма с floor/capsule clearance, камера и руки от тела, переход к плаванию; исправлен kinematic reset/rescue. Терминал рубки: видимость, paused rendering, hover/input и сетевой ViewTarget, настройки/resume/exit. Обычный запуск теперь OceanFreighterR7, WaterLab сохранён. 30/30 automation и отдельные сетевые/GPU проверки — в отчёте. Runtime meshes R6B, Blender R8 не импортирован; Content не изменён. **Пальцы, полноценный active locomotion, physical storm и динамический корпус не сделаны**. Старый пакет не обновлён. Mixamo login ранее выполнен пользователем; хранение выбрано локальное, облако не подключалось. Все неудачные прогоны сохранены.

## 2026-09-09 — Grip/Health R1, первый этап нового запроса

[Отчёт](../Changes/GRIP_HEALTH_R1_2026-09-09.md): physical cargo IK/исправление обратного drive, NPC на противоположной грани, reachable carry distance, null guard, replicated HP/landing damage/grounded recovery, motion default 100% без сброса preferences. Добавлены GPU grip views и регрессии; неудачные кандидаты сохранены. Source/DLL теперь не frozen R6, meshes R6B и карты неизменны. Осталось: dynamic grips/sockets/skin, active ragdoll, BridgeMenu/ship-first, физическое море и шторм — не объявлять их выполненными этим этапом.

## 2026-09-09 — Rigify и подключение редакторов

[Отчёт](../Changes/TOOLING_SETUP_2026-09-09.md). Включён уже поставляемый Rigify, preferences backup, disposable rig generation PASS (706 total bones, не игровой экспорт). Blender MCP compatibility/execute PASS; Unreal MCP initialize/tools-list PASS, только 127.0.0.1. start_dev получил explicit MCP/loopback и Hidden default, добавлены setup/test scripts. Новых загрузок нет, Git LFS уже настроен, Plugin Management найден в 0.1.0. Mixamo требует входа; backup destination не выбран. R6B/R8/ship assets не редактировались; gameplay tests/importer shutdown не перепроверялись. Следующее: независимая копия и выбранный skin/animation pipeline.

## 2026-09-09 — handover и организация

[Подробный аудит и выполненные проверки](../Changes/HANDOVER_AUDIT_2026-09-09.md).

Запрос: полный документ для другого устройства, порядок в репозитории и запись будущих изменений. Добавлены HANDOVER, каталог моделей R6B/R7/R8, инструкция переноса, каталог назначения папок/скриптов, Incoming/Characters, переносимый launcher и генератор/валидатор инвентаря. AGENTS теперь требует обновлять handover/journal. README и текущие статусы исправлены: **runtime R6B ≠ Blender candidate R8**. Исторические пути/Content/SourceAssets не переставлялись; совместимость сохранена. Снимок прежних документов — Docs/Archive/Before-Handover-20260909.zip.

Проверено: наличие исходников/20 GLB и 20 текстур R8; read-only Blender показал относительные внешние texture paths. DLL и обе карты совпадают с frozen R6. Git: 0 tracked, нет коммитов/remote. Полный игровой тест, импорт моделей, перенос на реальный второй ПК, commit/push **не выполнялись**. Итог в Docs/Handover/INVENTORY.json и read-only doc checks. Следующее: независимая копия, выбранный пользователем герой/skin pipeline, отдельно BridgeMenu.

## 2026-09-08 — Crew R8, кандидат вне игры

[Отчёт](../Changes/CREW_R8_2026-09-08.md): геометрия лица, position-based face texture, palette×AO bake, суперэллиптические сечения одежды, уменьшение стыков. 6012/6048 triangles. Blender-контракты заявлены в отчёте, UE automation/physics/GPU не проводились. AO в base color и жёсткая сегментация остаются ограничениями.

## 2026-09-08 — Crew R7, кандидат вне игры

[Отчёт](../Changes/CREW_R7_2026-09-08.md): пересборка по двум AI-бордам, 5462/5442 triangles, 20 деталей, четыре Blender-рендера. Не импортирован; не подменяет принятую тестами R6B.

## 2026-09-08 — R6B и парные концепты

[R6](../Changes/CREW_R6_2026-09-08.md): 4802/4882 triangles в Unreal, 28/28 automation, Character/Helper/Ship/Water/Pause, solo helper, GPU. Стыки/сумрак/пальцы частично; max Ship joint 7,812/8. Два import shutdown crash записаны как diagnostics. Отдельно созданы imagegen PNG Миши и Deckhand с prompts, не 3D.

## 2026-09-08 — BridgeMenu draft / атмосфера / двери

Меню на экране рубки начато и осталось незавершённым; записанного clean end-to-end PASS нет. Не закрывать обычным Pause PASS. [Nightwatch](../Changes/NIGHTWATCH_R1_2026-09-08.md), [Doors](../Changes/SHIP_DOORS_R1_2026-09-08.md), [Engine Dream](../Changes/ENGINE_DREAM_LOOP_R1_2026-09-08.md) — отдельные исторические итерации.

## До 2026-09-08

[Каталог всей истории](../INDEX.md), [старые handovers](../History/INDEX.md). Unity→UE setup, корабль R3–R7, лаборатории/течь/питание/смена, персонаж R2–R4, вода и NPC, G1-A, металл и ship import. Их тестовые результаты не переносить автоматически на следующий бинарник.
