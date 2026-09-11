# Nightwatch R1 — персонажи R5 и тревожная морская атмосфера

8 сентября 2026. Запрос: переделать главные модели и сделать игру более жуткой. Это ограниченный первый художественный проход по двум персонажам и двум актуальным картам, не приёмка всей игры или процент готовности корабля.

## Что изменено

- **Deckhand R5:** уменьшенная голова, узкие тёмные глаза без белых квадратов, капюшон и бафф, серо-зелёная рабочая куртка, перчатки/ботинки, радио и bodycam.
- **Миша R5:** отдельное лицо, приглушённая охристая каска и защитные наушники, ржаво-коричневая рабочая куртка. Не появляется из воздуха: существующий постоянный NPC сохранён.
- Новые исходники: `SourceAssets/Generated/CrewNightwatchR5/Crew_Nightwatch_R5.blend`, 20 GLB, manifest, два Blender-render. Deckhand **3474**, Миша **3592** треугольника. Десять частей на человека, прежние pivot/rig и капсула 34×88 см; новые NDOP26 физические формы для модулей. Старый R4 не перезаписан.
- `CalmyCrewPresentation.cpp` переключён на `/Game/Characters/CrewNightwatchR5`; существующий asset-тест использует новые пути и дополнительный предел ширины головы. Массы, суставы, RPC, IK, bodycam и правила движения не переписаны. Новых skeletal animation clips нет.
- **OceanFreighterR7 и WaterLab:** холодный сумрак, морской туман, десатурация, умеренная виньетка/зерно, приглушённые рабочие лампы и холодное заполнение. На корабле дополнительно освещены три внутренних сервисных галереи. Добавлены источники света, не новая геометрия светильников.
- Два собственных материала моря в `/Game/Atmosphere/NightwatchR1`: дубликаты действующих графов, изменён только Tint. Маска сухого корпуса и волновая реакция сохранены. Это не новая симуляция воды.
- Оригинальные синтезированные **Wind/Hull**: mono PCM16, 22050 Гц, 32 с, тихие loops (ветер на двух картах, гул корпуса на корабле). Исходники WAV и параметры в `SourceAssets/Generated/NightwatchAudioR1`. Без сторонних записей, покупок, монстров, jumpscares или новой системы угроз. Субъективное прослушивание не выполнено: технические тесты запускаются с NoSound.

## Цикл и выявленные дефекты

Использованы навыки **calmy-dream-loop** (повторяемые UE-кадры, независимый read-only критик, физические регрессии) и **imagegen** (только целевой концепт). Blender CLI и Unreal Python уже были доступны; новые программы/MCP не устанавливались, RTX/Firewall не переключались. Прикладные правила проекта сохранены; более тревожная подача не означает миграцию из современной co-op игры в иной жанр.

1. Модели, проход 1: убраны игрушечные глаза и одинаковые головы. Критик указал на высокие плечи и жёсткую одежду.
2. Модели, проход 2: снижены/скошены плечи, добавлены сгибы/складки, убрана нарисованная поверх баффа линия рта и висящие ремни каски. Round1 сохранён отдельно, а не удалён.
3. Свет, проход 1: реальный маршрут выявил почти чёрные внутренние переходы/персонажей и горячие локальные пятна. Критик подтвердил, что читаемость ещё не достигнута.
4. Свет, проход 2: поднято холодное заполнение, уменьшены пики ламп, добавлен свет галерей. GPU выявил **VSM local-light overflow** после расширения радиусов; радиусы сокращены (`fix_nightwatch_light_overlap.py`), предупреждения/тени не отключались и лимит VSM не повышался.
5. Техническая коррекция: увеличение Source Angle давало неестественно большой солнечный диск в WaterLab. Восстановлен угол 0,5357°, Source Soft Angle отделён (5°). Это исправление побочного дефекта, не новая бесконечная арт-итерация.

Первое сохранение WaterLab было заблокировано одновременно запущенным read-only тестом (Windows Error 32). Ship успел сохраниться, WaterLab оставалась исходной. После завершения теста выполнен защищённый WaterLab-only resume. `Apply.log` сохранён как **failed diagnostic**, не как PASS; `ApplyResume.log` подтверждает восстановление. Старый маркер LP4_IMPORT в импортёре и LP4/R3 названия runner относятся к совместимости скриптов, реальные пути/хеши — R5.

## Проверки и доказательства

