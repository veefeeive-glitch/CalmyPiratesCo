# Calmy Pirates Co. — начать продолжение здесь

**Последнее изменение: [Git LFS baseline B1](Docs/Changes/GIT_BASELINE_B1_2026-09-11.md), 11 сентября.** Локальный первый коммит `7bb0048b431705a61b74406882677373d01a55d7` на `main`: 2880 tracked / 1826 LFS. Evidence/Builds/Binaries/Saved исключены из Git и остаются на диске. Remote и push нет. Gameplay/DLL/ZIP не менялись.

**Последнее изменение: [Repo check / doc fix](Docs/Changes/REPO_CHECK_FIX_2026-09-11.md), 11 сентября.** Повторная проверка после F1: ZIP P1 SHA256 совпал, editor DLL = S1 `dd75bb31…`, скрытые ссылки целы, `verify_project_docs` PASS. Исправлен устаревший README проекта (T1/9 сентября → актуальные Releases/S1/R6). Удалён пустой обломок `References/desktop-fly/.git`; настоящий desktop-fly git в скрытом `desktop-fly-reference` без изменений. Git UE по-прежнему 0 commits/0 tracked/нет remote; `Windows-Current` от 4 сентября ≠ share ZIP. Gameplay не трогался.

**Последнее изменение: [File organization F1](Docs/Changes/FILE_ORGANIZATION_F1_2026-09-11.md), 11 сентября.** Упорядочена вся E:/Game: текущий UE-проект на прежнем месте; готовый ZIP в Releases/Windows/2026-09-11, Unity/Studio/старые инструкции в Archive, desktop-fly доступен через References. Старые пути сохранены скрытыми ссылками; при переносе desktop-fly Windows отказала, частичный перенос восстановлен, репозиторий оставлен на месте и проверен Git fsck. Игровые файлы/версии P1/S1/R6 и ярлык прежние, удаления не выполнялись. Навигация: E:/Game/README.md; журнал/проверки в отчёте F1.

**Последнее изменение: [Windows share P1](Docs/Changes/WINDOWS_SHARE_P1_2026-09-11.md), 11 сентября.** Отдельный ZIP текущей Windows-игры для друга: E:/Game/CalmyPiratesCo-Windows-2026-09-11.zip. Исправлен cook динамических моделей CrewR6B/CargoR3 и материала меню; добавлен OceanFreighterR7 в MapsToCook. C++ S1/R6 не менялся. Самостоятельный EXE, обучение → корабль, новые настройки, инструкция/Play-Ship.cmd/redistributable/credits. Артефакт и точные packaged проверки в отчёте; Windows-Current и ярлык editor-game прежние. Нижние «package старый» относятся к предыдущим этапам; для отправки использовать новый отдельный ZIP.

**Последнее изменение: [Settings / performance S1](Docs/Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md), 11 сентября.** Лаги возле мух вызваны главным образом CPU-пересчётом океана. После кеширования фаз/устранения пустых выборок: 20.68 → 91.85 FPS в одинаковой HOLD 1 сцене, 1280×800 Epic; это локальный benchmark, не гарантия всех сцен. Добавлено Esc → Настройки: экран, графика, звук, переназначение клавиш, камера/HUD; применение/сохранение и 15-секундный откат видеорежима. Рой R6/SNN/карты/модели сохранены. Финальная DLL build7 dd75bb31…; 42/42 automation, Settings сеть/GPU на build6, WaterInteraction/Melee/BridgeMenu на build4. Точные границы, проверки и ранние неудачи в отчёте S1; package прежний. Старый ярлык запускает актуальную DLL с сохранёнными параметрами экрана/FPS. Дальше — обычная игра и оценка комфорта/FPS на маршруте пользователя. Нижние «последние» записи — история.

