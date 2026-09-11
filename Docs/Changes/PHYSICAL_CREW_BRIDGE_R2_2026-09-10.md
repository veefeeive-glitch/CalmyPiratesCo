# Physical Crew / Bridge R2 — 10 сентября 2026

Запрос: active ragdoll, пальцевой обхват, главное меню на корабле и физическое штормовое море. **В этой итерации реализованы физическое падение/моторный подъём и корабельный entry/menu. Весь запрос из четырёх пунктов не закрыт.** Пальцы, skinned персонаж и новая физика моря не добавлены.

## Что изменено

- В `CalmyCrewPresentation` торс во время падения действительно становится динамическим Chaos body на сервере; вместе с ним работают все десять частей. Поддерживающая вертикальная капсула отключается, её координаты следуют за телом. Защитная поза рук задаётся слабыми угловыми приводами.
- Клиенты получают серверные root position/rotation, capsule anchor и epoch падения; девять локальных конечностей следуют этому торсу. Клиентские тела не определяют исходы игры. Dedicated server больше не пропускает создание физического тела.
- Подъём использует силы и момент на торсе, усиливающиеся суставные приводы и проверку фактической позиции/ориентации. Нет мгновенного вставания по истечении одного таймера. Проверяются пол, скорость тела и свободный объём полной капсулы. Под низким потолком или у препятствия подъём откладывается.
- При переходе в воду тела используют существующую плавучесть. После выхода торса к поверхности управление передаётся плаванию; обычный этот переход не считается спасением. При HP=0 сохраняется существующий отдельный safe rescue.
- Камера при падении получает положение реального торса и ограниченный наклон; steady сохраняет горизонт. Видимые владельцу руки повторяют физические, а не остаются в стоячей IK-позе.
- Исправлены attachment-перенос конечностей при подъёме и кэшированная kinematic target при reset/rescue. `ResetBodies` теперь явно обновляет физический transform, даже если scene transform уже совпал с целевым.
- Терминал рубки видим через отдельную камеру. Убрано размазывание изображения на паузе: camera/full paused tick и локальные настройки постобработки. Исправлены стартовый mouse hover, обработка реальных нажатий в тесте, ClientRestart и серверная проверка ViewTarget, которая возвращала подключившемуся игроку камеру персонажа поверх меню.
- V/M/чувствительность обновляют текст терминала. Solo замораживает мир; сетевое меню остаётся локальным. Resume возвращает камеру и ввод, не переносит персонажа.
- GameDefaultMap/ServerDefaultMap и launchers `Project.ps1`/`start_game.ps1` теперь выбирают OceanFreighterR7. EditorStartupMap и отдельный WaterLab сохранены. Карты/меши/материалы `.uasset` не перезаписывались.

## Версии и сохранность

Runtime meshes: **Crew R6B**. Blender candidate: **Crew R8, не импортирован**. Ship: прежний статический R7, 61 дверь. Новые приложения/ассеты не скачивались. Исходники до изменений сохранены в `Saved/Before-PhysicalCrew-R2-20260910/Source`. Git по-прежнему без tracked baseline/commit/remote; пользователь выбрал пока хранить проект локально, без облачной загрузки. Вход в Mixamo ранее выполнен пользователем; модели туда не отправлялись.

Финальная DLL SHA256: `dec6d435f3e56eba61fbf58320543e9525357eaa049278d0ace5ed419bb82ed5`.

## Проверки

Все строки ниже относятся к финальной DLL dec6d435…; предыдущие кандидаты не смешаны с этим набором. [Frozen acceptance](../../Evidence/Gameplay/PhysicalCrewBridge-R2-20260910/acceptance.json) · [SHA256 manifest](../../Evidence/Gameplay/PhysicalCrewBridge-R2-20260910/manifest.json).

| Проверка | Evidence в Saved | Результат |
|---|---|---|
| Automation | Logs/Automation-20260910-012546.log | **30/30 PASS** |
| Character, Host/Client/LateJoin | Verification/20260910-012546-Character | PASS; root drop 79.5/75.0 см, joint max 0.07/0.02/0.01 см, rescue=0 |
| BridgeMenu, Host/Client/LateJoin | Verification/20260910-012546-BridgeMenu | PASS; spawn/menu/settings/resume/clock/walk/no teleport |
| Ship, Host/Client/LateJoin | Verification/20260910-012845-Ship | PASS; 37 точек, Host/Client 9088.6/10264.0 см; joint 7.812/8 см; resets=0, teleports=0. LateJoin route не выполнял |
| Helper, Host/Client/LateJoin | Verification/20260910-012846-Helper | PASS; heavy delivery/replay/dismiss-rehire/disconnect; pace минимум 80 |
| Water, Host/Client/LateJoin | Verification/20260910-012846-Water | PASS; плавание/рампа/груз/запрет хвата/no instant rescue. Не новый шторм |
| Pause, Host/Client/LateJoin | Verification/20260910-012847-Pause | PASS; повторное открытие/ввод/clock/камера |
| Solo helper | Logs/Helper-Solo-20260910-013516.log | PASS |
| GPU physical crew | Verification/PhysicalCrew-Visual-20260910-012418 | PASS, 6 кадров; physical fall → swimming, rescue=0, max joint .21 см |
| GPU bridge | Verification/BridgeMenu-Visual-20260910-012545 | PASS, 3 кадра; читаемые terminal/settings/bodycam, реальный mouse click |
| GPU grip regression | Verification/LP4-Visual-20260910-013505 | PASS, 10 кадров; world wrist error герой 1.120 см, Миша .621 см (лимит 3 см) |

