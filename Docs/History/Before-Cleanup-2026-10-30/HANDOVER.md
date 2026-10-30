> Снимок документа до актуализации 30 октября 2026. Статусы и команды ниже относятся к указанным в тексте этапам. Относительные ссылки адаптированы к месту хранения.

# Calmy Pirates Co. — начать продолжение здесь

**Последнее изменение: [Full audit 2026-09-16](../../Changes/FULL_AUDIT_2026-09-16.md), 16 сентября.** Editor PortHub→R7 играбелен (61/61). Критично: 0 remotes, огромный dirty tree. Package устарел. Дальше: сохранить → doc sync CrewR8 → human fun gate → share.

**Последнее изменение: [Переработка играбельности корабля](../../Changes/SHIP_PLAYABILITY_REWORK_2026-09-15.md), 15–16 сентября.** Проверка теперь идёт с точки зрения игрока: съёмка всех палуб в игре капсулой игрока, ходоки до каждой задачи и прохождение вахты настоящим персонажем (вахта → трюм → заделать течь → ящик → сдача → заправка) — **PASS**. Задачи перенесены на осмысленные места: клапан течи на переборке трюма 1 (вода у льяльного колодца), топливо на бункеровочной станции погодной палубы, метки маршрута на полу. Леер S2 пересобран по снятым кромкам (309 сегментов, Nanite), двери доступны с обеих сторон 61/61, исторические маршруты 291/332 без регрессий, обрывов 16 кластеров вместо 155. Добавлены CC0-модели Poly Haven (огнетушители, круги, светильники, щиты, бочки, знак у затопленного трюма). Automation 61/61, 21 смоук-набор PASS. Не закоммичено.

**Последнее изменение: [R7 deck details S3, раунд 4 — служебные помещения](../../Changes/SHIP_DETAILS_S3_2026-09-15.md), 15 сентября.** Помещение CO2 на твиндеке МО (22 баллона, коллектор, распределительные клапаны, шкаф пуска), малярная кладовая и кладовая палубного снабжения в носу, по светильнику в каждом (35% служебного света; две рабочие лампы носа вынесены из объёма кладовых). Проходы 291/332, падения 0/515, Door audit 61/61, Ship/Door/Interior/Cargo/FlyTravel PASS, automation 60/60, HUD review 23 кадра без VSM. Не закоммичено.

**Последнее изменение: [R7 deck details S3](../../Changes/SHIP_DETAILS_S3_2026-09-15.md), 15 сентября.** Детали по прототипу MPP (`build_ship_r7_details.py` + `install_ship_r7_details.py`, меш `SM_R7_82_DETAILS`, Nanite): бункеровочные станции на комингсе трюма 1, грибовидные вентиляторы трюмов, кницы и задрайки комингсов, S-band радар, VSAT/Inmarsat-C, GMDSS, компас и плоты на крыше мостика, прожекторы и тифон на мачте, забортные трапы под шлюпками. Леер S2 дополнен кормовым краем площадки у крыши мостика (левый борт). Проходы 291/332, падения 0/515, Door audit 61/61, смоуки и automation 60/60 PASS, предупреждение VSM устранено переводом на Nanite. `CalmyShipProbe` ждёт 13 мешей R7. Не закоммичено.

**Последнее изменение: [Heavy doors, audio, breakdown arrow, R7 guardrails](../../Changes/HEAVY_DOORS_AUDIO_ARROW_RAILS_2026-09-14.md), 14 сентября.** Тяжёлые двери как в Amnesia (E — открыть, удерживать E + мышь — тянуть створку), позиционные звуки с затуханием и окклюзией вместо «везде на корабле», стрелка поломки сверху экрана (течь/топливо, метры, палубы), in-game аудит проходимости и леер S2: падения 0/515, проходы 291/332 без регрессий, Door audit `map_pose_clear=61/61`, Door/Ship/Interior/Cargo/FlyTravel PASS. Двери площадок 52/53/55/56/58/59 раскрываются на 90°. Референс корабля для детализации: [SHIP_REFERENCE_DETAILING](../../Current/SHIP_REFERENCE_DETAILING.md). Не закоммичено.

**13 сентября, материалы дверей ([аудит](../../Changes/SUPER_AUDIT_2026-09-13.md), находка 11).** Материалы водонепроницаемых дверей переподключены на проектную копию glTF-цепочки с флагом Nanite, в игре больше не подменяются материалом по умолчанию. Внимание: движковый `E:/Games/UE_5.8/Engine/Plugins/Interchange/Assets/Content/gltf/M_Default.uasset` был пересохранён (флаг возвращён в исходное `False`); при сомнениях — «Проверить» установку UE 5.8 в лаунчере.

**Последнее изменение: [Ship rider carry fix](../../Changes/SHIP_RIDER_CARRY_FIX_2026-09-13.md), 13 сентября.** Корабль терял дельты движения меньше 0.05 за кадр и поворачивал незакреплённых пассажиров вокруг начала мира, поэтому мухи уходили в стенки, а груз и Миша смещались. Теперь `ConsumeRiderDelta` накапливает движение и поворачивает вокруг корпуса. FlyTravel ×3 PASS, automation 57/57, все затронутые smoke PASS. DLL `756a54fd…`.

