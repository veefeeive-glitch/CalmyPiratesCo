# Каталог моделей и референсов

Актуально на 30 октября 2026: runtime CrewR8, Cargo R3 и Fly R2 в HOLD 1. R6B сохранён как предыдущая версия. Датированные записи ниже описывают историю переходов; R8 уже импортирован 11 сентября. Новых моделей эта чистка не создаёт.

**Hero G1 (12 сентября):** [sockets/fingers scaffold](../Changes/HERO_G1_SOCKETS_FINGERS_2026-09-12.md). На существующих `SM_Cargo_*` — `Grip_Near/Far_L/R`; на CrewR8 `SM_*_Forearm*` — `Wrist`/`Palm`/`Tip`. Новых мешей нет; continuous skinned body / articulated fingers в Content отсутствуют (список нужд в отчёте).

**Interaction R5:** [правая рука при ударе и плавный разворот Миши](../Changes/INTERACTION_R5_2026-09-10.md) реализованы runtime моторной IK. Новых Blender/Static Mesh/материалов нет: Crew R6B и Fly R2 прежние. Это не исправление всех стыков старых моделей и не новая skinned боевая анимация.

**Размещение обновлено R3/R4:** тот же R2 mesh/materials используется одним роем 32 мух в HOLD 1. Мухи удалены из WaterLab и перенесены с рубки. [Память/поведение и проверки](../Changes/FLY_MIND_R4_2026-09-10.md); новая Blender-модель не создавалась. Нижняя запись об обоих saved роях — состояние R2.

**Fly R2 (10 сентября):** [источник и runtime](../Changes/FLY_IN_GAME_R2_2026-09-10.md). `SourceAssets/Generated/FlyR2/Fly_R2.blend` и `SM_Fly_R2_Corrected.glb` → `/Game/Creatures/FlyR2/SM_Fly_R2_Corrected`; 1840 треугольников, четыре материала, анимация крыльев по vertex mask. Уже импортирован и назначен обоим saved роям. Исходный `SM_Fly_R2.glb` — промежуточный экспорт до исправления COLOR_0.

Снимок 2026-09-09. Пути относительно корня проекта. Основной [HANDOVER](../../HANDOVER.md).

## Герои

| Ревизия | Где лежит | Статус | Deckhand / Миша, triangles |
|---|---|---|---|
| R8 | [CrewR8](../../SourceAssets/Generated/CrewR8) / Crew_R8.blend | **В игре** `/Game/Characters/CrewR8` ([runtime](../Changes/CREW_R8_RUNTIME_2026-09-11.md)) | 6012 / 6048 |
| R7 | [CrewR7](../../SourceAssets/Generated/CrewR7) / Crew_R7.blend | Предыдущий Blender-кандидат; не в игре | 5462 / 5442 |
| R6B | [CrewR6B](../../SourceAssets/Generated/CrewR6B) / Crew_R6.blend | Предыдущий runtime; Content сохранён | 4802 / 4882 |
| R6 | [CrewR6](../../SourceAssets/Generated/CrewR6) | Первый диагностический проход R6 | 4726 / 4811 |
| R5 | [CrewNightwatchR5](../../SourceAssets/Generated/CrewNightwatchR5) | История Nightwatch; не текущий runtime | 3474 / 3592 |
| R4/R3 | [Generated](../../SourceAssets/Generated) / CrewLowPolyR4, CrewCargoR3, Deckhand | Старые модели/грузы, не удалять по номеру | См. соответствующие манифесты |

Отдельный runtime `/Game/Characters/CrewR8` импортирован 2026-09-11; `/Game/Characters/CrewR6B` сохранён. `/Game/Characters/CrewR7` отсутствует. [R8 runtime](../Changes/CREW_R8_RUNTIME_2026-09-11.md), [R8 authoring](../Changes/CREW_R8_2026-09-08.md).

### R8: что переносить в другую программу

