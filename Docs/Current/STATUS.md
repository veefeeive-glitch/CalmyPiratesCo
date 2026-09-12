# Текущее состояние — 2026-09-12

**12 сентября — [Net G4 LAN session](../Changes/NET_G4_LAN_SESSION_2026-09-12.md).** Bridge CREW SESSION: Host LAN / Join IP:port / Copy Address; MaxPlayers=4; Session 4-peer + Lag100 smoke PASS; 52/52 automation. Steam/internet invites — gap (нет OSS). DLL `6DEB7261…`. Не fun gate.

**12 сентября — [Voyage G3](../Changes/VOYAGE_G3_2026-09-12.md) slice (A03).** На OceanFreighterR7: брифинг → curated смена (1 STANDARD + 1 BULKY) → leak → итог → host restart. `ACalmyVoyageDirector`; leak valve у green zone без `.umap`. Labs без директора — прежнее поведение. Automation: `CalmyPiratesCo.Gameplay.VoyageCycle`. Editor-playable; не полный 12–20 мин рейс и не human fun gate.

**12 сентября — [Hero G1 sockets/fingers](../Changes/HERO_G1_SOCKETS_FINGERS_2026-09-12.md).** Mesh handle sockets на Cargo R3 + Wrist/Palm/Tip на CrewR8 forearms; `ResolveWorld` + tip contact + cosmetic curl. Build PASS; HandleGrips+HandSockets 2/2; DLL `FFA664E9…`. Continuous skin / articulated fingers / art acceptance **нет**.

**12 сентября — [Ship G2 storm / proxy / helm](../Changes/SHIP_G2_STORM_PROXY_HELM_2026-09-12.md).** Storm scale + `StormBoost`, helm intent, boarding ramp, `ACalmyHullProxy`; 52/52; Ship/Water PASS; DLL `0128BC98…`. Не Chaos R7 / не fun gate.

**12 сентября — [HOLD 1 FPS mind](../Changes/FLY_MIND_FPS_HOLD1_2026-09-12.md).** Epic: mind 80.87 / NoBrain 75.48 FPS; SNN +~1.5 мс swarm_step, FPS упирается в render; vs S1 91.85 — game_ms↑ от стека после S1. DLL `D7D175ED…`.

**12 сентября — [Fly think + roam](../Changes/FLY_THINK_TRAVEL_2026-09-12.md).** Думает вслух о пути + путешествует; FlyTravel/FlyMind PASS; DLL `D7D175ED…`.

**12 сентября — [FlyWire parity A](../Changes/FLY_PARITY_A_2026-09-12.md).** Gates/gait/burst как desktop-fly на 668; FlyMind PASS; DLL `B6AA9C6D…`.

**12 сентября — [FlyWire ← inscriptions](../Changes/FLY_WIRE_INSCRIPTIONS_2026-09-12.md).** Метки → loom/JO/ascending LIF; FlyMind PASS; DLL `5D622F47…`.

**12 сентября — [Fly inscriptions](../Changes/FLY_INSCRIPTIONS_2026-09-12.md).** Общие метки Dark Souls на рое; echoes; сейв v2; FlyMind PASS; DLL `1F2F4DAB…`.

**12 сентября — [Fly presence](../Changes/FLY_PRESENCE_2026-09-12.md).** Занятия видны: речь/HUD/темп Observe·Socialize·Rest; travel-pause только у E. FlyTravel+FlyMind PASS; Fly* 7/7; DLL `FC7F68A2…`.

**12 сентября — [Ship drift + doors closed](../Changes/SHIP_DRIFT_DOORS_2026-09-12.md).** Двери закрыты на старте; freighter дрейфует от W1; 44/44; Door/Ship/Water/WaterInteraction/FlyTravel PASS; DLL `EB86E392…`.

**12 сентября — [Fly deck height](../Changes/FLY_DECK_HEIGHT_2026-09-12.md).** Отдых у палубы, полёт ~1 м; коннектом 668/18968 без претензии на феноменальное сознание. FlyTravel+FlyMind PASS; Fly* 7/7; DLL `6AB74606…`.

**11 сентября — [Tutorial progress save](../Changes/TUTORIAL_PROGRESS_SAVE_2026-09-11.md).** Прохождение сохраняется; повтор из меню рубки. 44/44; Tutorial+BridgeMenu PASS.