**Последнее изменение: [Remaining fixes](../../Changes/REMAINING_FIXES_2026-09-13.md) + [Super audit](../../Changes/SUPER_AUDIT_2026-09-13.md), 13 сентября.** Smoke Settings и Melee снова PASS (изоляция `-UserDir`, восьмисекторное колесо); звук роя у клиентов; `PreLogin` учитывает загружающихся; удар по товарищу — толчок без урона; свет и звенья в цепном ящике. Аудит: automation 56/56 (изолированный профиль), 20 из 21 сетевых suite PASS; **FlyTravel открыт** (муха застревает в стенке надстройки, отставшие мухи). Главный риск — ничего не закоммичено и нет remote.

**Последнее изменение: [Cursor review fixes + R7 service spaces S1](../../Changes/CURSOR_REVIEW_FIX_AND_SHIP_S1_2026-09-13.md), 13 сентября.** Исправлены 12 дефектов механики Cursor (топливо/рестарт, дрейф фикстур, звук у клиентов, `SetActorLabel`, RPC) и регрессия Tutorial (приём ящика при обучении, урок → PortHub). На R7 добавлены цепной ящик, рубка аварийного генератора и провизии, судовой офис, фок-мачта и огни, марки осадки и название — отдельным мешем `SM_R7_80_SERVICE_SPACES`, мастер не изменён. 56/56; Tutorial/Shift/Leak/Ship/Door/Carry/BridgeMenu PASS. DLL `c7604ba1…`, карта `df6f2914…`.

**Последнее изменение: [Audio SFX 1–4](../../Changes/AUDIO_VOYAGE_SFX_2026-09-13.md), 13 сентября.** Procedural UI/voyage/doors/cargo + ambient wind/hull/sea + storm/leak/fuel + punch/flies. Automation **55/55**; Door/Bridge/Ship/Cargo/Carry/FlyTravel PASS; Melee wheel_hover FAIL (P1). DLL `F733684A…`. Не WAV banks / human listen.

**Последнее изменение: [Voyage Loop G4](../../Changes/VOYAGE_LOOP_G4_2026-09-13.md), 13 сентября.** PortHub → доска миссий → OceanFreighterR7 (G3b) → RETURN TO PORT. Playability gate (menu/doors/live HOLD1). Automation **55/55**; Door/Bridge/Ship/Cargo/Carry/FlyTravel PASS; DLL `50876B80…`. Не fun gate / Steam / Chaos / Shipping.

**Последнее изменение: [Door visual V1](../../Changes/DOOR_VISUAL_V1_2026-09-13.md), 13 сентября.** 61 дверь: `SM_WatertightDoor` вместо синих кубов; закрытая посадка (oversize/embed). 54/54; Door Host/Client/LateJoin + Visual PASS; DLL `0F15C432…`.

**Последнее изменение: [Voyage G3b + cargo HOLD 1](../../Changes/VOYAGE_G3B_CARGO_HOLD_2026-09-13.md), 13 сентября.** Старт «настоящей» игры: ~18 мин watch, 4 вида груза из HOLD 1 → green zone, storm/leak/fuel; FlyTravel P0 clamp. 54/54; Ship/Door/Cargo/Carry/BridgeMenu/FlyTravel PASS; DLL `C53C5189…`. Порт/миссии закрыты в G4 выше.

**Последнее изменение: [Regression sweep + ship visual baseline](../../Changes/REGRESSION_SWEEP_2026-09-13.md) / [SHIP_MODEL_FIX](../../Changes/SHIP_MODEL_FIX_2026-09-13.md), 13 сентября.** Матрица сети 14 PASS / 4 FAIL (Tutorial/Settings/Melee P1 backlog); ship baseline 7 PNG; R7-ART без опасного reimport.

**Последнее изменение: [Minimal HUD + wheelhouse screens H1](../../Changes/HUD_MINIMAL_H1_2026-09-13.md), 13 сентября.** HUD как в PEAK: прицел, подсказка только при цели, здоровье при ранении, плашка течи, короткие сообщения, субтитры; без наложений (`FCalmyHudLayoutSolver`). Вахта/течи/экипаж/море — на пультах рубки (`ACalmyBridgeDisplays`), управление и советы — Настройки → «Подсказки». 54/54; HUD review PASS (корабль 16:9 и 4:3, WaterLab); BridgeMenu smoke PASS. DLL `B3F5F9A0…`.

**Последнее изменение: [Bridge menu visual M1](../../Changes/BRIDGE_MENU_VISUAL_M1_2026-09-12.md), 12 сентября.** Меню рубки: квад точно по экрану 94×40 см, прикреплён к `ShipMotion_R7` вместе с прямой камерой; ручная экспозиция без lens-эффектов, сброс TSR при смене страницы, FOV под пропорции окна; RT 2048×872, Roboto, две колонки EN/RU; hover только по реальному движению курсора. 53/53; Visual PASS solo 1920×1080 / 21:9 / 4:3 и клиент; BridgeMenu smoke 3/3. DLL `AE6D1091…`. Страница BODYCAM недостижима.

