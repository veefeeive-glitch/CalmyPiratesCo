# Журнал продолжения проекта

## 2026-10-30 — приватный GitHub игры

[Подготовка](../Changes/GITHUB_PREPARATION_2026-10-30.md): отдельная копия и очищенная история main, сохранён исходный репозиторий, Git LFS для ассетов. Игровой код и payloads не менялись. Автотесты существующей DLL — 61/61; чистая сборка/packaged/network/human gate остаются.

## 2026-10-30 — актуализация и чистка перед GitHub

[Отчёт](../Changes/CLEANUP_2026-10-30.md). Удалено 9,786 ГиБ генерируемых файлов, включая кэши двух worktree, IDE-решения и 9 .blend1 при сохранённых .blend. Обновлены текущие инструкции и пометки истории, SourceAssets R8/R6B, roadmap и инвентарь. Launcher принимает PortHub и путь движка. Android File Server отключён, литеральный токен удалён из рабочего конфига; история Git не переписывалась. Расширен ignore. Существующая запись Higgsfield и пользовательские правки сохранены. C++/UE-ассеты, Evidence, Saved/Verification, архивы и намеренные backups не менялись. Automation 61/61 на имеющейся DLL; чистая сборка, сеть/GPU и human gate ещё нужны. Коммит/push не выполнялись.

## 2026-09-16 — Full project audit

[Отчёт](../Changes/FULL_AUDIT_2026-09-16.md): срез после playability/S3/heavy doors/voyage; automation 61/61; критично — 0 remotes + untracked Content; package Share 12 сен устарел; CrewR8 doc drift.

## 2026-09-15/16 — Ship playability rework

[Отчёт](../Changes/SHIP_PLAYABILITY_REWORK_2026-09-15.md): `UCalmyPlayabilityProbe` + `analyze_ship_playability.py` (сетка пола всех палуб, обрывы, что закрыто леером/деталями, клетки использования задач), `playability_routes.py` + `run_ship_traversal_ue.ps1 -Routes`, `UCalmyPlaythroughProbe` (вахта → течь → груз → сдача → заправка, двери открываются по E), `UCalmyPhotoTourProbe` + `playability_shots.py`/`make_contact_sheets.py`; места задач вынесены в `ACalmyVoyageDirector::LeakValveLocal/LeakPoolLocal/FuelStationLocal/DeliveryLocal` и закреплены тестом `CalmyPiratesCo.Ship.ActivityAnchors`; вода течи разливается по танктопу трюма; насос скрыт за станцией S3, лампа приглушена; леер S2 пересобран из снятых кромок и переведён на Nanite; CC0-модели Poly Haven (`import_polyhaven_ship_props.py`, `place_ship_r7_props.py`).

## 2026-09-15 — R7 service rooms (CO2, paint store, deck store)

[Отчёт](../Changes/SHIP_DETAILS_S3_2026-09-15.md): `build_ship_r7_details.py` — `room()` и три помещения, проверенные против отрезков маршрутов и дуг дверей 15 и 28/29; `install_ship_r7_details.py` — светильники `S3_Light_*` (35% от `Nightwatch_Service_Fill`); `CalmyHudReviewProbe` — кадры 21–23, стадии 70…97, тайм-аут 205 с.

## 2026-09-15 — R7 deck details S3

[Отчёт](../Changes/SHIP_DETAILS_S3_2026-09-15.md): `Scripts/build_ship_r7_details.py` (проверка объёмов против мастера, S1, S2, собственных групп и проёмов входов в трюмы) и `Scripts/install_ship_r7_details.py` (Nanite, usage на проектных инстансах); леер S2 пересобран с отчётом `ShipTraversal-UE-20260915-113439`; `CalmyShipProbe` — 13 мешей и `SM_R7_82_DETAILS`; `CalmyHudReviewProbe` — кадры 16–20, стадии 70…91, тайм-аут 185 с. Справка `SHIP_REFERENCE_DETAILING.md`: отмечено уже существующее оборудование и пункты 1, 6, 11, 13, 14, 15.

## 2026-09-14 — Heavy doors, positional audio, breakdown arrow, R7 guardrails