**11 сентября — [Grip R2a](../Changes/GRIP_R2A_HANDLES_2026-09-11.md).** Kind-specific хваты + moving yaw ≤5 см. 43/43; Helper+GripViews PASS; DLL `A935E36D…`. Пальцы/skin/package нет.

**11 сентября — [Crew R8 runtime](../Changes/CREW_R8_RUNTIME_2026-09-11.md).** В игре **CrewR8**; recovery motor укреплён. 42/42 automation; Character+Helper network PASS; DLL `fbe951fa…`.

**11 сентября — [Crew R8 import](../Changes/CREW_R8_IMPORT_2026-09-11.md).** Content импортирован; первая runtime-попытка откатилась — закрыта runtime-отчётом выше.

**11 сентября — [Automation smoke S1](../Changes/AUTOMATION_SMOKE_S1_2026-09-11.md).** 42/42 automation на S1 до R8; см. также прогон после R8/revert.

**11 сентября — [Phased ops: remote SKIP + ops](../Changes/PHASED_OPS_REMOTE_SKIP_OPS_2026-09-11.md).** Remote SKIP; JPG LFS; Unity safe.directory; Supermemory partial SKIP.

**11 сентября — [Off-device backup B1](../Changes/BACKUP_OFFDEVICE_B1_2026-09-11.md).** Копия на `D:\Backups\CalmyPiratesCo-2026-09-11`; ZIP/git/Evidence/key paths PASS; 6 nested Intermediate + 2 reparse probes задокументированы. Remote нет. Не gameplay PASS.

**11 сентября — [Git LFS baseline B1](../Changes/GIT_BASELINE_B1_2026-09-11.md).** Локальный commit `7bb0048b431705a61b74406882677373d01a55d7` на `main` (2880 files / 1826 LFS). Evidence и Builds не в Git. Remote/push нет. Это не gameplay PASS и не off-device backup.

**11 сентября — [проверка репо / doc fix](../Changes/REPO_CHECK_FIX_2026-09-11.md).** После F1 подтверждены ZIP P1 SHA256, DLL S1, рабочие ссылки и DOCS_PASS. README проекта приведён к текущему состоянию; пустой `References/desktop-fly/.git` удалён. На момент той проверки Git ещё был пуст; baseline B1 — отдельная следующая запись. Windows-Current от 4 сентября не является share ZIP. Это не новый gameplay PASS.

**11 сентября — [организация файлов F1](../Changes/FILE_ORGANIZATION_F1_2026-09-11.md).** Корень E:/Game разделён на CalmyPiratesCoUE, Releases, Archive и References; ZIP для друга перенесён в Releases/Windows/2026-09-11 с тем же SHA256. Скрытые ссылки сохраняют старые адреса. Windows заблокировала перенос desktop-fly, поэтому используется ссылка из References к сохранённому исходнику; Git целостность проверена. Gameplay/C++/карты/модели и последний package P1 прежние; это не новый runtime PASS.

**11 сентября — [Windows share P1](../Changes/WINDOWS_SHARE_P1_2026-09-11.md).** Подготовлен отдельный ZIP текущей Windows Development-игры для друга, с исправленным cook CrewR6B/CargoR3/BridgeMenu и включённым кораблём. Исходники gameplay/карты/модели прежние; новый standalone EXE включает S1/R6. Локальные packaged проверки и SHA256 в отчёте. Windows-Current не продвигался; предыдущие замечания о старом package не описывают этот ZIP. Steam/интернет-приглашения и приёмка на другом ПК остаются вне проверки.

**11 сентября — [Settings / performance S1](../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md).** Найден и исправлен CPU bottleneck пересчёта океана внутри корабля: 20.68 → 91.85 FPS в изолированном HOLD 1 benchmark (1280×800 Epic, 32 SNN-мухи). Полное меню текущих настроек: экран, графика, звук, клавиши, камера/HUD, сохранение и откат смены видеорежима. Автономный рой R6 не упрощался. Точные версии/tests/ограничения в отчёте; Source/DLL/scripts новые, карты/модели прежние, packaged build старый. Это не гарантия 60 FPS на любом маршруте и не human acceptance. Нижние записи относятся к предыдущим ревизиям.