**Последнее изменение: [Replay training host-only](../../Changes/REPLAY_HOST_ONLY_2026-09-12.md), 12 сентября.** Клиент больше не может перебросить сессию на WaterLab: server RPC `ServerReplayTutorial` удалён, `FCalmySession::CanRequestSessionTravel`. 53/53; BridgeMenu Host/Client/LateJoin **5/5** PASS после исправления гонки пробы (`menu_teleported_pawn` при входе клиента). DLL `A4DE1FF0…`. `SetActorLabel` в VoyageDirector не исправлен; INVENTORY не обновлён (нет PowerShell 7).

**Последнее изменение: Stream merge → `main` @ `6c7335d` (+ docs), 12 сентября.** Hero G1 + Voyage G3 + Sea G2 + Net G4 собраны в одном дереве `E:\Game\CalmyPiratesCoUE`. Build PASS; stream smoke PASS; DLL `700A808B…`. Не fun gate. Подробности — Project store `docs/stream-merge.md`.

**Последнее изменение: [Net G4 LAN session](../../Changes/NET_G4_LAN_SESSION_2026-09-12.md), 12 сентября.** LAN host/join в меню рубки; 4 peers + Lag100 Session PASS; 52/52; Steam invites нет. DLL 6DEB7261….

**Последнее изменение: [Ship G2 storm / proxy / helm](../../Changes/SHIP_G2_STORM_PROXY_HELM_2026-09-12.md), 12 сентября.** Ocean storm scale; ShipMotion `StormBoost`+helm+boarding ramp; `ACalmyHullProxy` physics test hull. 52/52 automation; Ship/Water PASS; DLL `0128BC98…`. Chaos R7 / Character boarding climb / fun gate **не** закрыты.

**Последнее изменение: [Hero G1 sockets/fingers](../../Changes/HERO_G1_SOCKETS_FINGERS_2026-09-12.md), 12 сентября.** `Grip_*` sockets на Cargo R3; `Wrist`/`Palm`/`Tip` на CrewR8 forearms; `ResolveWorld` + tip contact + curl. Build PASS; HandleGrips+HandSockets 2/2; DLL `FFA664E9…`. Continuous skin / articulated fingers / fun gate **не** закрыты.

**Последнее изменение: [HOLD 1 FPS mind](../../Changes/FLY_MIND_FPS_HOLD1_2026-09-12.md), 12 сентября.** Epic HOLD 1: с мозгом 80.87 FPS (`swarm_step` 1.97 мс), NoBrain 75.48 (`swarm_step` 0.45); vs S1 91.85 — сдвиг game thread от стека после S1, не от одного SNN. DLL `D7D175ED…`.

**Последнее изменение: [Fly think + roam](../../Changes/FLY_THINK_TRAVEL_2026-09-12.md), 12 сентября.** Рой говорит о маршруте, HUD THINKING, soft nudge от меток; FlyTravel/FlyMind PASS; DLL `D7D175ED…`.

**Последнее изменение: [FlyWire parity A](../../Changes/FLY_PARITY_A_2026-09-12.md), 12 сентября.** Sleep/arousal gates, gait→ascending, burst noise, air puff как в desktop-fly на 668. FlySwarm/FlyMind PASS; DLL `B6AA9C6D…`.

**Последнее изменение: [FlyWire ← inscriptions](../../Changes/FLY_WIRE_INSCRIPTIONS_2026-09-12.md), 12 сентября.** Метки посетителей усиливают/ослабляют loom, JO sensory и ascending в LIF. `InscriptionsDriveFlyWire` + FlyMind PASS; DLL `5D622F47…`.

**Последнее изменение: [Fly inscriptions](../../Changes/FLY_INSCRIPTIONS_2026-09-12.md), 12 сентября.** Dark Souls-метки на рое (колесо E, диагонали); echoes; репликация/сейв v2; влияют на занятие/речь. FlyMind PASS + SharedInscriptions; DLL `1F2F4DAB…`.

**Последнее изменение: [Fly presence](../../Changes/FLY_PRESENCE_2026-09-12.md), 12 сентября.** Занятия заметны: речь при смене, HUD `SHIP RESIDENT | ACTIVITY`, Observe/Socialize/Rest с разным темпом; пауза travel только у диалога E. FlyTravel/FlyMind PASS; Fly* 7/7; DLL `FC7F68A2…`. Не феноменальное сознание.

**Последнее изменение: [Ship drift + doors closed](../../Changes/SHIP_DRIFT_DOORS_2026-09-12.md), 12 сентября.** 61 дверь стартует закрытой; `ACalmyShipMotion` качает/дрейфует R7 от поля W1; dry-hull в rest-frame. 44/44 automation; Door/Ship/Water/WaterInteraction/FlyTravel PASS; DLL `EB86E392…`.

**Последнее изменение: [Fly deck height](../../Changes/FLY_DECK_HEIGHT_2026-09-12.md), 12 сентября.** Rest ≈ на палубе (`WalkClearanceCm`), полёт ~1 м (`FlightHeightCm`); цели пути deck-relative, без hard Z-snap. FlyWire 668/18968 + LIF + cognition без новых претензий на феноменальное сознание. FlyTravel/FlyMind PASS; Fly* automation 7/7; DLL `6AB74606…`.

**Последнее изменение: [Tutorial progress save](../../Changes/TUTORIAL_PROGRESS_SAVE_2026-09-11.md), 11 сентября.** После обучения прогресс пишется в `[Calmy.Progress]`; следующий обычный запуск сразу на корабль. В меню рубки — «REPLAY TRAINING / ОБУЧЕНИЕ СНОВА». 44/44 automation; Tutorial Solo + BridgeMenu Visual PASS. DLL `0D6DBDE5…`.

