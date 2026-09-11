# Проверки и границы доказательств

**11 сентября — [Off-device backup B1](../Changes/BACKUP_OFFDEVICE_B1_2026-09-11.md).** Robocopy FAILED=0; ZIP SHA256; git HEAD `47efb30…`; Evidence counts; key-path hashes. Манифест в `Saved/Verification/Backup-OffDevice-B1-20260911-182808` и `D:\Backups\...\MANIFEST`.

**11 сентября — [Git LFS baseline B1](../Changes/GIT_BASELINE_B1_2026-09-11.md).** `git rev-parse HEAD` = `7bb0048b431705a61b74406882677373d01a55d7`; 2880 tracked / 1826 LFS; status clean до docs-followup. Evidence намеренно вне Git. Это не проверка gameplay и не наличие remote.

**11 сентября — [repo check / doc fix](../Changes/REPO_CHECK_FIX_2026-09-11.md).** Повторно сверены ZIP SHA256, editor DLL S1, junction/symlink targets, desktop-fly `git status`/`fsck`, `verify_project_docs` (102/752/0). README и пустой leftover `.git` исправлены; новый gameplay/automation не запускался. Риски Git (нет baseline; Evidence не ignored) зафиксированы в отчёте.

**11 сентября — [файловая организация F1](../Changes/FILE_ORGANIZATION_F1_2026-09-11.md).** Проверяются сохранность NTFS identities/файловых SHA256, ссылки совместимости, Git fsck desktop-fly, launcher DryRun и документационные ссылки. Gameplay и артефакт ZIP не изменялись; новые GPU/automation/network не запускались, результаты P1/S1 остаются привязанными к тем же бинарникам. Журнал перемещений и исключение Windows directory rename сохранены в отчёте.

**11 сентября — [Windows share P1](../Changes/WINDOWS_SHARE_P1_2026-09-11.md).** Новый Windows EXE из распакованного ZIP проверяется отдельно от editor DLL S1: GPU обучение/переход/меню/настройки, Tutorial Host/Client/LateJoin, 7-Zip integrity. Первый пакет не принят из-за пропущенных LoadObject assets, несмотря на логические PASS; исправлены always-cook каталоги и обязательная проверка missing-content warnings. Финальные run IDs и границы в отчёте. Это локальный packaged smoke, не проверка компьютера друга или интернет-сессии.

**11 сентября — [Settings / performance S1](../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md).** 42/42 automation; Settings Host/Client/LateJoin и GPU пяти вкладок; WaterInteraction/Melee/BridgeMenu сетевые регрессии. Версии DLL и точные run IDs приведены в отчёте: не переносить результаты между сборками без указанной границы изменений. Изолированный GPU benchmark HOLD 1: 20.68 → 91.85 FPS при Epic 1280×800, без отключения SNN/воды. WaterInteraction: mesh error 0.0000, 944/929/318 проверок вершин. Первые неудачи видеотаймера/захвата Slate сохранены. Это не весь корабельный маршрут, не 200 мух и не human acceptance.

**11 сентября — [Fly travel R6](../Changes/FLY_TRAVEL_R6_2026-09-11.md).** Build2 и 41/41 automation PASS, включая инкрементальный поиск вокруг стены и ограничение работы за callback. FlyTravel Host/Client/LateJoin PASS: один saved actor, самостоятельный выход из HOLD 1, движение/отдых/когезия, проверка серверных сегментов по реальному collision, клиентские poses без мозгов. На сервере 106.85 м за 125 с, spread до 55 см, наибольший измеренный шаг 10.59 мс при параллельной automation; это наблюдение 32 мух, не benchmark 200. Первый прогон завершил host слишком рано для LateJoin; исправление/повтор и остальные регрессии описаны в отчёте. Полный обход всех кают и человеческая оценка не заявляются.

**Текущая DLL — [Interaction R5](../Changes/INTERACTION_R5_2026-09-10.md).** Build5/40 из 40 automation, Melee Host–Client–LateJoin, FlyMind Host–Client–LateJoin + отдельный Restore, изолированный GPU с мышью — PASS. Проверены порог 3 попаданий от одного игрока, ответы/HP, невиновный, replay, swat, E/hover/клик/Esc без удара/паузы. Carry/Helper/BridgeMenu регрессии PASS на build4 (после них менялись только punch IK и подпись меню). Первый link потребовал RenderCore; первый быстрый разворот растягивал руки, исправлен; один параллельный GPU hover не прошёл и не считается PASS. Все версии/неудачи в frozen `Evidence/Gameplay/Interaction-R5-20260910`. Это не human fun gate/релизная боевая анимация/packaged acceptance.