**Последнее изменение: [Fly travel R6](Docs/Changes/FLY_TRAVEL_R6_2026-09-11.md), 11 сентября.** Рой — один самостоятельный моб: общие потребности/память/цель, единое перемещение и компактное облако 32 SNN-мух. Начинает в HOLD 1, сам исследует доступное пространство корабля, отдыхает, наблюдает за людьми и уходит от угроз. Серверный инкрементальный поиск и sweeps проверяют препятствия; закрытые двери не открывает. Старые описания «только в HOLD 1» ниже — история. Build2 / 41 из 41 automation / FlyTravel Host–Client–LateJoin PASS: выход из отсека, 106.85 м за 125 с, максимальный радиус облака 55 см. Проверки интерфейса/памяти/GPU и ограничения — в отчёте. DLL `0470d68f926f898a11e2c8b0e552f38cdcdb46983044d43e6bdcccc9e1ce5246`. Source/DLL/scripts новые, карты и модели прежние; package не обновлён. Дальше — обычная игра и оценка заметности/темпа автономного поведения.

**Последнее изменение: [Interaction R5](Docs/Changes/INTERACTION_R5_2026-09-10.md), 10 сентября.** **ЛКМ пустыми руками — удар / взмах по мухам, с грузом — бросок. E на рое — колесо**, мышь/клик выбирают, E/Esc отменяют; отдельные J/K/L/O удалены. Миша предупреждает на 1–2 попадания, после 3 от одного игрока отвечает ему (12 HP, cooldown 1.25 с, до 8 с), прекращая помощь; обидчики считаются отдельно. Удар по мухам вызывает разлёт и память угрозы. Внешняя правая рука движется через моторную IK, поворот Миши плавный. Build5 / 40 из 40 automation / Melee и FlyMind Host–Client–LateJoin / Restore / GPU мыши PASS; Carry/Helper/BridgeMenu регрессии PASS на build4, версии в отчёте. DLL `02f9cbec18380aa5f398a41cfdfc589c6ea9320751acae05b8739c4d93a2126c`. Maps/меши/materials прежние, рой HOLD 1; обычный Миша не переносился (на кадрах test fixture). Frozen `Evidence/Gameplay/Interaction-R5-20260910`. Нового packaged build нет. Следующее — обычная игра и человеческая оценка удобства/реакций; физический контакт кулака, polished art/audio не заявлены. Нижние «последние» записи — предыдущие ревизии.

**Последнее изменение: [Fly habitat R3 / cognitive model R4](Docs/Changes/FLY_MIND_R4_2026-09-10.md), 10 сентября.** Один рой 32 мух теперь живёт в **HOLD 1** `(-800,0,390)`, а не в рубке; WaterLab без мух. Добавлены общая память/обучение по игрокам, потребности и автономные занятия, короткое общение **J/K/L/O** в пределах 4 м/видимости и серверный SaveGame. Это вычисляемая когнитивная модель поверх исходного SNN, **не установленное сознание и не свободный языковой диалог**. Пользователь отверг прежнее понимание как сюжетного названия. 38/38 automation, Host/Client/LateJoin, рестарт с восстановлением опыта, GPU обычного HUD PASS. Финальная DLL `5b83b29599d255d3da6338baee2a28992e65485edefc03dc19d8f9f183416bde`; новые maps/Source, арт мух R2 прежний, packaged build старый. Frozen `Evidence/Gameplay/Fly-Mind-R4-20260910`. NULL/LAN гости узнаются только в рамках подключения; хозяин — один локальный профиль. Нижние «последние» R2/R1 записи исторические. Дальше — оценка поведения в обычной игре; свободный диалог/богатое планирование/нейропластичность остаются отдельной работой.

**Последнее изменение: Fly in game R2, 10 сентября.** Мухи теперь **уже в игре**: 32 на WaterLab перед стартом, 32 над приборами рубки OceanFreighterR7. Оригинальный fly mesh с красными глазами/шестью лапками/движущимися крыльями назначен в saved actors; placeholder на этих роях заменён. [Отчёт и запуск](Docs/Changes/FLY_IN_GAME_R2_2026-09-10.md). Build, 35/35 automation, GPU и Host/Client/LateJoin обеих карт PASS. DLL `20f2bfb60aab554efd0313f48b05e26f3eaf42ed184851062bee7b56aab466b1`, обе карты изменены через UE API; backups до R2 сохранены. Старое «на картах не расставлен» ниже относится к R1. Старт по-прежнему Tutorial → Ship T1. Collision/урон/звук и новый package не добавлены.

