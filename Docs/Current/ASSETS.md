# Ассеты и происхождение — 2026-09-06

**11 сентября — [Windows share P1](../Changes/WINDOWS_SHARE_P1_2026-09-11.md).** Для standalone cook явно включены динамически загружаемые CrewR6B/CargoR3/BridgeMenu; сами assets не изменялись. В ZIP сохранены DesktopFly MIT/FlyWire CC BY-NC 4.0 notices и credits других наборов. Добавлен [локальный Poly Haven license record](../../SourceAssets/ThirdParty/PolyHaven/LICENSE-PROVENANCE-2026-09-11.md) по официальной странице CC0; прежняя заметка ниже о недостающем record теперь историческая.

**10 сентября — [Water Interaction W1](../Changes/WATER_INTERACTION_W1_2026-09-10.md):** новый собственный Unreal material `Content/Materials/M_WaterInteraction_W1.uasset`, VertexColor + процедурные мягкие края, без внешних текстур/моделей. Создание: `Scripts/create_water_interaction_material.py`, настройка: `Scripts/tune_water_interaction_material.py`. Назначается C++ default `CalmyOcean.InteractionMaterial`; старый FoamR3 и карты не переписаны. Геометрия локальных волн/пены/капель формируется runtime-кодом. R6B/R8 и ship master не изменялись.

**Актуализация 2026-09-09:** [каталог моделей](../Handover/MODELS.md) разделяет runtime **R6B** и последние Blender-кандидаты **R7/R8**. R8: 6012/6048 triangles, 20 GLB и 20 внешних PNG с относительными путями в blend; в Unreal не импортирован. Оба AI-концепта и prompts находятся в SourceAssets/References/CrewR6. Для новых внешних моделей предусмотрена SourceAssets/Incoming/Characters. Лицензии старых наборов заново не проверялись, ассеты не перемещались.

Актуальный реестр на основе локальных файлов и прежней provenance-записи. [Статус](STATUS.md). В проходе R3 ниже интернет использован для референсов; новые внешние модели не скачивались и не покупались. Лицензии старой библиотеки целиком заново не аудировались.

## Актуальные персонажи — Crew R8

В runtime — **Crew R8**: `ACalmyCrewPresentation` собирает 10 частей Deckhand/Миши из `/Game/Characters/CrewR8` ([импорт](../Changes/CREW_R8_IMPORT_2026-09-11.md), [runtime](../Changes/CREW_R8_RUNTIME_2026-09-11.md)); проверка ассетов — automation `CrewLowPolyTests`. Ниже — предыдущая версия R6B, сохранена.

[Crew R6, 8 сентября](../Changes/CREW_R6_2026-09-08.md): собственные Deckhand 4802 / Миша 4882 triangles, 20 GLB, master `SourceAssets/Generated/CrewR6B/Crew_R6.blend`, импорт `/Game/Characters/CrewR6B`. Два прохода геометрии, новые материалы рабочей одежды/каски/перчаток, сохранён физический контракт 10 частей. R5 и первый кандидат R6 сохранены. Внешних ассетов/анимаций нет. `SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png` — imagegen-референс, не runtime. Ограничения: жёсткие стыки, однородная ткань, недостаточная читаемость в сумраке.

## Предыдущие персонажи — Crew Nightwatch R5

[Nightwatch R1, 8 сентября](../Changes/NIGHTWATCH_R1_2026-09-08.md): оригинальные Deckhand (3474 треугольника) и Миша (3592), 20 GLB и отдельный [Blender master](../../SourceAssets/Generated/CrewNightwatchR5/Crew_Nightwatch_R5.blend). Игрок — капюшон/бафф, Миша — каска/защитные наушники, уменьшенные головы и тихие глаза, рабочая одежда со складками. Импорт `/Game/Characters/CrewNightwatchR5`; R4 сохранён как история. Нет новых внешних моделей или animation clips.

`Content/Atmosphere/NightwatchR1`: два собственных синтезированных 32-секундных mono WAV loops (ветер, гул корпуса) и два производных материала моря. [Imagegen-концепт](../../SourceAssets/References/Nightwatch-R1-20260908/CONCEPT-not-gameplay.png) — только референс, НЕ игровой ассет/скриншот. Происхождение, промпт, инструменты и границы проверки — в отчёте.

## Предыдущие персонажи — Crew Low-Poly R4