**Текущая DLL — [Fly mind R4](../Changes/FLY_MIND_R4_2026-09-10.md).** Build3 и 38/38 automation PASS: причинное влияние выученного опыта на SNN, автономные занятия и SaveGame roundtrip. Host/Client/LateJoin + новый процесс Restore PASS; invalid/far и повторные обращения отвергаются, у клиентов нет мозгов/личной памяти. GPU обычного HUD с ответом о реальной угрозе просмотрен. Ранний визуальный `;` включал DebugCamera: исправлено на O; ранний visual `disk_saved=1` ошибочен и исправлен (диск проверяется Host/Restore). Saved HOLD 1 включён, WaterLab 0 роёв. Успешные тесты не доказывают сознание; это не human fun gate/общая приёмка/новый package.

**Последняя DLL/assets — [Fly in game R2](../Changes/FLY_IN_GAME_R2_2026-09-10.md).** Build/35 из 35 automation, GPU обзоры WaterLab и рубки, макро WaterLab (макро рубки заслонён консолью), Host/Client/LateJoin обеих карт PASS. Проверялись сохранённые рои, не новые test actors. Первая ошибка wing shader/fallback обнаружена по GPU и исправлена; первый ship visual probe зависел от solo pause, теперь явно resume в тесте. Все ранние логи сохранены. Это не full gameplay soak или packaged acceptance.

**Последняя проверенная DLL — [Fly Swarm R1](../Changes/FLY_SWARM_2026-09-10.md), поверх T1.** Development Editor build и 35/35 automation PASS; в том числе 1000 мс step-by-step сравнения LIF с исходной float-очередью торможения. Host/Client/LateJoin на 200 мух PASS; мозги только на сервере, инстансы движутся у всех. NullRHI, placeholder mesh: GPU/финальный арт не оценивались. Нижние T1/W1 числа относятся к их собственным ревизиям.

**Последняя DLL — [Tutorial → Ship T1](../Changes/TUTORIAL_SHIP_T1_2026-09-10.md).** 32/32 automation; реальные movement/grab/drop/settled-delivery → ServerTravel → bridge menu/resume в solo GPU и Host/Client/LateJoin. Первое сетевое падение перехода не скрыто: absolute travel терял listen, исправлено сохранением текущей сессии через relative travel. W1/R2 результаты ниже относятся к прежним DLL, не общая переаттестация всех механик на T1.

**Текущая итерация — [Water Interaction W1, 10 сентября](../Changes/WATER_INTERACTION_W1_2026-09-10.md).** Новый стенд различает события, физическую силу и фактические вершины поверхности; отдельно сравниваются параметры событий Host/Client/LateJoin. Итоговые run IDs и SHA256 в отчёте W1. Результаты предыдущих DLL ниже — исторические; живые события/пена не равны полному шторму, dedicated test или human acceptance.

**10 сентября, текущая runtime — [Physical Crew / Bridge R2](../Changes/PHYSICAL_CREW_BRIDGE_R2_2026-09-10.md).** DLL dec6d435…; 30/30 automation, отдельные сетевые и GPU прогоны перечислены в новом отчёте. Свободный серверный торс/моторный подъём и BridgeMenu entry проверяются отдельно от skin/пальцев, новой fluid simulation и human acceptance. Все сведения Grip/Health R1 и R6B ниже — история прежних итераций, не описание текущей DLL.

**Последняя runtime-итерация — [Grip/Health R1, 9 сентября](../Changes/GRIP_HEALTH_R1_2026-09-09.md).** Финальная DLL d7735d5a…: 29/29 automation; Character/Helper/Ship/Water Host/Client/LateJoin, solo helper и 10 GPU кадров PASS. Wrist static: 1.013/0.622 см, Ship max joint 7.919 из 8 — тонкий запас. [Frozen manifest](../../Evidence/Gameplay/GripHealth-R1-20260909/manifest.json). Старое обозначение «актуальная R6B» ниже — исторический baseline; meshes остаются R6B, Source/DLL изменены. Нет нового active ragdoll, полной fluid simulation или BridgeMenu PASS.