**Последнее изменение: Fly Swarm R1, 10 сентября.** Добавлен отдельный `AFlySwarmMob`: 20–200 HISM-мух с независимыми LIF-состояниями и одним общим FlyWire-графом 668/18968; fixed timer + 1-мс подшаги, ParallelFor, серверная симуляция и poses для клиентов. [Код, экспорт и настройка](Docs/Changes/FLY_SWARM_2026-09-10.md). Сборка и 35/35 automation, Host/Client/LateJoin PASS. DLL теперь `2e1c43ec4b993e1494b5672da589c64ff2ed2c2197789b9c440bd9e7dd377b67`. Меш placeholder, на картах не расставлен; collision/урон/финальный fly art/package отсутствуют. Данные CC BY-NC 4.0. Tutorial → Ship T1 остаётся обычным входом; более старые «последняя DLL» ниже исторические.

**Последнее изменение: Tutorial → Ship T1, 10 сентября.** Обычный запуск теперь **WaterLab как обучение**, а не отдельный стенд: пройти 4 м → E взять деревянный ящик → G поставить в зелёной зоне → 8 секунд → вся команда в рубке OceanFreighterR7. Enter продолжает игру из корабельного терминала. [Полный отчёт и проверки](Docs/Changes/TUTORIAL_SHIP_T1_2026-09-10.md). Запуск: [Play-Calmy.cmd](Scripts/Play-Calmy.cmd) или `Scripts/start_game.ps1`; прямой вход на корабль: `-MapName OceanFreighterR7`. Прогресс обучения общий в рамках сессии, без сохранения между запусками. Лабораторные probes и `-CalmyLab` не запускают обучение. Нижние формулировки W1/R2 о запуске сразу на корабле описывают предыдущую версию.

Актуализировано **2026-09-10**. Это живой хендовер, не архивный отчёт. Все пути ниже относительны корню проекта. Последняя runtime-итерация — **[Water Interaction W1](Docs/Changes/WATER_INTERACTION_W1_2026-09-10.md) поверх Physical Crew / Bridge R2, с мешами Crew R6B**, последний Blender-кандидат — **Crew R8, не импортирован**. Нижние R6-хеши сохранены как история. Текущие Source/DLL уже другие.

Последняя настройка среды: [Rigify и MCP-мосты](Docs/Changes/TOOLING_SETUP_2026-09-09.md). Rigify проверен; соединения только loopback. Mixamo-вход пользователь выполнил, модели не загружались. Пользователь выбрал пока хранение на ПК, без облака. Локальный Git+LFS baseline B1 выполнен; remote/push, импорт R8 и исправление SemanticSearch — нет.

## 1. Коротко о проекте и приоритетах пользователя

**Последняя runtime-итерация:** [Water Interaction W1](Docs/Changes/WATER_INTERACTION_W1_2026-09-10.md): серверные удары/след у поверхности, общее локальное поле для геометрии воды и сил плавучести, передача живых волн позднему клиенту, отдельный исправленный материал/меш брызг. Это приближённая response-модель, не полный fluid solver/шторм. Сохраняется [Physical Crew / Bridge R2](Docs/Changes/PHYSICAL_CREW_BRIDGE_R2_2026-09-10.md): динамический торс, моторный подъём, bodycam, переход к плаванию и меню рубки. Обычный запуск — OceanFreighterR7. **Пальцевой обхват, полный active locomotion и физическое штормовое море не сделаны**; весь запрос не закрыт.

Современный грузовой корабль, first person/bodycam, low-poly с убедительными материалами, физическая кооперативная комедия для 2–4 игроков и solo с NPC. Исходный Unity-проект находится отдельно; текущая разработка — Unreal Engine 5.8.2. Не мигрировать обратно в Unity/Three.js. Последний художественный запрос добавил более тревожную морскую атмосферу, но готовых врагов/хоррор-AI нет.

Корабль и его внутренние палубы — центральное игровое пространство. Нужны реальные маршруты, машинное отделение, жилые/служебные помещения, груз и встречные люди; недостаточно красивого внешнего силуэта. Пользователь неоднократно отвергал качество корабля и героев. Процент готовности из числа тестов не вычислять; «топ» и человеческая приёмка не достигнуты. После документационного аудита выполнена runtime-итерация Grip/Health R1; её результаты выше, старые данные R6 ниже — baseline.

## 2. Что есть, а чего ещё нет