**Актуально: [Fly travel R6](../Changes/FLY_TRAVEL_R6_2026-09-11.md).** Один рой с общей памятью/намерениями теперь сам перемещается из HOLD 1 по доступным пространствам корабля, исследует, отдыхает и реагирует на людей. Инкрементальный поиск, collision и возврат отставших мух по следу; цели/позиции реплицируются. Build2 / 41 из 41 automation / FlyTravel Host–Client–LateJoin PASS. Полные проверки и ограничения — в отчёте. Source/DLL новые; maps/mesh/materials и package прежние. Нижнее «житель только HOLD 1» больше не ограничивает runtime.

**Актуально: [Interaction R5](../Changes/INTERACTION_R5_2026-09-10.md).** ЛКМ пустыми руками бьёт, с грузом бросает. Миша отвечает после 3 попаданий от конкретного игрока; мухи разлетаются и запоминают угрозу. E на мухах открывает радиальное колесо вместо J/K/L/O. Исправлены input lease/отмена/подтверждение без лишнего удара, плавный разворот и punch IK. Финальные build5/40 из 40 automation/Melee и FlyMind сети/Restore/GPU PASS; carry/helper/menu регрессии — в отчёте. Новые Source/DLL, карты и модели прежние; старый package не обновлён. R4 ниже описывает предыдущий интерфейс.

**Актуально: [Fly mind R4 и перенос R3](../Changes/FLY_MIND_R4_2026-09-10.md).** 32 мухи только в HOLD 1, обучение и рубка без роя. Вычисляются память о посетителях, обучение угрозе, потребности/занятия; J/K/L/O для коротких обращений, опыт хозяина переживает перезапуск. Исходная SNN сохранена. Сознание не подтверждено; свободный языковой диалог не реализован. Build / 38 из 38 automation / сеть трёх peers / новый процесс Restore / GPU обычного HUD PASS. Новый Source/DLL и карты, R2 mesh/materials прежние. Подробные границы и evidence в отчёте; нижние R2/R1 состояния — история.

**Fly in game R2:** [мухи размещены и проверены](../Changes/FLY_IN_GAME_R2_2026-09-10.md): 32 в WaterLab и 32 в рубке R7, свой меш 1840 треугольников, анимация крыльев. Saved maps/новые материалы, свежая DLL поверх R1/T1. Build/35 из 35 automation/GPU/Host–Client–LateJoin на обеих картах PASS. Предыдущая запись R1 о placeholder и отсутствии размещения теперь историческая; урона/collision/звука/package нет.

**Fly Swarm R1 — новое программное существо:** [реализация и проверки](../Changes/FLY_SWARM_2026-09-10.md). Один общий коннектом 668/18968, до 200 отдельных мозгов, HISM, 25-Гц таймер с 1-мс нейронами, серверная authority. Development Editor build / 35 из 35 automation / Host–Client–LateJoin PASS. Это последняя source/DLL поверх T1; maps и Crew R6B meshes прежние. Fly mesh — placeholder, размещение на карте, collision/урон и финальный арт остаются. FlyWire данные CC BY-NC 4.0.

Главная точка продолжения: **[HANDOVER](../../HANDOVER.md)**. [Журнал](../Handover/CHANGELOG.md) · [Модели](../Handover/MODELS.md) · [Перенос](../Handover/TRANSFER.md).

## Текущая версия и неимпортированная работа

- Tutorial → Ship T1: обычный запуск WaterLab; после прохождения прогресс сохраняется и следующий старт — сразу корабль; повтор из меню рубки. [Отчёт save](../Changes/TUTORIAL_PROGRESS_SAVE_2026-09-11.md).

- Игра: **Crew R6B meshes + [Water Interaction W1](../Changes/WATER_INTERACTION_W1_2026-09-10.md)** поверх Physical Crew / Bridge R2. Локальные удары/след меняют геометрию воды и buoyancy forces, живые события реплицируются. Добавлен один собственный material asset; карты/модели прежние. Свободное падение/моторный подъём/bodycam/меню сохранены. Обычный запуск — OceanFreighterR7.
- Проверено W1: **31/31 automation**, WaterInteraction/Water/Character/Ship в Host/Client/LateJoin, 11 GPU кадров. Художественное качество воды PARTIAL: кольцевая форма ещё заметна, капли/подводная оптика нуждаются в доработке. Полные хеши/ограничения — в отчёте W1.
- Последний авторинг и **runtime:** **Crew R8** в `/Game/Characters/CrewR8`. R6B Content сохранён. Character/Helper network PASS на R8; Ship/Water на этой DLL в этом прогоне не повторялись.
- **BridgeMenu R2 entry работает:** world-space экран читается, кнопки/настройки/resume, solo freeze, Host/Client/LateJoin. Сессии/сохранения/полная локализация ещё не реализованы. WaterLab сохранён для отдельных тестов.
- Предыдущая настройка — [Rigify/MCP](../Changes/TOOLING_SETUP_2026-09-09.md): Rigify включён, generation PASS; Blender/Unreal отвечали на loopback. Для Grip/Health сборки служебный Unreal Editor штатно закрыт; живое подключение не гарантировано после закрытия. Git: локальный baseline B1 `7bb0048…`; remote нет; Evidence не tracked.