**Аудит документации 2026-09-09:** текущие Source/DLL/карты сверяются с frozen R6 через [инвентарь](../Handover/INVENTORY.json). Blender R7/R8 не подключены, UE automation/physics/GPU для них отсутствуют. Этот аудит проверяет файлы, пути, ссылки и launcher DryRun; не новый игровой PASS и не тест на втором ПК.

**Актуальная ревизия — [Crew R6B, 8 сентября](../Changes/CREW_R6_2026-09-08.md).** 28/28 automation, Character/Helper/Ship, solo helper, девять кадров персонажей и семь кадров/37-точечный GPU маршрут корабля. Max joint на Ship 7,812 из 8 см — маленький запас, не доказательство длительной устойчивости. Два shutdown-crash импортера сохранены как diagnostics; последующие чистые runtime/automation загрузки прошли. BridgeMenu draft не принят. Полные журналы, дополнительные прогоны и границы — в отчёте/frozen acceptance. Старые LP4/G1-A/R5 результаты не переносятся на R6B; кадры не равны human acceptance, 60 fps cap не измерение производительности.

Актуализация 2026-09-06. Последний проход — [статическая интеграция корабля R7](../Changes/SHIP_R7_INTEGRATION_2026-09-06.md). Прежние G1-A/R3/R1/R2/LP4 сохранены отдельно и не доказывают новый бинарник.

## R7 в игре — ограниченный непрерывный маршрут

Последний gameplay-проход — [Doors R1, 8 сентября](../Changes/SHIP_DOORS_R1_2026-09-08.md): 61 закрывание/открывание в реальной карте, owning-client RPC + duplicate/stale, LateJoin и повтор Ship Host/Client. Два крупных кадра одной двери; это не прохождение всех 61 порогов. Финальная automation/Interaction/GPU и хеши — в новом frozen evidence, прежние PASS ниже исторические.

[Engine Dream Loop R1](../Changes/ENGINE_DREAM_LOOP_R1_2026-09-08.md), 8 сентября: два GPU обхода по 37 точек, два независимых набора Ship network, визуальный критик по семи кадрам каждой ревизии. Это ограниченная световая итерация: не новый automation run, не весь интерьер и не подтверждение 60 FPS.

После интеграции выполнен [материальный повтор Metal R1](../Changes/SHIP_METAL_R1_2026-09-06.md): семь GPU-кадров и 37-точечный solo обход, Ship Host/Client PASS, LateJoin scene/spawn PASS. Геометрия и DLL прежние; 26 automation ниже относятся к integration, не к новому запуску этой итерации.

26/26 automation. На финальной карте с десятью meshes: Host и Client по 37 точек weather → нижнее машинное → обратно, 91,049 / 102,904 м, максимальное расхождение физических соединений 5,634 / 5,633 см при лимите 8 см. Без телепортов игрока, отключения Actor Tick, rescue или swimming внутри. LateJoin — проверка состава карты и grounded spawn, не третий обход. GPU — отдельный solo маршрут и семь просмотренных кадров, включая три постановочных CameraActor. WaterLab повторно PASS в трёх сетевых ролях. Точные границы/логи/DLL указаны в отчёте выше; это не повтор всех старых 260/336/56 маршрутов и не приёмка full ship.

```powershell
& ./Scripts/run_network_smoke.ps1 -Suite Ship -Port 17882
& ./Scripts/run_ship_r7_review.ps1
```

## Последняя ревизия моделей: Crew Low-Poly R4

[Отчёт](../Changes/CREW_LOWPOLY_R4_2026-09-06.md), [Evidence](../../Evidence/Gameplay/CrewLowPoly-R4-20260906/acceptance.json). На новых моделях — 25/25 automation, девять просмотренных GPU-кадров. Сетевая проверка новой геометрии фиксируется в этом Evidence отдельно. Старый G1-A soak ниже использовал R3 meshes: его нельзя считать 15-минутным PASS новых NDOP26 тел. Карты/вода/грузы/R7 в LP4 не менялись.

## Проверки G1-A (предыдущие модели персонажей)

**PASS:** 24/24 automation; девять прежних сетевых наборов и новый Interaction × три роли; Solo Helper; отдельные процессы PrefsWrite/PrefsRead; семь просмотренных GPU-кадров и GPU-пауза. Interaction: Host 909,4 с / Client 900,0 с, 89 падений/восстановлений у каждого, максимум 3,912/3,260 см при лимите 8 см, без rescue. Это плоский тестовый маршрут, не R7. [Evidence и точные версии](../../Evidence/Gameplay/G1-A-20260906/acceptance.json): A1 регрессии, B soak/Carry, C финальный HUD/preferences и короткий повтор Interaction/Carry.