| Область | Реализовано / проверено | Не завершено |
|---|---|---|
| Герой | CharacterMovement при ходьбе; при падении десять физических частей на сервере, моторный подъём/clearance; R6B в игре | Непрерывный skin, active walking balance, bracing/get-up clips, длинный soak/сложные контакты |
| Камера/руки | Bodycam, инерция, проверка стены, comfort, чувствительность, видимые локальные IK-руки | Физический хват/лазанье и обхват отдельными пальцами; human comfort gate |
| NPC Миша | Заранее стоит на карте; H рядом просит помощь; тяжёлая доставка solo/сеть, dismissal/rehire | Двери сам не открывает; весь рейс в одиночку не пройден |
| Грузы | Четыре разных типа/модели; прицел, reach/LOS, отказы, перенос, бросок/передача по правилам | Все типы/ручки/узкие проходы, полный контракт/manifest, чистый итоговый арт |
| Вода | W1: события удара/следа меняют поверхность и силы плавучести; общая скорость воды для пловца, репликация; новые пена/капли | Полный fluid solver/шторм/финальный арт пены/прибой/звук, подвижное судно, гидродинамическое затопление |
| Корабль | Отдельная статическая OceanFreighterR7, сухие внутренние зоны, 61 интерактивная проходная дверь, 37-точечный сетевой маршрут | Движение/управление кораблём/качка, возврат с воды на судно, все отсеки и рабочие позиции |
| Атмосфера | Metal R1, Engine Dream, Nightwatch: матовая сталь, локальный свет, сумрак/туман, свои wind/hull loops | Локальные пересветы, тёмные персонажи, свет и материалы всех помещений |
| Меню | Bridge R2: старт в рубке, читаемый экран, настройки/resume/подтверждение выхода; solo freeze / локальная co-op | Сессии/приглашения, save/load, remapping/gamepad, локализация |
| Игра целиком | Смена/доставка/аварии/питание проверялись отдельными лабораториями | Единый рейс, прогресс/сохранения, сессии/приглашения, production audio/UI, свежий packaged build |

Подробности: [механики и управление](Docs/Current/GAMEPLAY.md), [судно](Docs/Current/SHIP.md), [roadmap игры](Docs/Roadmaps/GAME.md), [roadmap корабля](Docs/Roadmaps/SHIP.md).

## 3. Модели: что брать и чего не перепутать

Полный [каталог моделей и референсов](Docs/Handover/MODELS.md).

- **Последний авторинг:** [Crew_R8.blend](SourceAssets/Generated/CrewR8/Crew_R8.blend). Переносить всю [папку CrewR8](SourceAssets/Generated/CrewR8): 20 GLB, 20 PNG-текстур, четыре рендера и манифест. Текстуры `.blend` внешние, ссылки `//T_*.png`; GLB содержат встроенные изображения. 6012/6048 triangles. **R8 не импортирован в Unreal и не прошёл игровые проверки**.
- **В игре сейчас:** [Crew_R6.blend](SourceAssets/Generated/CrewR6B/Crew_R6.blend), `/Game/Characters/CrewR6B`; 4802/4882 triangles. Пути подтверждены в `CalmyCrewPresentation.cpp` и `CrewLowPolyTests.cpp`.
- **Промежуточный кандидат:** [Crew_R7.blend](SourceAssets/Generated/CrewR7/Crew_R7.blend), только Blender, 5462/5442 triangles; R6B/R7/R8 не перезаписывать друг другом.
- **AI-концепты:** [Миша](SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png), [Deckhand](SourceAssets/References/CrewR6/DECKHAND-CONCEPT-not-gameplay.png). Это PNG-ориентиры imagegen, не 3D и не runtime. Промпты рядом.
- **Master корабля:** [CalmyOceanFreighter.blend](SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend), R7-Cargo-C06. Runtime материалы/свет/двери доработаны отдельно в UE; простой повтор старого экспорта потеряет эти изменения.
- Для результатов другой программы предусмотрена [Incoming/Characters](SourceAssets/Incoming/Characters/README.md). Сохранять исходный файл, текстуры и происхождение; не заменять проверенные модели до сравнения и тестов.