- [Master](../../SourceAssets/Generated/CrewR8/Crew_R8.blend), [общий рендер](../../SourceAssets/Generated/CrewR8/R8_Lineup.png), [лица](../../SourceAssets/Generated/CrewR8/R8_Faces.png), [сзади](../../SourceAssets/Generated/CrewR8/R8_Back.png), [Миша 3/4](../../SourceAssets/Generated/CrewR8/R8_Misha_ThreeQuarter.png).
- Вся папка: один blend, 20 GLB, 20 `T_SM_*.png`, 4 preview PNG, mesh_manifest.json. 20 GLB — **не двадцать персонажей**, а по десять частей на каждого. В `.blend` обе фигуры собраны для просмотра.
- 9 сентября Blender read-only подтвердил: 20 текстур FILE, не packed, все пути относительные `//T_*.png`. Их нельзя отрывать от `.blend`. GLB по отчёту содержит embedded texture; для стороннего инструмента проверить после импорта.
- Текстурный base color включает AO и зерно; bake теней фиксирован, умножение было в byte-encoding, не scene-linear. При переносе в UE возможна слишком тёмная ткань; возможно потребуется отдельный AO channel. Metallic не нужен для одежды.
- Генератор [create_crew_r8.py](../../Scripts/create_crew_r8.py), заготовка импорта [import_crew_r8.py](../../Scripts/import_crew_r8.py). Не запускать поверх существующего кандидата/мастера. Новые внешние модели не прогонять через процедурный генератор: он не умеет сохранять чужой скульпт.

### Общий rig-контракт нынешних моделей

Имена `SM_Deckhand_*` / `SM_Misha_*`: Torso, Head, UpperArmL/R, ForearmL/R, ThighL/R, ShinL/R. Это Static Mesh части + PhysicsConstraint, **не skeletal mesh со skin weights**. Локальные кисти — отдельные owner-only экземпляры.

Пивоты в сантиметрах: Torso (0,0,-6), Head (0,0,51), UpperArm (0,±27,44), Forearm (0,±29,17), Thigh (0,±12,-6), Shin (0,±12,-44). Blender geometry хранится в метрах, GLB импортируется в UE сантиметры. Forward +X, up +Z, бок Y. Не применять ещё один коэффициент 100 после импорта вслепую.

Тесты: <6500 triangles на героя; деталь <100 см по max extent, center <70 см; head Min.Z ≤ -4,9 см и width Y <31 см; forearm width Y <14 см; коллизии и материалы обязательны. Новый скелет может потребовать осознанной миграции presentation/physics/IK и тестов, а не одной замены пути.

### AI-ориентиры, НЕ модели

- [Миша](../../SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png), [prompt](../../SourceAssets/References/CrewR6/prompt.txt).
- [Deckhand](../../SourceAssets/References/CrewR6/DECKHAND-CONCEPT-not-gameplay.png), [prompt](../../SourceAssets/References/CrewR6/DECKHAND-prompt.txt).
- [Реальные модели R6B, рендер Blender](../../SourceAssets/Generated/CrewR6B/R6_Lineup.png) не путать с более красивой AI-картинкой.

## Корабль и остальные ассеты

Корабль: [master](../../SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend), [walk-инструкция](../../SourceAssets/Generated/OceanFreighter/WALKTHROUGH_RU.md). Игровой экспорт — ShipR7Game, стекло отдельно — ShipR7RenderB, дверной derivative — ShipDoorsR1. UE: Content/Ships/OceanFreighterR7, карта OceanFreighterR7. 122 × 20,4 м; уровни/дефекты/исторические проверки — [SHIP](../Current/SHIP.md).

**Не переимпортировать старый корабль без переноса** MetalR1B component overrides, DoorsR1 mesh и runtime swing directions, Engine Dream/Nightwatch свет, dry hull, BR starts и меню draft. Master Blender не содержит все последующие UE изменения.

Грузы: собственные геометрические R3 в [CrewCargoR3](../../SourceAssets/Generated/CrewCargoR3), плюс существующие Content-зависимости. FreighterKit/FreighterInterior/FullFreighter — прежние наборы, не последние master судна. NightwatchAudioR1 — свои wind/hull loop исходники. [ASSETS](../Current/ASSETS.md) и старый provenance содержат подробности.

Внешние источники: SourceAssets/External/ambientCG, SourceAssets/ThirdParty/KenneyFactoryKit_3.0 и PolyHaven. Переносить с лицензиями/манифестами. Этот аудит не обновлял лицензии в интернете и не разрешает перепубликовывать любой найденный ассет.