[Отчёт](../Changes/HEAVY_DOORS_AUDIO_ARROW_RAILS_2026-09-14.md): `FCalmyDoorMotion` + `ACalmyDoor` (Grab/Drag/AutoTarget, Blocked, импакты по счётчику), RPC `ServerDoorGrab`/`ServerDoorDrag`; `FCalmyAudio::PlayAt` с `MakeAttenuation` (радиусы по звуку, окклюзия); `FCalmyBreakdownCompass` + стрелка в HUD (кадр 13); `UCalmyTraversalProbe` / `run_ship_traversal_ue.ps1`; `build_ship_r7_guardrails.py` (дуги створок по реальной ширине, фальшборт на кронштейнах) и `install_ship_r7_guardrails.py`; `limit_r7_landing_door_swing.py` (90°, 58/59 в корму); Door audit проверяет позу карты (`DOOR_SWING`, `map_pose_clear`). Справка по прототипу: `Docs/Current/SHIP_REFERENCE_DETAILING.md`.

## 2026-09-13 — Door material Nanite usage

[Аудит, находка 11](../Changes/SUPER_AUDIT_2026-09-13.md): `Scripts/fix_door_material_nanite_usage.py`, `/Game/Art/FreighterInterior/Materials/MI_Gltf_Default_Opaque_DS_Nanite` → `M_Gltf_Default_Nanite`; движковый `M_Default` пересохранён с исходным флагом `False`. HUD review 0 предупреждений, Door smoke/Visual PASS.

## 2026-09-13 — Ship rider carry fix

[Отчёт](../Changes/SHIP_RIDER_CARRY_FIX_2026-09-13.md): `ACalmyShipMotion::ConsumeRiderDelta` (накопление, `Last.Inverse() * Current`), `Cargo * Delta`, рой без порога, шаг отставшей мухи вдоль следа, проба FlyTravel в системе корабля; automation `Ship.RidersKeepSmallMotion`. FlyTravel ×3, Ship, Cargo, Helper, Carry, Melee, FlyMind, Shift, Leak, Tutorial, BridgeMenu, FlySwarm PASS; 57/57.

## 2026-09-13 — Remaining fixes + super audit

[Исправления](../Changes/REMAINING_FIXES_2026-09-13.md) и [аудит](../Changes/SUPER_AUDIT_2026-09-13.md): Settings smoke изолирован `-UserDir` (UE 5.8 пересчитывает `GGameUserSettingsIni` в `SaveSettings`), Melee под восьмисекторное колесо, `SwatAudioAt`/`VoiceAudioAt`, `FCalmySession::SeatsTakenAtPreLogin`, `FCalmyMeleePolicy`, свет S1, `LoadAccepted` из каталога, `run_automation.ps1 -UserDir`, FlySwarm клиент/карта. 56/56; все suite кроме FlyTravel PASS.

## 2026-09-13 — Cursor review fixes + R7 service spaces S1

[Отчёт](../Changes/CURSOR_REVIEW_FIX_AND_SHIP_S1_2026-09-13.md): 12 исправлений по ревью Voyage G4, регрессия Tutorial (коммит `9ab78a4`), недостающие помещения R7 через `build_ship_r7_service_spaces.py` / `install_ship_r7_service_spaces.py` с бэкапом карты. 56/56; Tutorial, Shift, Leak, Ship, Door, Carry, BridgeMenu PASS; HUD review 12 кадров. Master `8b4fe72d…` не изменён.

## 2026-09-13 — Audio voyage / atmosphere / combat SFX

[Отчёт](../Changes/AUDIO_VOYAGE_SFX_2026-09-13.md): procedural `FCalmyAudio` + `ACalmyAmbientBeds`; UI/voyage/doors/cargo/storm/leak/fuel/punch/flies. 55/55; Door/Bridge/Ship/Cargo/Carry/FlyTravel PASS; Melee P1 FAIL; DLL `F733684A…`.

## 2026-09-13 — Voyage Loop G4 (порт → миссия → море → порт)

[Отчёт](../Changes/VOYAGE_LOOP_G4_2026-09-13.md): PortHub + MissionBoard + contract → R7 G3b watch → RETURN TO PORT; menu/doors/live HOLD1 gate; 55/55; smoke PASS; DLL `50876B80…`.