- Editor Development build **Succeeded**, DLL SHA256 `4af0c8e437de2fd89903365797fa55e165f5fb9aa1600a401e31e4efbc9a7b46`.
- **27/27 automation**: `Saved/Logs/Automation-20260908-115600.log` (после R5 и второго светового прохода). Последующие коррекции меняли только карты/свет, не DLL или тестируемые модели.
- **Character**, Host/Client/LateJoin: новые физические конечности, ходьба, падение/восстановление, bodycam/comfort. `20260908-113854-Character`.
- **Helper**, три роли: помощь/тяжёлая доставка, повторный запрос, увольнение/повторный найм, persistent NPC после disconnect. `20260908-115216-Helper`; отдельно `Helper-Solo-20260908-115505.log` PASS.
- **Door**, три роли: все 61 закрываются и открываются, owning-client E, duplicate/stale, поздний snapshot. `20260908-115313-Door`. Это не физический проход через все 61 порог.
- **Ship**, три роли: `20260908-115226-Ship`, Host/Client по 37 точек, 90,861/102,641 м; max joint 6,329/6,330 см при лимите 8; без сбросов/телепортов. LateJoin — состав карты/сухой grounded spawn, не третий обход.
- **Water**, три роли: вход, плавание, выход по рампе, груз, отказ grab при плавании — `20260908-115429-Water`.
- Реальные GPU серии отделены от концепта. В каждой корабельной серии — 37-точечный физический solo маршрут плюс три camera-only вида; в серии персонажей — девять постановочных проверочных кадров, **не** физический обход.
- Финальные карта/Source/Content/исходники/логи/кадры и последняя повторная проверка после технических коррекций перечислены в `Evidence/Gameplay/Nightwatch-R1-20260908/acceptance.json` и `manifest.json`. Прежние frozen evidence не перезаписаны.

Финальный повтор после обеих технических коррекций: **Ship Host/Client/LateJoin PASS**, `20260908-120758-Ship`; Host/Client 91,056/102,706 м, max joint 6,329/6,328 см, без resets/teleports. GPU: `ShipR7-Visual-20260908-120747` (37 точек, 90,778 м, max joint 6,328 см), `LP4-Visual-20260908-120849` (9 кадров, R3_REVIEW PASS). Все 16 финальных кадров просмотрены, предупреждения VSM overflow отсутствуют в финальном логе и просмотренных изображениях. В Source относительно резервной копии изменены только presentation loader и asset-тест.

Финальные SHA256 карт: OceanFreighterR7 `89c6b702af47ff1c7d316d56c90704683e9c020e977c7d08bf866d97a16ac665`; WaterLab `0122f4aca4ec281214979814e024fd3fb5a430856e284eed778d1a9f911ca180`.

## Границы и следующие дефекты

Финальный независимый критик подтвердил восстановление читаемости route06/12/18 и машинной, сохранение ясных грузов/хвата. Оставил частичными персонажей в тени, пики ламп и длинную тень на море. Его вывод сохранён в `Audit-And-Diagnostics/Independent-Critic.md` внутри frozen evidence, не подменяет оценку пользователя.

Мрачная палитра/головные уборы/спокойные глаза получились; плечи в runtime всё ещё выступают треугольными краями, складки местами напоминают сегментированную броню. Нужен следующий скульптурный проход плеч/локтей с проверкой в движении, не ещё одна перекраска. Руки в неосвещённой воде остаются слишком тёмными, знак помощи и часть лица Миши требуют отдельной проверки читаемости. Некоторые светильники/потолочные пятна по-прежнему яркие; низкое солнце даёт длинную тень на воде. Вся атмосфера ещё не имеет финальной ручной приёмки. Новые лампы не заменяют полную ведомость физических светильников.

Не добавлены: новые враги/поведение угроз, сюжетные события, финальная лицевая/скелетная анимация, пространственный аудиомикс/окклюзия/шаги, подвижное судно, новые грузы. Сохранены 61 дверь, геометрия судна, текущие материалы MetalR1/EngineDream и мокрая/сухая логика. Ship master SHA256 остаётся `8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93`.

Это не human fun gate, не полный R7/NPC-маршрут, не новый 15-минутный soak R5, не измерение 60 FPS и не packaged build. `Builds/Windows-Current` по-прежнему старый; смотреть актуальный Editor/game запуск.

## Концепт: происхождение и точный prompt

Built-in image generation, не CLI fallback. Сохранён в [CONCEPT-not-gameplay.png](../../SourceAssets/References/Nightwatch-R1-20260908/CONCEPT-not-gameplay.png), **не импортирован** в Content. Вход 1 — старый LP4 lineup, вход 2 — настоящий прежний кадр машинной. Генерация не является доказательством игрового качества.

> Use case: stylized-concept / lighting-weather. Create a feasible visual target sheet for an existing Unreal low-poly modern freighter cooperative game, NOT actual gameplay. Two panels: character redesign and the engine-room lighting target. Image1 is character proportion/style baseline to substantially improve: two original adult merchant crew members, smaller realistically proportioned heads, no square white toy eyes, no smiles, rugged quiet faces partly obscured by a hood or a hard hat, layered weathered work clothes, restrained grey-green waxed hooded jacket on player, muted ochre safety helmet and charcoal/rust workwear on Misha, slim work gloves, practical boots. Low polygon sculpted planes, ~4000 triangles per figure, NO photoreal scans, no monsters, no weapons, no logos. Anatomically credible but still stylized. Image2 is environment EDIT TARGET: preserve EXACT camera/layout/geometry/objects and floor dry roughness, change only light and mood. Cold maritime dusk fill, pools of dim warm practical work-light, steel/slate shadows, slight atmosphere, tense and eerie but stairs/doors/walking floor stay clearly legible. NO new pipes/clutter/signs, no reflective mirror floor, no blacked-out image. Label small 'CONCEPT — NOT GAMEPLAY'.