**Последнее изменение: [Grip R2a](../../Changes/GRIP_R2A_HANDLES_2026-09-11.md), 11 сентября.** Kind-specific grip locals (смещение к R3 ручкам) + измерение wrist при yaw. 43/43 automation; Helper + LP4 GripViews PASS (static ~1.2/0.8 см, moving ~1.2 см). DLL `A935E36D…`. Пальцы/skin, mesh sockets, proxy/package не делались.

**Последнее изменение: [Crew R8 runtime](../../Changes/CREW_R8_RUNTIME_2026-09-11.md), 11 сентября.** Runtime на **CrewR8**. Recovery: Overlap пола на моторе, чуть сильнее pull, soft-finish запас, ResetBodies после вставания. Automation 42/42; Character + Helper network PASS; DLL `fbe951fa…`. Grips/proxy/package не начинались.

**Последнее изменение: [Crew R8 import](../../Changes/CREW_R8_IMPORT_2026-09-11.md), 11 сентября.** Импорт Content; первая попытка runtime откатилась из‑за recovery — закрыто отчётом выше.

**Последнее изменение: [Automation smoke S1](../../Changes/AUTOMATION_SMOKE_S1_2026-09-11.md) / [Phased ops](../../Changes/PHASED_OPS_REMOTE_SKIP_OPS_2026-09-11.md) / [Backup B1](../../Changes/BACKUP_OFFDEVICE_B1_2026-09-11.md), 11 сентября.** См. CHANGELOG.

**Последнее изменение: [Git LFS baseline B1](../../Changes/GIT_BASELINE_B1_2026-09-11.md), 11 сентября.** Локальный первый коммит `7bb0048b431705a61b74406882677373d01a55d7` на `main`: 2880 tracked / 1826 LFS. Evidence/Builds/Binaries/Saved исключены из Git и остаются на диске. Remote и push нет. Gameplay/DLL/ZIP не менялись.

**Последнее изменение: [Repo check / doc fix](../../Changes/REPO_CHECK_FIX_2026-09-11.md), 11 сентября.** Повторная проверка после F1: ZIP P1 SHA256 совпал, editor DLL = S1 `dd75bb31…`, скрытые ссылки целы, `verify_project_docs` PASS. Исправлен устаревший README проекта (T1/9 сентября → актуальные Releases/S1/R6). Удалён пустой обломок `References/desktop-fly/.git`; настоящий desktop-fly git в скрытом `desktop-fly-reference` без изменений. Git UE по-прежнему 0 commits/0 tracked/нет remote; `Windows-Current` от 4 сентября ≠ share ZIP. Gameplay не трогался.

**Последнее изменение: [File organization F1](../../Changes/FILE_ORGANIZATION_F1_2026-09-11.md), 11 сентября.** Упорядочена вся E:/Game: текущий UE-проект на прежнем месте; готовый ZIP в Releases/Windows/2026-09-11, Unity/Studio/старые инструкции в Archive, desktop-fly доступен через References. Старые пути сохранены скрытыми ссылками; при переносе desktop-fly Windows отказала, частичный перенос восстановлен, репозиторий оставлен на месте и проверен Git fsck. Игровые файлы/версии P1/S1/R6 и ярлык прежние, удаления не выполнялись. Навигация: E:/Game/README.md; журнал/проверки в отчёте F1.

**Последнее изменение: [Windows share P1](../../Changes/WINDOWS_SHARE_P1_2026-09-11.md), 11 сентября.** Отдельный ZIP текущей Windows-игры для друга: E:/Game/CalmyPiratesCo-Windows-2026-09-11.zip. Исправлен cook динамических моделей CrewR6B/CargoR3 и материала меню; добавлен OceanFreighterR7 в MapsToCook. C++ S1/R6 не менялся. Самостоятельный EXE, обучение → корабль, новые настройки, инструкция/Play-Ship.cmd/redistributable/credits. Артефакт и точные packaged проверки в отчёте; Windows-Current и ярлык editor-game прежние. Нижние «package старый» относятся к предыдущим этапам; для отправки использовать новый отдельный ZIP.

**Последнее изменение: [Settings / performance S1](../../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md), 11 сентября.** Лаги возле мух вызваны главным образом CPU-пересчётом океана. После кеширования фаз/устранения пустых выборок: 20.68 → 91.85 FPS в одинаковой HOLD 1 сцене, 1280×800 Epic; это локальный benchmark, не гарантия всех сцен. Добавлено Esc → Настройки: экран, графика, звук, переназначение клавиш, камера/HUD; применение/сохранение и 15-секундный откат видеорежима. Рой R6/SNN/карты/модели сохранены. Финальная DLL build7 dd75bb31…; 42/42 automation, Settings сеть/GPU на build6, WaterInteraction/Melee/BridgeMenu на build4. Точные границы, проверки и ранние неудачи в отчёте S1; package прежний. Старый ярлык запускает актуальную DLL с сохранёнными параметрами экрана/FPS. Дальше — обычная игра и оценка комфорта/FPS на маршруте пользователя. Нижние «последние» записи — история.