R8 лучше проработан по методике, но не принят пользователем и всё ещё состоит из десяти жёстких деталей. AO запечён в base color, может затемнять в сумраке; пальцевой анимации нет. Его Blender CONTRACT PASS — сведения отчёта, **не Unreal automation PASS**.

## 4. Историческая база R6 и честные границы

Текущая DLL и результаты — в [Water Interaction W1](Docs/Changes/WATER_INTERACTION_W1_2026-09-10.md). Числа и хеши R6 ниже относятся к сохранённой исторической версии, не к новой воде.

[Crew R6 report](Docs/Changes/CREW_R6_2026-09-08.md), [frozen acceptance](Evidence/Gameplay/Crew-R6-20260908/acceptance.json), [manifest](Evidence/Gameplay/Crew-R6-20260908/manifest.json).

- Frozen baseline automation: **28/28**, 2026-09-08. R6B Character/Helper/Ship/Water/Pause — Host/Client/LateJoin, отдельно solo helper. Более новые результаты — в Grip/Health R1.
- Ship Host/Client: 37 точек weather → engine → return, 90,898/102,671 м, без resets/teleports. LateJoin не выполнял третий маршрут. Max joint **7,812 при лимите 8 см**, запас мал; длительный стенд после R6B не подтверждён.
- Девять GPU-кадров героев — постановочный fixture с grab/swim, не обход. Семь GPU-кадров судна — ограниченный реальный маршрут плюс три camera-only.
- Старые ship: 260/260 обязательных, 293/336 широких, 56/56 cargo/NPC — исторические наборы других ревизий/стендов. Не выдавать их за полный R8/current runtime проход; 43 старых отказа не удалять.
- В этом документационном аудите сверены хеши текущей DLL/карт и Source с frozen R6; игровые тесты заново **не запускались**. Машиночитаемый снимок: [INVENTORY.json](Docs/Handover/INVENTORY.json).

Проверенная DLL SHA256: `ce3b270d3b58f16e6efd2b6c4fb842f2667319a97afda3147735fed32fcb7ef3`.
Карты: OceanFreighterR7 `9804dcd7af9d9742006b34951cad355ce1caf89269a01cb366e40e1786c8f03a`; WaterLab `0122f4aca4ec281214979814e024fd3fb5a430856e284eed778d1a9f911ca180`.

### Открытые технические проблемы

1. **Git baseline локальный, remote нет:** [B1](Docs/Changes/GIT_BASELINE_B1_2026-09-11.md) — commit `7bb0048b431705a61b74406882677373d01a55d7`, LFS включён. `Evidence/` не в Git (~854 MB на диске). Клон без LFS fetch и без отдельной копии Evidence/Builds неполный. Push/remote — только по явному запросу. Off-device backup по-прежнему отдельная задача.
2. **BridgeMenu исходные отказы исправлены в R2:** input/visibility/paused rendering и server camera verification. Рубка BR0–BR3 стала обычным входом; Ship/Door fixtures по-прежнему выбирают прежние starts. PASS входа не равен завершённому рейсу/production menu.
3. **Выход импортера:** оба R6 import-процесса сохранили модели и упали при shutdown в SemanticSearch, exit 3. Отключение одним CLI-флагом не помогло. Чистые последующие загрузки/тесты passed; сам shutdown не исправлен. R7/R8 импортеры ещё не доказали чистое выполнение.
4. **Null в `GetCarryLocation` исправлен в Grip/Health R1:** добавлена отдельная проверка `HeldCargo->Mesh`. Старые automation warnings `World has no context!` остаются отдельным пунктом.
5. **Packaged share ≠ Windows-Current:** актуальный ZIP для друга — `E:/Game/Releases/Windows/2026-09-11` (P1). `Builds/Windows-Current` от 4 сентября устарел. `package_windows.ps1` автоматически продвигает пакет до smoke и удаляет previous; не запускать как безобидную проверку.
6. R7-ART-01/02/03: обвязка HX2, верхние крепления, мебель/отделка/стыки/свет. Полная проходимость встречного потока и рабочих мест не принята.

## 5. Продолжение на другом устройстве

Пошагово: [TRANSFER.md](Docs/Handover/TRANSFER.md). Документы переносимы; текущий tested build target — **Windows**, не обещание работы игры на любой ОС.