[Doors R1, 2026-09-08](../Changes/SHIP_DOORS_R1_2026-09-08.md): производный GLB superstructure без 61 полотна и 27 ручек; 61 игровой actor на основе engine Cube и существующих материалов. Файлы импорта в Content/Ships/OceanFreighterR7/DoorsR1; внешних загрузок нет, Blender master прежний.

[Engine Dream Loop R1](../Changes/ENGINE_DREAM_LOOP_R1_2026-09-08.md), 8 сентября: пять новых собственных Unreal material assets на старых текстурах и один imagegen-эталон вне Content. Новые внешние ассеты не загружались; эталон не импортирован в игру.

Корабельные материалы Unreal дополнены [Metal R1](../Changes/SHIP_METAL_R1_2026-09-06.md): 42 собственных material assets с уже имеющимися текстурами ambientCG; новых скачиваний/покупок нет. Только overrides игровой карты, не новый Blender master.

- [Отчёт](../Changes/CREW_LOWPOLY_R4_2026-09-06.md), [Blender master](../../SourceAssets/Generated/CrewLowPolyR4/Crew_LowPoly_R4.blend), [lineup](../../SourceAssets/Generated/CrewLowPolyR4/R4_Lineup.png).
- Оригинальная геометрия проекта: 20 GLB, по 10 частей для Deckhand (2804 треугольника) и Миши (2788). Гранёная одежда/лица, матовые материалы; разные куртки и головные уборы, более тонкие руки.
- Авторинг `Scripts/create_crew_lowpoly_r4.py`, импорт `Scripts/import_crew_lowpoly_r4.py`; Unreal `Content/Characters/CrewLowPolyR4`. Новых скачанных моделей, сторонних текстур/анимаций и покупок нет. Грузовые модели R3 сохранены и продолжают использоваться.
- Старые персонажи R3 ниже — сохранённая предыдущая версия, не текущий runtime mesh.

## Crew / Cargo R3 — 2026-09-06 (история персонажей, актуальные грузы)