**Последнее изменение: [Fly travel R6](../../Changes/FLY_TRAVEL_R6_2026-09-11.md), 11 сентября.** Рой — один самостоятельный моб: общие потребности/память/цель, единое перемещение и компактное облако 32 SNN-мух. Начинает в HOLD 1, сам исследует доступное пространство корабля, отдыхает, наблюдает за людьми и уходит от угроз. Серверный инкрементальный поиск и sweeps проверяют препятствия; закрытые двери не открывает. Старые описания «только в HOLD 1» ниже — история. Build2 / 41 из 41 automation / FlyTravel Host–Client–LateJoin PASS: выход из отсека, 106.85 м за 125 с, максимальный радиус облака 55 см. Проверки интерфейса/памяти/GPU и ограничения — в отчёте. DLL `0470d68f926f898a11e2c8b0e552f38cdcdb46983044d43e6bdcccc9e1ce5246`. Source/DLL/scripts новые, карты и модели прежние; package не обновлён. Дальше — обычная игра и оценка заметности/темпа автономного поведения.

**Последнее изменение: [Interaction R5](../../Changes/INTERACTION_R5_2026-09-10.md), 10 сентября.** **ЛКМ пустыми руками — удар / взмах по мухам, с грузом — бросок. E на рое — колесо**, мышь/клик выбирают, E/Esc отменяют; отдельные J/K/L/O удалены. Миша предупреждает на 1–2 попадания, после 3 от одного игрока отвечает ему (12 HP, cooldown 1.25 с, до 8 с), прекращая помощь; обидчики считаются отдельно. Удар по мухам вызывает разлёт и память угрозы. Внешняя правая рука движется через моторную IK, поворот Миши плавный. Build5 / 40 из 40 automation / Melee и FlyMind Host–Client–LateJoin / Restore / GPU мыши PASS; Carry/Helper/BridgeMenu регрессии PASS на build4, версии в отчёте. DLL `02f9cbec18380aa5f398a41cfdfc589c6ea9320751acae05b8739c4d93a2126c`. Maps/меши/materials прежние, рой HOLD 1; обычный Миша не переносился (на кадрах test fixture). Frozen `Evidence/Gameplay/Interaction-R5-20260910`. Нового packaged build нет. Следующее — обычная игра и человеческая оценка удобства/реакций; физический контакт кулака, polished art/audio не заявлены. Нижние «последние» записи — предыдущие ревизии.

**Последнее изменение: [Fly habitat R3 / cognitive model R4](../../Changes/FLY_MIND_R4_2026-09-10.md), 10 сентября.** Один рой 32 мух теперь живёт в **HOLD 1** `(-800,0,390)`, а не в рубке; WaterLab без мух. Добавлены общая память/обучение по игрокам, потребности и автономные занятия, короткое общение **J/K/L/O** в пределах 4 м/видимости и серверный SaveGame. Это вычисляемая когнитивная модель поверх исходного SNN, **не установленное сознание и не свободный языковой диалог**. Пользователь отверг прежнее понимание как сюжетного названия. 38/38 automation, Host/Client/LateJoin, рестарт с восстановлением опыта, GPU обычного HUD PASS. Финальная DLL `5b83b29599d255d3da6338baee2a28992e65485edefc03dc19d8f9f183416bde`; новые maps/Source, арт мух R2 прежний, packaged build старый. Frozen `Evidence/Gameplay/Fly-Mind-R4-20260910`. NULL/LAN гости узнаются только в рамках подключения; хозяин — один локальный профиль. Нижние «последние» R2/R1 записи исторические. Дальше — оценка поведения в обычной игре; свободный диалог/богатое планирование/нейропластичность остаются отдельной работой.

**Последнее изменение: Fly in game R2, 10 сентября.** Мухи теперь **уже в игре**: 32 на WaterLab перед стартом, 32 над приборами рубки OceanFreighterR7. Оригинальный fly mesh с красными глазами/шестью лапками/движущимися крыльями назначен в saved actors; placeholder на этих роях заменён. [Отчёт и запуск](../../Changes/FLY_IN_GAME_R2_2026-09-10.md). Build, 35/35 automation, GPU и Host/Client/LateJoin обеих карт PASS. DLL `20f2bfb60aab554efd0313f48b05e26f3eaf42ed184851062bee7b56aab466b1`, обе карты изменены через UE API; backups до R2 сохранены. Старое «на картах не расставлен» ниже относится к R1. Старт по-прежнему Tutorial → Ship T1. Collision/урон/звук и новый package не добавлены.

**Последнее изменение: Fly Swarm R1, 10 сентября.** Добавлен отдельный `AFlySwarmMob`: 20–200 HISM-мух с независимыми LIF-состояниями и одним общим FlyWire-графом 668/18968; fixed timer + 1-мс подшаги, ParallelFor, серверная симуляция и poses для клиентов. [Код, экспорт и настройка](../../Changes/FLY_SWARM_2026-09-10.md). Сборка и 35/35 automation, Host/Client/LateJoin PASS. DLL теперь `2e1c43ec4b993e1494b5672da589c64ff2ed2c2197789b9c440bd9e7dd377b67`. Меш placeholder, на картах не расставлен; collision/урон/финальный fly art/package отсутствуют. Данные CC BY-NC 4.0. Tutorial → Ship T1 остаётся обычным входом; более старые «последняя DLL» ниже исторические.