Всего 19 GPU кадров. Скрипт `run_physical_crew_review.ps1 -FallWater` делает шесть кадров: стойка, ходьба, падение, восстановление, first person и отдельное падение в воду. Последняя постановка содержит явно отмеченный перенос тестового персонажа; это **не обход борта/лестницы**. Не было нового packaged build, четырёх реальных игроков или human acceptance. Запас joint на корабле остаётся малым: 7.812 из 8 см, долгий soak не доказан.

Новый automation `Character.PhysicalRecoveryClearance` проверяет отсутствие пола, свободную капсулу, низкий потолок, боковое препятствие и его удаление, слишком далёкую опору и недопустимые размеры. Это проверка того же collision-query, не полный физический сценарий зажатого под потолком тела.

Промежуточные прогоны сохраняются в Saved, не переименовываются в PASS:

- `20260910-004259-Character`: отрыв конечностей при переносе recovery capsule, 64.79 см. Исправлен attachment.
- `BridgeMenu-Visual-20260910-004533`: кнопки прошли, но крупный кадр показал нечитаемый размытый экран. Визуально не принят.
- Следующие BridgeMenu failures выявили cursor-dependent selection и отдельно смену ViewTarget у клиента. Исправлены код и диагностические сообщения; проверки не отключались.
- `PhysicalCrew-Visual-20260910-011256`, `011634`, `012045`: отрыв тела после тестового переноса. Ни ожидание кадра, ни один scene transform не исправили проблему. Явный physics transform reset устранил кэшированную kinematic target.

## Что не считать готовым

Это **не полноценная Euphoria/GTA IV система**: обычная ходьба по-прежнему CharacterMovement + kinematic torso; нет непрерывного skeletal skin, физического баланса стоящего тела, contact-driven palm bracing, анимационных get-up clips, полного damage/revive/death loop. Локальные конечности не реплицируются покостно. Плохая сеть, присоединение ровно посреди падения, длительный физический soak и физически зажатое тело требуют отдельных прогонов. Human comfort/fun gate не выполнен.

Пальцы R6B остаются частью неподвижных мешей перчаток: нового обхвата отдельных пальцев, проверенных ручек всех грузов и настоящего force-based удержания нет. Нельзя помечать этот пункт закрытым наличием прежних wrist IK.

Код `CalmyOcean` не менялся. Новая fluid simulation, physical storm, динамический корпус, затопление и вылет груза при качке **не реализованы**. Применение существующей buoyancy к падающему телу не закрывает эти задачи. Шторм сначала проверять на подвижном тестовом корпусе, затем переносить на большое судно.

Меню закрывает старт в рубке, resume, настройки и подтверждение выхода. Нет session browser/invites, save/load, remapping/gamepad, production localization и полного рейса. Старый `Builds/Windows-Current` не обновлён; запускать текущую editor-game сборку.

## Дальше

1. Пальцевой хват: выделить ладонь/пальцы и реальные ручки всех четырёх грузов; непрерывный skin/rig, reach/clearance и крупные кадры обоих персонажей. Не импортировать R8 поверх R6B вслепую.
2. Physical storm: единое поле воды для визуала/сил, реакция на тела, подвижный корпус/инерция, наклон палубы и незакреплённый груз, based movement/boarding. Проверить сеть и dry hull в движущейся системе координат до замены статического R7.
3. Для active body: повреждения от повторных контактов, bracing/get-up качество, тесные места, длительный стенд и active-fall late join.

Техническая сверка: [Epic — SetSimulatePhysics](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/UPrimitiveComponent/SetSimulatePhysics), [physics-driven animation](https://dev.epicgames.com/documentation/unreal-engine/physics-driven-animation-in-unreal-engine?lang=en-US). Поведение kinematic teleport и camera verification дополнительно сверено с локальным исходником установленного UE 5.8.2.