## Реализовано и открыто

Подробная матрица — в HANDOVER, [GAMEPLAY](GAMEPLAY.md) и [SHIP](SHIP.md). G0/G1/G2/G3/G4 остаются частичными; G5/G6 не приняты. Ни проценты готовности, ни human fun gate, ни измеренные 60 FPS не заявлены.

| ID старого аудита | Статус | Что важно сейчас |
|---|---|---|
| A01 сохранность | Частично | Git+LFS B1 + off-device backup на D:; Evidence вне Git; remote ещё нет |
| A02 упаковка | Открыт | Windows-Current старый; package_windows продвигает до smoke и удаляет previous |
| A03 цельный рейс | Частично | G3 slice на R7: briefing→curated shift→leak→outcome→restart; полный 12–20 мин / 4 peers / human gate открыты |
| A04 сеть | Частично | 4-peer Session + Lag100 loopback PASS; LAN join UI; не интернет/Steam/host migration |
| A05 герой | Частично | R8 runtime + Grip R2a + G1 sockets/tip curl; continuous skin / articulated fingers / human art acceptance открыты |
| A06 вода | Частично | W1 + G2 storm scale / ShipMotion ride / HullProxy; не полный fluid solver, не Chaos на всём R7 |
| A07 NPC/грузы | Частично | Постоянный NPC, aim/reach grab, тяжёлая solo доставка; не весь solo-контракт и не точные физические хваты |
| A08 маршруты | Частично | R6B повтор 37-точечного Ship; старые 260/260 и 56/56 не заменяют полный current sweep |
| A09 арт | Открыт | Герои не приняты; R7-ART-01/02/03, все помещения, тёмные персонажи/пересветы |
| A10 документация | Обновлена | HANDOVER/журнал/перенос/инвентарь, обязательное поддержание в AGENTS; не автосинхронизация |
| A11 продукт | Частично | BridgeMenu + CREW SESSION LAN host/join; Steam invites/сохранения рейса/full sound ещё открыты |
| A12 воспроизводимость | Частично | Сохранены исходники/evidence; fresh rebuild с нуля и приёмка переноса на другом ПК не доказаны |

## Ближайшая работа

Следующее по герою: continuous skinned body (нет SKM в Content — список в [G1 отчёте](../Changes/HERO_G1_SOCKETS_FINGERS_2026-09-12.md)); articulated fingers; GPU grip re-check после sockets. В физике моря — отдельный плавающий proxy, based movement и подвижный dry hull до большого R7. Не называть моторный подъём полноценной Euphoria, а локальное response-поле — полной fluid simulation.

1. Пользователь выбрал пока локальное хранение; off-device backup/проверка переноса остаются будущей задачей.
2. Выбор последнего героя R8/внешнего candidate пользователя; непрерывная skinned одежда/веса/пальцы вместо нового набора жёстких труб.
3. Отдельный импорт/коллизии/build/automation/network/GPU и физический soak, прежде чем объявлять кандидата runtime.
4. Корабельный арт/реальные маршруты, вода+ship proxy и цельный рейс. Меню рубки R2 не закрывает эти этапы.

[Roadmap игры](../Roadmaps/GAME.md) · [Roadmap корабля](../Roadmaps/SHIP.md). Предыдущие результаты: [R6](../Changes/CREW_R6_2026-09-08.md), [R7 candidate](../Changes/CREW_R7_2026-09-08.md), [R8 candidate](../Changes/CREW_R8_2026-09-08.md), [Nightwatch](../Changes/NIGHTWATCH_R1_2026-09-08.md), [вся история](../INDEX.md).

Старый STATUS до этого аудита сохранён в Docs/Archive/Before-Handover-20260909.zip. Не считать исторические «последние изменения» текущим runtime. Новые факты записывать сюда и в HANDOVER/CHANGELOG; frozen Evidence не переписывать.