Перенести как минимум `.uproject`, `Source`, `Config`, `Content`, `Scripts`, `SourceAssets`, `Docs`, `Evidence`, README/HANDOVER/AGENTS, `.gitattributes`, `.gitignore`, `.vsconfig`. `Content` переносить целиком: UE зависимости между материалами/текстурами/мешами не определяются по имени папки. `SourceAssets/External` и `ThirdParty` — с лицензиями. Unity-исходник и MD стека из Downloads вне этого корня — отдельные материалы, не входят в перенос автоматически.

UE 5.8.2; Blender 5.2.1 LTS; Visual Studio Community 2026 / MSVC 14.50 / Windows SDK 10.0.22621.0 по последней сборке. Engine/tool installations не содержатся в репозитории. Старые runners имеют пути `E:/Games/...` и `E:/Tools/...`; новая [переносимая точка запуска](Scripts/Project.ps1) принимает `-EngineRoot` и `-BlenderExe`. Не искать инструменты по чужому профилю Valerii на новом ПК.

С 2026-09-11 есть локальный Git+LFS baseline ([B1](Docs/Changes/GIT_BASELINE_B1_2026-09-11.md)). Клон требует Git LFS. Отдельно скопировать `Evidence/` (и при необходимости `Builds/`, `Binaries/`, ZIP из `E:/Game/Releases`). Корневые junction/symlink `E:/Game` в этом репозитории не лежат.

```powershell
# Из корня перенесённого проекта, подставить существующие локальные пути:
./Scripts/Project.ps1 -Action Check -EngineRoot 'D:/Unreal/UE_5.8' -BlenderExe 'D:/Blender/blender.exe'
./Scripts/Project.ps1 -Action Build -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Automation -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Play -EngineRoot 'D:/Unreal/UE_5.8' -StartPaused
```

Не выполнять import/create/publish/fix скрипты как setup. Они могут перезаписать сцены/override/выходные файлы. Предпочитать существующие masters и отдельно именованный кандидат.

## 6. Следующая работа — порядок

1. Off-device backup: как минимум `Evidence/`, share ZIP в Releases, при необходимости Builds. Remote для Git — только после явного разрешения. Локальный baseline уже есть (B1).
2. Взять актуальный R8 и обе картинки, оценить будущую внешнюю модель пользователя. Не начинать заново с R6B по старому README.
3. Для действительно лучших героев — непрерывный skinned surface, веса плеч/локтей, пальцы/позы, low-poly силуэт и UV/material readability. Не выдавать ещё один жёсткий loft за законченного героя.
4. Импортировать только выбранный кандидат, пересоздать коллизии, обновить два mesh paths, build → automation → Character/Helper/Ship/Water → GPU позы/крупные планы → длительный стенд. Старые файлы оставить.
5. Bridge R2 entry/menu проверен отдельно. W1 добавил физически связанную локальную реакцию воды, но не шторм. Далее — пальцы/skin/реальные ручки, отдельный плавающий proxy и based movement/подвижный dry hull, затем большой корабль и полный рейс.
6. Затем покомнатный арт/физические маршруты, вода + подвижный ship proxy, цельный solo/co-op рейс и безопасная candidate → smoke → promote упаковка. Все открытые пункты — в roadmaps, не закрывать их этим хендовером.

## 7. Как документ остаётся актуальным

После каждой задачи исполнитель обязан обновить этот файл, [STATUS](Docs/Current/STATUS.md), затронутый Current/roadmap и добавить строку/запись в [CHANGELOG](Docs/Handover/CHANGELOG.md). Для значимой итерации — отдельный dated report в Docs/Changes; шаблон в [Templates](Docs/Handover/Templates/CHANGE_ENTRY.md). Это закреплено в [AGENTS.md](AGENTS.md).

```powershell
./Scripts/Maintenance/Update-HandoverInventory.ps1
./Scripts/verify_project_docs.ps1
```

Инвентарь генерируется автоматически командой; он **не знает**, нравится ли модель человеку и готова ли механика. Смысловые «сделано/не сделано/проверено» записывает исполнитель. Нет фонового демона, синхронизации или авто-push. При конфликте проверять Source/asset path + hash-bound evidence, затем править Current; History и frozen Evidence не переписывать.