- Оригинальный [Blender-master](../../SourceAssets/Generated/CrewCargoR3/CrewCargo_R3.blend), [lineup](../../SourceAssets/Generated/CrewCargoR3/R3_Lineup.png): десять модульных частей матроса и четыре модели груза. Это **не** ревизия корабля OceanFreighter R3/R7.
- Авторинг: [create_crew_cargo_r3.py](../../Scripts/create_crew_cargo_r3.py), Blender 5.2.1. Импорт: [import_crew_cargo_r3.py](../../Scripts/import_crew_cargo_r3.py).
- Четырнадцать GLB в `SourceAssets/Generated/CrewCargoR3`; Unreal: `Content/Characters/DeckhandR3` и `Content/Cargo/R3`. Материал реакции воды: `Content/Materials/M_WaterLab_FoamR3.uasset`.
- Новые модели: рабочая куртка, шапка, перчатки/ботинки; деревянный ящик, бочка жидкости в раме, приборный кейс, тяжёлый агрегат. Геометрия и плоские PBR-материалы созданы в проекте. Старые Deckhand и грузовые материалы сохранены.
- Референсы: [PEAK, официальная страница](https://store.steampowered.com/app/3527290/PEAK/) — общая читаемая комедийная стилизация; [Poly Haven plastic crate](https://polyhaven.com/a/plastic_crate_01) и [Pelican BX255](https://business.pelican.com/us/en/product/cases/Cargo/BX255) — транспортная тара; [Epic Water System](https://dev.epicgames.com/documentation/en-us/unreal-engine/water-system-in-unreal-engine) — ориентир функций воды.
- Эти страницы не являются источником импортированных моделей/текстур. PEAK-персонажи, брендинг и продуктовые meshes не копировались; плагин Epic Water не подключался в этом проходе. Новые инструменты/подписки не устанавливались: существующих Blender CLI и Unreal Python хватило.
- Результат и ограничения: [R3 change report](../Changes/CREW_CARGO_WATER_R3_2026-09-06.md). Четыре формы — не библиотека десятков вариантов и не финальная текстурная/анимационная работа.

## Последние собственные изменения

- R6: медпункт, радио, носовое служебное оснащение и связи маршрутов.
- R7-Cargo-C06: собственные изменения проходов, холлов, площадок, мебели и указателей; свежая копия импортирована в отдельную OceanFreighterR7, WaterLab сохранена. Текущий hash master указан в [паспорте](SHIP.md), экспорт/импорт — в [отчёте интеграции](../Changes/SHIP_R7_INTEGRATION_2026-09-06.md). Новые сторонние ассеты для этого импорта не загружались.
- Deckhand R2: изменения C++ физики/процедурной анимации/камеры и NPC; нового skeletal asset или скачанных animation clips нет. В Content/Characters/Deckhand проверено наличие 45 файлов, их hash-реестр сохранён в R2 acceptance.
- Старые FBX/GLB не объявляются свежим R7-экспортом. Ни число объектов, ни число файлов не являются процентом готовности.

Оригинальные наборы ниже сохранены для восстановления и учёта; «imported subset» относится к ранее импортированным лабораториям, не к R7 в WaterLab.

## OceanFreighter R3, 2026-09-05

Original in-project geometry refit, authored by `Scripts/ocean_freighter_refit.py`
through Blender 5.2.1. Reuses the ambientCG maps already recorded below; no additional
third-party models, paid assets, subscriptions or marketplace acquisitions in this pass.
R3 — исторический этап; текущий статус R7 находится в `Docs/Current/SHIP.md`.

## Generated in-project

- `SourceAssets/Generated/FreighterKit/CalmyFreighterKit.blend` and twelve exported GLBs.
- Authoring script: `Scripts/create_freighter_kit.py`.
- Purpose: original modular coastal-freighter hull, deck, bridge, rail, bollard, winch, vent, lifebuoy, hatch, cargo container, crane and exhaust stack.
- Tool: Blender 5.2.1 LTS.
- `SourceAssets/Generated/FreighterInterior/CalmyFreighterInterior.blend` and 21 exported GLBs.
- Authoring script: `Scripts/create_freighter_interior_kit.py`.
- Purpose: original modular bridge, engine-room, crew-space and service-corridor kit: floors, walls, ceiling, door/frame, stairs, ladder, helm, consoles, navigation table, machinery, pipes, workbench, lockers, bunks, galley, rack, ducting, lights and junction box.
- Assembly scripts: `Scripts/build_freighter_interior.py`, `Scripts/polish_freighter_interior.py`, `Scripts/add_freighter_cutaway.py`.
- `SourceAssets/Generated/FullFreighter/CalmyFullFreighter.blend` and 12 exported GLBs.
- Authoring script: `Scripts/create_full_freighter_kit.py`.
- Purpose: original opening weather deck, lower-deck floor, main engine, fuel tank, pump skid, electrical cabinets, mess furniture, washroom unit, air handler, cargo pallets, bilge pump and propeller shaft.
- Assembly scripts: `Scripts/build_full_freighter.py`, `Scripts/fix_full_freighter_layout.py`, `Scripts/polish_full_freighter.py`.
- `SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend` and `build_manifest.json`.
- Authoring script: `Scripts/create_ocean_freighter.py`.
- Purpose: original 122 m game general-arrangement master with faired hull stations, four internal deck levels, four-tier aft superstructure, two cargo holds, machinery plant, accommodation, navigation, cranes, mooring and safety fittings.
- Status: standalone R7-Cargo-C06 Blender authority; last recorded user assessment 22%, no accepted 30%. Not imported into WaterLab; production collision, authoring quality and full first-person validation remain incomplete. Not a class-approved ship drawing.

## Kenney Factory Kit 3.0

- Source: https://kenney.nl/assets/factory-kit
- Downloaded: 2026-09-03.
- License: Creative Commons CC0.
- Local source: `SourceAssets/ThirdParty/KenneyFactoryKit_3.0`.
- Imported subset: crane, fortified machine, large pipe valve, wide screen and orange warning prop.

## Poly Haven

- Asset: Metal Plate — https://polyhaven.com/a/metal_plate
- Asset: Barrel 03 — https://polyhaven.com/a/barrel_03
- Downloaded: 2026-09-03 at 1K working resolution.
- License: CC0 — https://polyhaven.com/license
- Local source: `SourceAssets/ThirdParty/PolyHaven`.
- Imported subset: deck base-colour/DirectX-normal/ARM maps and the textured barrel model.

**2026-09-15 — ship props for the playability rework** (разрешение пользователя на загрузку получено в сессии):
- Assets (glTF 1K): [Lifebuoy](https://polyhaven.com/a/lifebuoy), [Korean Fire Extinguisher 01](https://polyhaven.com/a/korean_fire_extinguisher_01), [Fire Alarm](https://polyhaven.com/a/fire_alarm), [Industrial Caged Sconce](https://polyhaven.com/a/industrial_caged_sconce), [Modular Industrial Pipes 01](https://polyhaven.com/a/modular_industrial_pipes_01), [Power Box 01](https://polyhaven.com/a/power_box_01), [Barrel 01](https://polyhaven.com/a/Barrel_01), [Wet Floor Sign 01](https://polyhaven.com/a/WetFloorSign_01).
- License: CC0; authors, byte sizes and SHA-256 of all 50 files: `SourceAssets/ThirdParty/PolyHaven/LICENSE-PROVENANCE-2026-09-15.md`, `MANIFEST-2026-09-15.json`.
- Import: `Scripts/import_polyhaven_ship_props.py` → `/Game/ThirdParty/PolyHaven/<asset>`; wall/ceiling fittings without collision, floor props with a simple box.

## ambientCG

- Assets: [Diamond Plate 009](https://ambientcg.com/view?id=DiamondPlate009), [Metal Walkway 014](https://ambientcg.com/view?id=MetalWalkway014), [Metal 049 A](https://ambientcg.com/view?id=Metal049A), [Wood Floor 051](https://ambientcg.com/view?id=WoodFloor051).
- Downloaded: 2026-09-04 at 1K JPG working resolution, including color, roughness, normal, metalness/AO where supplied and the original packaged source files.
- License: Creative Commons CC0; commercial use and redistribution are allowed by the source.
- Local source: `SourceAssets/External/ambientCG`; untouched archives are retained in `_downloads`.
- Intended use: shared PBR deck, stair/grating, clean machinery metal and accommodation wood materials in the Blender master. These do not replace the need for authored ship geometry.

No Fab/Megascans marketplace content is included yet because acquiring it requires the user's Epic account and accepting asset-specific marketplace terms.

## R4/R5 machinery and outfit authoring — 2026-09-05

R5 adds an original authored workshop bench, fire post and portable cable reel
in `Scripts/ocean_freighter_outfit.py`, reusing existing materials. No external mesh,
new marketplace licence, paid service or downloaded asset was added for this pass.

- All new R4 machinery geometry is original procedural project work in
  `Scripts/ocean_freighter_machinery.py`; no downloaded mesh or brand logo.
- General arrangement references only: [Wärtsilä marine gensets](https://www.wartsila.com/marine/products/engines-and-generating-sets/generating-sets/wartsila-gensets)
  and [KSB pump lexicon, baseplate / back pull-out design](https://www.ksb.com/en-global/centrifugal-pump-lexicon/b).
  These informed diesel/alternator separation and pump/drive/baseplate layout,
  not certified dimensions, performance claims or copied product artwork.
- Existing local ambientCG maps reused. No new asset licence or purchase required.

## Deckhand character — 2026-09-05

- Original geometry and flat PBR materials authored by `Scripts/create_deckhand.py`.
- Source master: `SourceAssets/Generated/Deckhand/Deckhand_Master.blend`; ten GLB
  articulated parts imported to `/Game/Characters/Deckhand`.
- No downloaded model, commercial character, scanned person, brand logo, paid
  generation service or third-party animation clip was used. Bodycam is an
  aesthetic reference, not a copied game asset. Movement is project-authored
  procedural motor targets plus Chaos simulation.

## Локальные подтверждения и остатки реестра

Kenney: `SourceAssets/ThirdParty/KenneyFactoryKit_3.0/extracted/License.txt` прочитан, содержит CC0. ambientCG: `SourceAssets/External/ambientCG/ASSET_MANIFEST.json` содержит ID, даты, CC0-1.0 и хеши архивов. Poly Haven: локальные model/texture файлы присутствуют; отдельный локальный текст лицензии в этом обходе не найден — прежняя provenance-ссылка сохранена, требуется добавить снимок лицензии/метаданных перед распространением. Отсутствие найденного файла не доказывает отсутствие лицензии.

Перед новой внешней загрузкой записывать источник, автора, версию, лицензию, дату, локальный путь, назначение, импортированный поднабор и hash. Fab/Megascans не значится в подтверждённом provenance; приобретения и принятие новых условий не выполнялись. Ассет-библиотека не заменяет проверку масштаба, проходов и единого визуального качества.