**Последнее изменение: Tutorial → Ship T1, 10 сентября.** Обычный запуск теперь **WaterLab как обучение**, а не отдельный стенд: пройти 4 м → E взять деревянный ящик → G поставить в зелёной зоне → 8 секунд → вся команда в рубке OceanFreighterR7. Enter продолжает игру из корабельного терминала. [Полный отчёт и проверки](../../Changes/TUTORIAL_SHIP_T1_2026-09-10.md). Запуск: [Play-Calmy.cmd](../../../Scripts/Play-Calmy.cmd) или `Scripts/start_game.ps1`; прямой вход на корабль: `-MapName OceanFreighterR7`. Прогресс обучения общий в рамках сессии, без сохранения между запусками. Лабораторные probes и `-CalmyLab` не запускают обучение. Нижние формулировки W1/R2 о запуске сразу на корабле описывают предыдущую версию.

Актуализировано **2026-09-10**. Это живой хендовер, не архивный отчёт. Все пути ниже относительны корню проекта. Последняя runtime-итерация — **[Water Interaction W1](../../Changes/WATER_INTERACTION_W1_2026-09-10.md) поверх Physical Crew / Bridge R2, с мешами Crew R6B**, последний Blender-кандидат — **Crew R8, не импортирован**. Нижние R6-хеши сохранены как история. Текущие Source/DLL уже другие.

Последняя настройка среды: [Rigify и MCP-мосты](../../Changes/TOOLING_SETUP_2026-09-09.md). Rigify проверен; соединения только loopback. Mixamo-вход пользователь выполнил, модели не загружались. Пользователь выбрал пока хранение на ПК, без облака. Локальный Git+LFS baseline B1 выполнен; remote/push, импорт R8 и исправление SemanticSearch — нет.

## 1. Коротко о проекте и приоритетах пользователя

**Последняя runtime-итерация:** [Water Interaction W1](../../Changes/WATER_INTERACTION_W1_2026-09-10.md): серверные удары/след у поверхности, общее локальное поле для геометрии воды и сил плавучести, передача живых волн позднему клиенту, отдельный исправленный материал/меш брызг. Это приближённая response-модель, не полный fluid solver/шторм. Сохраняется [Physical Crew / Bridge R2](../../Changes/PHYSICAL_CREW_BRIDGE_R2_2026-09-10.md): динамический торс, моторный подъём, bodycam, переход к плаванию и меню рубки. Обычный запуск — OceanFreighterR7. **Пальцевой обхват, полный active locomotion и физическое штормовое море не сделаны**; весь запрос не закрыт.

Современный грузовой корабль, first person/bodycam, low-poly с убедительными материалами, физическая кооперативная комедия для 2–4 игроков и solo с NPC. Исходный Unity-проект находится отдельно; текущая разработка — Unreal Engine 5.8.2. Не мигрировать обратно в Unity/Three.js. Последний художественный запрос добавил более тревожную морскую атмосферу, но готовых врагов/хоррор-AI нет.

Корабль и его внутренние палубы — центральное игровое пространство. Нужны реальные маршруты, машинное отделение, жилые/служебные помещения, груз и встречные люди; недостаточно красивого внешнего силуэта. Пользователь неоднократно отвергал качество корабля и героев. Процент готовности из числа тестов не вычислять; «топ» и человеческая приёмка не достигнуты. После документационного аудита выполнена runtime-итерация Grip/Health R1; её результаты выше, старые данные R6 ниже — baseline.

## 2. Что есть, а чего ещё нет

| Область | Реализовано / проверено | Не завершено |
|---|---|---|
| Герой | CharacterMovement при ходьбе; при падении десять физических частей на сервере, моторный подъём/clearance; **R8 в игре** | Непрерывный skin, active walking balance, bracing/get-up clips, длинный soak/сложные контакты |
| Камера/руки | Bodycam, инерция, проверка стены, comfort, чувствительность, видимые локальные IK-руки | Физический хват/лазанье и обхват отдельными пальцами; human comfort gate |
| NPC Миша | Заранее стоит на карте; H рядом просит помощь; тяжёлая доставка solo/сеть, dismissal/rehire | Двери сам не открывает; весь рейс в одиночку не пройден |
| Грузы | Четыре разных типа/модели; прицел, reach/LOS, отказы, перенос, бросок/передача по правилам | Все типы/ручки/узкие проходы, полный контракт/manifest, чистый итоговый арт |
| Вода | W1: события удара/следа меняют поверхность и силы плавучести; общая скорость воды для пловца, репликация; новые пена/капли | Полный fluid solver/шторм/финальный арт пены/прибой/звук, подвижное судно, гидродинамическое затопление |
| Корабль | Отдельная статическая OceanFreighterR7, сухие внутренние зоны, 61 интерактивная проходная дверь, 37-точечный сетевой маршрут | Движение/управление кораблём/качка, возврат с воды на судно, все отсеки и рабочие позиции |
| Атмосфера | Metal R1, Engine Dream, Nightwatch: матовая сталь, локальный свет, сумрак/туман, свои wind/hull loops | Локальные пересветы, тёмные персонажи, свет и материалы всех помещений |
| Меню | Bridge R2 + CREW SESSION LAN host/join/copy | Steam invites, save/load рейса, remapping/gamepad, локализация |
| Игра целиком | Смена/доставка/аварии/питание проверялись отдельными лабораториями | Единый рейс, прогресс/сохранения, сессии/приглашения, production audio/UI, свежий packaged build |

