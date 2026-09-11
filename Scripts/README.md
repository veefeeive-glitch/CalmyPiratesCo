# Скрипты — назначение и риск

[run_packaged_share_review.ps1](run_packaged_share_review.ps1) — GPU-проверка самостоятельного EXE из распакованного ZIP: обучение → корабль и пять вкладок настроек. Требует -PackagedExe; тестовые профили изолированы, отсутствующий игровой контент вызывает FAIL. run_network_smoke.ps1 с -PackagedExe также использует отдельные профили. Этот runner не публикует/не продвигает Windows-Current.

S1: [run_performance_review.ps1](run_performance_review.ps1) — изолированный GPU benchmark HOLD 1, backup/restore пользовательского ini, требует отсутствия других Unreal-процессов. [run_settings_review.ps1](run_settings_review.ps1) — GPU семи кадров/пяти вкладок, применение/сохранение/видеоподтверждение с отдельным ini. [run_network_smoke.ps1](run_network_smoke.ps1) поддерживает -Suite Settings (Host/Client/LateJoin). Обычные start_game.ps1/Project.ps1 Play больше не переопределяют сохранённые экран/FPS.

Запускать из корня проекта. Пути старых скриптов сохранены: многие вычисляют root как `parents[1]`; перенос самих скриптов в подпапки сломает их. Этот индекс сортирует их по работе, не выполняет операции.

## Повседневное продолжение

- [Project.ps1](Project.ps1) — переносимый Check/Build/Automation/Play/Models, параметры EngineRoot/BlenderExe, DryRun.
- [Maintenance/Update-HandoverInventory.ps1](Maintenance/Update-HandoverInventory.ps1) — сгенерировать карту файлов/хешей или `-Verify` старую копию; не маркирует gameplay PASS.
- [verify_project_docs.ps1](verify_project_docs.ps1) — ссылки документов и legacy register.
- [start_dev.ps1](start_dev.ps1) — запуск Blender/UE MCP только loopback; Hidden default, `-ShowWindows` для новых видимых окон. Не открывает master в Blender и не сохраняет проект.
- [test_unreal_mcp.ps1](test_unreal_mcp.ps1) — read-only MCP handshake/discovery, не gameplay PASS.
- [setup_blender_rigify.py](setup_blender_rigify.py) — Blender background: backup preferences, enable bundled Rigify, disposable rig generation; не изменяет модели.
- [start_game.ps1](start_game.ps1), [start_dev.ps1](start_dev.ps1), [Open-OceanFreighter.ps1](Open-OceanFreighter.ps1) — исторические local launchers, местами фиксированные E:/ пути.

## Проверки игры, не арт-авторинг

- [Play-Calmy.cmd](Play-Calmy.cmd) — двойной клик: текущая Editor-game версия, туториал → корабль; через PowerShell `start_game.ps1 -MapName OceanFreighterR7` — прямо на корабль.
- [run_tutorial_review.ps1](run_tutorial_review.ps1) — полный solo проход урока и переход в рубку, шесть GPU кадров; `-NullRHI` — без графики. `run_network_smoke.ps1 -Suite Tutorial` проверяет этот маршрут Host/Client/LateJoin.

`run_crew_cargo_r3_review.ps1 -CrewLowPoly -GripViews` добавляет два world-space кадра хвата и статический замер реальных Chaos wrists (порог 3 см). Постановочная сцена, не cargo route; считать все PNG, не полагаться на старую строку views в логах.

- [run_automation.ps1](run_automation.ps1) — C++/asset tests (`-Editor`).
- [run_water_interaction_review.ps1](run_water_interaction_review.ps1) — пять GPU кадров gravity-driven входа груза; camera-only fixture, не обход и не автоматическая художественная приёмка.
- `run_network_smoke.ps1 -Suite WaterInteraction` — четыре типа груза, силы/вершины и сравнение параметров живых событий Host/Client/LateJoin.
- [run_network_smoke.ps1](run_network_smoke.ps1) — `-Suite Character/Helper/Ship/Water/Pause/Interaction/Door/BridgeMenu/...`, `-Editor`, уникальный loopback Port. BridgeMenu вход проверен в R2, 10 сентября.
- [run_physical_crew_review.ps1](run_physical_crew_review.ps1) — пять GPU кадров ходьбы/падения/подъёма; `-FallWater` добавляет шестой кадр и проверяет физическое падение → плавание без rescue. Водная постановка переносит тестового персонажа; это не traversal.
- [run_helper_solo.ps1](run_helper_solo.ps1), [run_pause_solo.ps1](run_pause_solo.ps1), [run_camera_preferences.ps1](run_camera_preferences.ps1).
- [run_crew_cargo_r3_review.ps1](run_crew_cargo_r3_review.ps1) — GPU fixture; `-CrewLowPoly -InteractionHUD`, старые имена R3/LP4 не версия текущего mesh.
- [run_ship_r7_review.ps1](run_ship_r7_review.ps1), [run_bridge_menu_review.ps1](run_bridge_menu_review.ps1) — реальные GPU; локальный путь engine внутри требует адаптации на новом ПК.
- `run_ocean_physics`, `test_*`, `verify_ocean_*`, `audit_*`, `recheck_*` — разные исторические стенды; сначала читать конкретный script и отчёт, не складывать PASS разных профилей.

## Авторинг персонажей

[create_crew_r8.py](create_crew_r8.py) / [import_crew_r8.py](import_crew_r8.py) — последний candidate; R7/R6/R5/R4/R3 сохранены. `create_*` создаёт output, `import_*` пишет Content; **не setup и не read-only**. Защиту существующей папки не обходить. Import R8 не был выполнен, чистый shutdown не доказан.

## Корабль, свет и материалы — миграции

- [audit_water_material.py](audit_water_material.py) — read-only старого FoamR3 и map binding. [create_water_interaction_material.py](create_water_interaction_material.py) создаёт только новый W1 asset (отказывается перезаписывать), [tune_water_interaction_material.py](tune_water_interaction_material.py) изменяет этот candidate один раз с проверкой исходного графа. Не setup; не запускать старый общий importer ради пены.

`build_ocean_*`, `ocean_freighter_*`, `export_ship_*`, `import_ship_*`, `export_r7_doors`, `fit_r7_door_swings`, `apply_r7_runtime_swings`, `fix_ship_*`, `apply_engine_dream_*`, `apply_nightwatch_*`, `refine_*`, `install_bridge_menu_r1.py`. Версия/порядок важны: прежний script может потерять поздние overrides, двери и свет. Не запускать весь каталог по очереди.

## Публикация / архивы / повышенный риск

- `freeze_*`, `archive_*`, `publish_*`, `promote_*` — неизменяемые evidence/candidate outputs, не повторять с прежним именем.
- [package_windows.ps1](package_windows.ps1) — build/cook + автоматическое продвижение Current, удаляет previous. Пока не безопасный двухэтапный smoke/promote.
- `install_dev_firewall_rule.ps1` — меняет системную сеть, не часть переноса/обычного запуска; без отдельного запроса не выполнять.

Полный актуальный список Scripts с размерами/хешами и перечень файлов с жёсткими путями — [INVENTORY](../Docs/Handover/INVENTORY.json). Новые housekeeping scripts живут в Maintenance, существующие сохраняют расположение для совместимости.