## 2026-09-13 — Door visual V1 (watertight)

[Отчёт](../Changes/DOOR_VISUAL_V1_2026-09-13.md): 61 дверь на `SM_WatertightDoor`; закрытая посадка; 54/54; Door + Visual PASS; DLL `0F15C432…`.

## 2026-09-13 — Voyage G3b + cargo HOLD 1

[Отчёт](../Changes/VOYAGE_G3B_CARGO_HOLD_2026-09-13.md): длинный watch ~18 мин, 4 груза из трюма, storm/leak/fuel; FlyTravel P0; 54/54; Ship/Door/Cargo/Carry/Bridge/FlyTravel PASS; DLL `C53C5189…`.

## 2026-09-13 — Regression sweep + ship visual baseline

[REGRESSION](../Changes/REGRESSION_SWEEP_2026-09-13.md) / [SHIP_MODEL](../Changes/SHIP_MODEL_FIX_2026-09-13.md): 14 network PASS; 4 P1 FAIL; baseline 7 PNG.

## 2026-09-13 — Minimal HUD + wheelhouse screens H1

[Отчёт](../Changes/HUD_MINIMAL_H1_2026-09-13.md): минимальный HUD с раскладкой без наложений и автотестом, четыре экрана рубки на пультах корабля, вкладка настроек «Подсказки», проба `-CalmyHudReview`. 54/54; HUD review PASS (ship 1920×1080, ship 4:3, WaterLab); BridgeMenu smoke PASS. DLL `B3F5F9A0…`.

## 2026-09-12 — Bridge menu visual M1

[Отчёт](../Changes/BRIDGE_MENU_VISUAL_M1_2026-09-12.md): геометрия и камера меню рубки в системе корабля, post-process и camera cut, FOV под пропорции окна, раскладка и шрифт, hover по реальному движению курсора; проба: обход страниц, логи клика, учёт недавнего входа, стоячесть в системе корабля с порогом 25 см. 53/53; Visual PASS solo 1920×1080 / 21:9 / 4:3 и клиент; BridgeMenu smoke 3/3. DLL `AE6D1091…`.

## 2026-09-12 — Replay training host-only

[Отчёт](../Changes/REPLAY_HOST_ONLY_2026-09-12.md): удалён server RPC `ServerReplayTutorial` (любой клиент мог сделать `ServerTravel` всей сессии), политика `CanRequestSessionTravel` + automation, шаг в BridgeMenu probe. 53/53; BridgeMenu smoke 5/5 PASS после исправления гонки пробы (`menu_teleported_pawn` при входе клиента → REBASE). `Scripts/__pycache__` убран из Git. DLL `A4DE1FF0…`. INVENTORY не обновлён: нужен PowerShell 7.

## 2026-09-12 — Stream merge (Hero+Voyage+Sea+Net → main)

Интеграция параллельных потоков в `main` @ `6c7335d` через `cursor/stream-merge-2b40`. Build PASS; focused smoke PASS (VoyageCycle, Ship.*, StormScale, Bridge, SessionLanHelpers, HandSockets, HandleGrips). DLL `700A808B…`. PendingOtherStreams → `Saved/PendingOtherStreams/Archive-20260912-superseded`. Releases ZIP 2026-09-12 не трогали. Не fun gate.

## 2026-09-12 — Net G4 LAN session / 4 peers

[Отчёт](../Changes/NET_G4_LAN_SESSION_2026-09-12.md): меню CREW SESSION (Host LAN / Join / Copy), MaxPlayers=4, Session smoke 4-peer + Lag100 PASS, 52/52 automation. Steam gap задокументирован. DLL 6DEB7261….


## 2026-09-12 — Ship G2 storm / proxy / helm

[Отчёт](../Changes/SHIP_G2_STORM_PROXY_HELM_2026-09-12.md): ocean `StormIntensity`, ShipMotion `StormBoost`+helm+boarding ramp, `ACalmyHullProxy`. Build PASS; 52/52; Ship/Water PASS; DLL `0128BC98…`. Не Chaos R7 / fun gate.