Подробности: [механики и управление](../../Current/GAMEPLAY.md), [судно](../../Current/SHIP.md), [roadmap игры](../../Roadmaps/GAME.md), [roadmap корабля](../../Roadmaps/SHIP.md).

## 3. Модели: что брать и чего не перепутать

Полный [каталог моделей и референсов](../../Handover/MODELS.md).

- **В игре сейчас:** [Crew_R8.blend](../../../SourceAssets/Generated/CrewR8/Crew_R8.blend), `/Game/Characters/CrewR8`; 6012/6048 triangles. Пути в `CalmyCrewPresentation.cpp` / `CrewLowPolyTests.cpp`. Character+Helper network PASS ([отчёт](../../Changes/CREW_R8_RUNTIME_2026-09-11.md)).
- **Предыдущий runtime (сохранён):** [Crew_R6.blend](../../../SourceAssets/Generated/CrewR6B/Crew_R6.blend), `/Game/Characters/CrewR6B`.
- **Промежуточный кандидат:** [Crew_R7.blend](../../../SourceAssets/Generated/CrewR7/Crew_R7.blend), только Blender; R6B/R7/R8 не перезаписывать друг другом.
- **AI-концепты:** [Миша](../../../SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png), [Deckhand](../../../SourceAssets/References/CrewR6/DECKHAND-CONCEPT-not-gameplay.png). Это PNG-ориентиры imagegen, не 3D и не runtime. Промпты рядом.
- **Master корабля:** [CalmyOceanFreighter.blend](../../../SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend), R7-Cargo-C06. Runtime материалы/свет/двери доработаны отдельно в UE; простой повтор старого экспорта потеряет эти изменения.
- Для результатов другой программы предусмотрена [Incoming/Characters](../../../SourceAssets/Incoming/Characters/README.md). Сохранять исходный файл, текстуры и происхождение; не заменять проверенные модели до сравнения и тестов.

R8 лучше проработан по методике, но не принят пользователем и всё ещё состоит из десяти жёстких деталей. AO запечён в base color, может затемнять в сумраке; пальцевой анимации нет. Его Blender CONTRACT PASS — сведения отчёта, **не Unreal automation PASS**.

## 4. Историческая база R6 и честные границы

Текущая DLL и результаты — в [Water Interaction W1](../../Changes/WATER_INTERACTION_W1_2026-09-10.md). Числа и хеши R6 ниже относятся к сохранённой исторической версии, не к новой воде.

[Crew R6 report](../../Changes/CREW_R6_2026-09-08.md), [frozen acceptance](../../../Evidence/Gameplay/Crew-R6-20260908/acceptance.json), [manifest](../../../Evidence/Gameplay/Crew-R6-20260908/manifest.json).

- Frozen baseline automation: **28/28**, 2026-09-08. R6B Character/Helper/Ship/Water/Pause — Host/Client/LateJoin, отдельно solo helper. Более новые результаты — в Grip/Health R1.
- Ship Host/Client: 37 точек weather → engine → return, 90,898/102,671 м, без resets/teleports. LateJoin не выполнял третий маршрут. Max joint **7,812 при лимите 8 см**, запас мал; длительный стенд после R6B не подтверждён.
- Девять GPU-кадров героев — постановочный fixture с grab/swim, не обход. Семь GPU-кадров судна — ограниченный реальный маршрут плюс три camera-only.
- Старые ship: 260/260 обязательных, 293/336 широких, 56/56 cargo/NPC — исторические наборы других ревизий/стендов. Не выдавать их за полный R8/current runtime проход; 43 старых отказа не удалять.
- В этом документационном аудите сверены хеши текущей DLL/карт и Source с frozen R6; игровые тесты заново **не запускались**. Машиночитаемый снимок: [INVENTORY.json](../../Handover/INVENTORY.json).

Проверенная DLL SHA256: `ce3b270d3b58f16e6efd2b6c4fb842f2667319a97afda3147735fed32fcb7ef3`.
Карты: OceanFreighterR7 `9804dcd7af9d9742006b34951cad355ce1caf89269a01cb366e40e1786c8f03a`; WaterLab `0122f4aca4ec281214979814e024fd3fb5a430856e284eed778d1a9f911ca180`.

### Открытые технические проблемы