G1-A добавляет два теста (всего 24 automation), сетевой `Interaction` с коротким прицельным захватом и 900-секундным техническим прогоном, отдельную проверку сохранения M/V/чувствительности двумя процессами с временным UserDir, GPU-кадры отказа/готовности/переноса и повтор грузовых регрессий. Окончательные результаты и граница бинарников фиксируются в [отчёте G1-A](../Changes/G1_A_2026-09-06.md); наличие runner не означает PASS. 15-минутный стенд не заменяет human gate или обход R7.

```powershell
& ./Scripts/run_network_smoke.ps1 -Suite Interaction -Port 17930 -SoakSeconds 900
& ./Scripts/run_camera_preferences.ps1
& ./Scripts/run_crew_cargo_r3_review.ps1 -InteractionHUD
```

## Предыдущие проверки R3

[Подробный отчёт](../Changes/CREW_CARGO_WATER_R3_2026-09-06.md), [приёмочная запись](../../Evidence/Gameplay/CrewCargoWater-R3-20260906/acceptance.json), [manifest](../../Evidence/Gameplay/CrewCargoWater-R3-20260906/evidence-manifest.json).

- Editor Development build; **22/22 automation**, включая новую двухзвенную IK, волновую огибающую и постоянного помощника.
- Helper, Character, Water, Cargo, Carry, Shift, Pause: **семь наборов × Host / Client / LateJoin PASS**, loopback. Для Helper/Character внешняя пакетная обёртка ошибочно проверила несвязанный LASTEXITCODE уже после NETWORK_SMOKE_PASS; логи всех ролей и штатное завершение проверены runner. Остальные пять наборов выполнены без этой обёртки.
- Solo Helper PASS: один человек, запрос/повтор, перенос тяжёлого груза, доставка, оставить ждать и повторно попросить; минимальный TeamPace 55%.
- GPU: шесть сохранённых и просмотренных кадров. В тестовой позе груза метрика недостижимого контакта рук 0,000 см, при водном кадре девять активных событий. Это не измерение проникновения пальцев или финальной анимации.
- Character max joint gap: 0,58 / 0,47 / 0,00 см; порог 8 см не ослаблен.

**Границы:** GPU fixture переставляет актёров/камеру для кадров, это не физический обход. Не повторены Leak/Interior, R7 NPC/пешая матрица, пакет, плохая сеть и человеческая приёмка. R7 геометрия не менялась, но высота переносимого предмета изменилась на 18 см: старые корабельные PASS не принимать за runtime R3. Полная вода/шторм, лазание и два физических хвата не реализованы.

Повторить визуальный стенд: `& ./Scripts/run_crew_cargo_r3_review.ps1`. Он создаёт новый timestamp-каталог и не перезаписывает Evidence. Наборы сети и solo запускаются командами ниже; PowerShell runner сообщает ошибку исключением, не проверяйте устаревший `$LASTEXITCODE` после вызова `.ps1` через `&`.

Основной [архив R2](../../Evidence/Gameplay/CrewHelper-R2-20260906/acceptance.json), [manifest](../../Evidence/Gameplay/CrewHelper-R2-20260906/evidence-manifest.json): **189 файлов, 0 несовпадений** при документационном аудите. Тогда Source совпали со snapshot; после добавления контроллера/меню R1 исходники уже отличаются. Это локальный архив, не облачная резервная копия.

## Новые проверки паузы R1

[Отчёт R1](../Changes/PAUSE_R1_2026-09-06.md): 21/21 automation, Pause Host/Client/LateJoin, solo GPU-проверка запуска на паузе, Esc/Enter и hit-box кнопки мышью. Character также повторён в трёх ролях: максимальный зазор 0,07 / 0,01 / 0,00 см. Это не повтор всей водной/грузовой/корабельной матрицы и не новая packaged-сборка.

```powershell
& ./Scripts/run_pause_solo.ps1 -Visual
& ./Scripts/run_network_smoke.ps1 -Suite Pause -Port 17917
```

## Последняя полная игровая регрессия R2 (до паузы)