## 2026-09-12 — Hero G1: mesh handle sockets + finger contact scaffold

[Отчёт](../Changes/HERO_G1_SOCKETS_FINGERS_2026-09-12.md): `Grip_*` на Cargo R3; Wrist/Palm/Tip на CrewR8; ResolveWorld + tip/curl. Build PASS; 2/2 automation; DLL `FFA664E9…`. Continuous skin нет.

## 2026-09-12 — HOLD 1 FPS: mind vs NoBrain

[Отчёт](../Changes/FLY_MIND_FPS_HOLD1_2026-09-12.md): Epic HOLD 1 — mind 80.87 FPS / swarm 1.97 мс, NoBrain 75.48 / 0.45 мс; vs S1 91.85. SNN не главный FPS-тормоз. DLL `D7D175ED…`.

## 2026-09-12 — Fly think + ship roam

[Отчёт](../Changes/FLY_THINK_TRAVEL_2026-09-12.md): travel-thoughts, HUD THINKING, soft mark nudge; FlyTravel/FlyMind PASS; DLL `D7D175ED…`.

## 2026-09-12 — FlyWire parity A (desktop-fly 668)

[Отчёт](../Changes/FLY_PARITY_A_2026-09-12.md): sensoryGate/activityScale, gait→ascending, burst noise, air puff; `DesktopFlyGatesAndGait` + SourceLIFParity PASS; FlyMind PASS; DLL `B6AA9C6D…`.

## 2026-09-12 — FlyWire driven by inscriptions

[Отчёт](../Changes/FLY_WIRE_INSCRIPTIONS_2026-09-12.md): wary/welcome метки → loom gain, airPuff sensory, ascending; `InscriptionsDriveFlyWire` PASS; FlyMind PASS; DLL `5D622F47…`.

## 2026-09-12 — Fly inscriptions (Dark Souls marks)

[Отчёт](../Changes/FLY_INSCRIPTIONS_2026-09-12.md): общие метки на рое, echoes, влияние на cognition, репликация/сейв v2. FlyMind PASS; SharedInscriptions + RadialSelection; DLL `1F2F4DAB…`.

## 2026-09-12 — Fly presence (занятия заметны)

[Отчёт](../Changes/FLY_PRESENCE_2026-09-12.md): utility Observe/Socialize, речь при смене занятия, HUD activity, travel-pause только у диалога E. FlyTravel/FlyMind PASS; Fly* 7/7; DLL `FC7F68A2…`.

## 2026-09-12 — Ship drift G2 + doors closed

[Отчёт](../Changes/SHIP_DRIFT_DOORS_2026-09-12.md): 61 дверь closed; `ACalmyShipMotion` от W1; 44/44; Door/Ship/Water/WaterInteraction/FlyTravel PASS; DLL `EB86E392…`.

## 2026-09-12 — Fly deck height (~1 m flight / rest on deck)

[Отчёт](../Changes/FLY_DECK_HEIGHT_2026-09-12.md): deck-relative Rest/FlightHeight; честный статус FlyWire SNN + cognition. FlyTravel/FlyMind PASS; Fly* automation 7/7; DLL `6AB74606…`.

## 2026-09-11 — Tutorial progress save + menu replay

[Отчёт](../Changes/TUTORIAL_PROGRESS_SAVE_2026-09-11.md): сохранение прохождения; автопосадка на корабль; «Обучение снова» в меню рубки. 44/44; Tutorial+BridgeMenu PASS.

## 2026-09-11 — Grip R2a (handles + moving wrist)

[Отчёт](../Changes/GRIP_R2A_HANDLES_2026-09-11.md): kind-specific grip locals; moving yaw probe ≤5 см. 43/43 automation; Helper + LP4 GripViews PASS; DLL `A935E36D…`. Пальцы/skin/sockets/package нет.

## 2026-09-11 — Crew R8 runtime (recovery)

[Отчёт](../Changes/CREW_R8_RUNTIME_2026-09-11.md): пути на CrewR8; recovery Overlap/мотор/soft-finish/ResetBodies. 42/42 automation; Character+Helper PASS; DLL `fbe951fa…`. Grips не начинались.

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