1. **Git baseline локальный + off-device backup B1; remote нет:** commit `7bb0048…` / docs HEAD `47efb30…`; LFS включён. Backup: `D:\Backups\CalmyPiratesCo-2026-09-11` ([отчёт](../../Changes/BACKUP_OFFDEVICE_B1_2026-09-11.md)). `Evidence/` не в Git. Push/remote — только по URL от пользователя.
2. **BridgeMenu исходные отказы исправлены в R2:** input/visibility/paused rendering и server camera verification. Рубка BR0–BR3 стала обычным входом; Ship/Door fixtures по-прежнему выбирают прежние starts. PASS входа не равен завершённому рейсу/production menu.
3. **Выход импортера:** оба R6 import-процесса сохранили модели и упали при shutdown в SemanticSearch, exit 3. Отключение одним CLI-флагом не помогло. Чистые последующие загрузки/тесты passed; сам shutdown не исправлен. R7/R8 импортеры ещё не доказали чистое выполнение.
4. **Null в `GetCarryLocation` исправлен в Grip/Health R1:** добавлена отдельная проверка `HeldCargo->Mesh`. Старые automation warnings `World has no context!` остаются отдельным пунктом.
5. **Packaged share ≠ Windows-Current:** актуальный ZIP для друга — `E:/Game/Releases/Windows/2026-09-11` (P1). `Builds/Windows-Current` от 4 сентября устарел. `package_windows.ps1` автоматически продвигает пакет до smoke и удаляет previous; не запускать как безобидную проверку.
6. R7-ART-01/02/03: обвязка HX2, верхние крепления, мебель/отделка/стыки/свет. Полная проходимость встречного потока и рабочих мест не принята.

## 5. Продолжение на другом устройстве

Пошагово: [TRANSFER.md](../../Handover/TRANSFER.md). Документы переносимы; текущий tested build target — **Windows**, не обещание работы игры на любой ОС.

Перенести как минимум `.uproject`, `Source`, `Config`, `Content`, `Scripts`, `SourceAssets`, `Docs`, `Evidence`, README/HANDOVER/AGENTS, `.gitattributes`, `.gitignore`, `.vsconfig`. `Content` переносить целиком: UE зависимости между материалами/текстурами/мешами не определяются по имени папки. `SourceAssets/External` и `ThirdParty` — с лицензиями. Unity-исходник и MD стека из Downloads вне этого корня — отдельные материалы, не входят в перенос автоматически.

UE 5.8.2; Blender 5.2.1 LTS; Visual Studio Community 2026 / MSVC 14.50 / Windows SDK 10.0.22621.0 по последней сборке. Engine/tool installations не содержатся в репозитории. Старые runners имеют пути `E:/Games/...` и `E:/Tools/...`; новая [переносимая точка запуска](../../../Scripts/Project.ps1) принимает `-EngineRoot` и `-BlenderExe`. Не искать инструменты по чужому профилю Valerii на новом ПК.

С 2026-09-11 есть локальный Git+LFS baseline ([B1](../../Changes/GIT_BASELINE_B1_2026-09-11.md)). Клон требует Git LFS. Отдельно скопировать `Evidence/` (и при необходимости `Builds/`, `Binaries/`, ZIP из `E:/Game/Releases`). Корневые junction/symlink `E:/Game` в этом репозитории не лежат.

```powershell
# Из корня перенесённого проекта, подставить существующие локальные пути:
./Scripts/Project.ps1 -Action Check -EngineRoot 'D:/Unreal/UE_5.8' -BlenderExe 'D:/Blender/blender.exe'
./Scripts/Project.ps1 -Action Build -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Automation -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Play -EngineRoot 'D:/Unreal/UE_5.8' -StartPaused
```

Не выполнять import/create/publish/fix скрипты как setup. Они могут перезаписать сцены/override/выходные файлы. Предпочитать существующие masters и отдельно именованный кандидат.

## 6. Следующая работа — порядок

1. Off-device backup B1 на `D:\Backups\...` выполнен. Remote — после URL.
2. **R8 в runtime** (Character/Helper PASS). Дальше: GPU/человеческий взгляд; затем grips/пальцы/skin отдельным этапом.
3. Water proxy; package — после grips или по явному запросу.
3. Для действительно лучших героев — непрерывный skinned surface, веса плеч/локтей, пальцы/позы, low-poly силуэт и UV/material readability. Не выдавать ещё один жёсткий loft за законченного героя.
4. Импортировать только выбранный кандидат, пересоздать коллизии, обновить два mesh paths, build → automation → Character/Helper/Ship/Water → GPU позы/крупные планы → длительный стенд. Старые файлы оставить.
5. Bridge R2 entry/menu проверен отдельно. W1 добавил физически связанную локальную реакцию воды, но не шторм. Далее — пальцы/skin/реальные ручки, отдельный плавающий proxy и based movement/подвижный dry hull, затем большой корабль и полный рейс.
6. Затем покомнатный арт/физические маршруты, вода + подвижный ship proxy, цельный solo/co-op рейс и безопасная candidate → smoke → promote упаковка. Все открытые пункты — в roadmaps, не закрывать их этим хендовером.

## 7. Как документ остаётся актуальным

После каждой задачи исполнитель обязан обновить этот файл, [STATUS](../../Current/STATUS.md), затронутый Current/roadmap и добавить строку/запись в [CHANGELOG](../../Handover/CHANGELOG.md). Для значимой итерации — отдельный dated report в Docs/Changes; шаблон в [Templates](../../Handover/Templates/CHANGE_ENTRY.md). Это закреплено в [AGENTS.md](../../../AGENTS.md).

```powershell
./Scripts/Maintenance/Update-HandoverInventory.ps1
./Scripts/verify_project_docs.ps1
```

Инвентарь генерируется автоматически командой; он **не знает**, нравится ли модель человеку и готова ли механика. Смысловые «сделано/не сделано/проверено» записывает исполнитель. Нет фонового демона, синхронизации или авто-push. При конфликте проверять Source/asset path + hash-bound evidence, затем править Current; History и frozen Evidence не переписывать.