| Проверка | Итог | Долговечное доказательство |
|---|---|---|
| Automation | 20/20 | [automation.log](../../Evidence/Gameplay/CrewHelper-R2-20260906/automation.log) |
| Solo Helper | PASS, один PlayerController; heavy delivery, replay, dismiss/rehire; минимальный темп 65% | [solo.log](../../Evidence/Gameplay/CrewHelper-R2-20260906/solo.log) |
| Helper | Host / Client / LateJoin PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Helper/) |
| Character | Три роли PASS, max gap 0,21 / 0,02 / 0,00 см, предел 8 см | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Character/) |
| Cargo | Три роли PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Cargo/) |
| Carry | Три роли PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Carry/) |
| Water | Три роли PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Water/) |
| Shift | Три роли PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Shift/) |
| Leak | Три роли PASS | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Leak/) |
| Interior | Три роли PASS, старая FreighterDeck, не R7 | [логи](../../Evidence/Gameplay/CrewHelper-R2-20260906/network/Interior/) |
| GPU-персонаж | Пять просмотренных кадров, max gap 0,17 см | [кадры](../../Evidence/Gameplay/CrewHelper-R2-20260906/views/), [лог](../../Evidence/Gameplay/CrewHelper-R2-20260906/visual.log) |
| NPC на корабле | 56/56, включая четыре контроля | [отчёт](../../Evidence/Gameplay/CrewHelper-R2-20260906/npc-ship-report.json) |
| Пеший корабельный | 260/260 обязательных, 293/336 широких; 43 прежних FAIL, 0 изменений исходов | [отчёт](../../Evidence/Gameplay/CrewHelper-R2-20260906/walking-report.json) |

Все сетевые результаты — локальный loopback, Editor/game, не интернет и не новый packaged build. Старые успешные packaged smoke относятся к старому содержимому. GPU и корабельные отчёты сняты до изолированных поздних изменений restart/тестового сценария; тело, NPC-route и вода соответствуют проверяемой реализации. Архив фиксирует эту границу, а не обещает один идентичный бинарник во всех исторических запусках.

R7 геометрия 98/98 и 17 кадров находятся в отдельном [архиве R7](../../Evidence/ShipTraversal/R7-Cargo-20260906/acceptance.json). Они относятся к master SHA256 `8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93`. R6 acceptance нельзя запускать как приёмку R7.

## Что не доказано

Весь solo-контракт, четыре человека/несколько NPC, плохая сеть, длительный стресс на корабле и движущемся основании, хваты двух игроков, все помещения/рабочие позиции судна, свободная bodycam на R7, движущийся корабль, финальная анимация/звук/UI, человеческий fun/comfort gate. Узкий 15-минутный плоский стенд G1-A уже PASS. Предыдущие провалы сохранены в [diagnostic-failures](../../Evidence/Gameplay/CrewHelper-R2-20260906/diagnostic-failures/), не удалены ради зелёной таблицы.

## Повторить проверку

После [сборки](OPERATIONS.md), из корня UE-проекта:

```powershell
& ./Scripts/run_automation.ps1
& ./Scripts/run_helper_solo.ps1
foreach ($suite in @('Helper','Character','Cargo','Carry','Water','Shift','Leak','Interior')) {
    & ./Scripts/run_network_smoke.ps1 -Suite $suite -Port 17820
}
$stamp = Get-Date -Format yyyyMMdd-HHmmss
& ./Scripts/run_ocean_physics.ps1 -NpcAudit -ReportName "R7-NPC-$stamp"
& ./Scripts/run_ocean_physics.ps1 -TraversalAudit -ReportName "R7-Walk-$stamp"
```

Не запускать два physics runner одновременно: общий экспорт защищён lock. Не повторять прежние имена отчётов. Строгий широкий runner **ожидаемо вернёт FAIL при 293/336**: сравнить имена/контакты, обязательные 260 и исходный hash, а не ослаблять тест. Четыре контроля входят в итоговое число.

Узел NPC-стенда отключает legacy rescue и начинает с совместного размещения капсул; после разделения они сталкиваются, NPC следует сам, лидер ждёт отстающего. Набор проверяет 13 маршрутов с грузом, не все 336 с NPC. Генератор/тестовые fixtures не выдаются за пользовательский ручной обход.

Для нового релиза сохранить source/config/model hashes, точный состав тестов, выводы и картинки в **новый** каталог Evidence. Не редактировать замороженные старые отчёты, их снапшоты документации или manifest.
