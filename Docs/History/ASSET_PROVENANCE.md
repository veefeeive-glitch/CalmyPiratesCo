> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Asset provenance

## OceanFreighter R3, 2026-09-05

Original in-project geometry refit, authored by `Scripts/ocean_freighter_refit.py`
through Blender 5.2.1. Reuses the ambientCG maps already recorded below; no additional
third-party models, paid assets, subscriptions or marketplace acquisitions in this pass.
See `OCEAN_FREIGHTER_R3_HANDOVER.md` for current status, scope and outstanding gates.

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
- Status: standalone Blender authority at the user-approved 20% production baseline after the general model audit; intentionally not imported into Unreal pending modular collision and first-person validation. It is not a class-approved ship drawing.

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

## ambientCG

- Assets: [Diamond Plate 009](https://ambientcg.com/view?id=DiamondPlate009), [Metal Walkway 014](https://ambientcg.com/view?id=MetalWalkway014), [Metal 049 A](https://ambientcg.com/view?id=Metal049A), [Wood Floor 051](https://ambientcg.com/view?id=WoodFloor051).
- Downloaded: 2026-09-04 at 1K JPG working resolution, including color, roughness, normal, metalness/AO where supplied and the original packaged source files.
- License: Creative Commons CC0; commercial use and redistribution are allowed by the source.
- Local source: `SourceAssets/External/ambientCG`; untouched archives are retained in `_downloads`.
- Intended use: shared PBR deck, stair/grating, clean machinery metal and accommodation wood materials in the Blender master. These do not replace the need for authored ship geometry.

No Fab/Megascans marketplace content is included yet because acquiring it requires the user's Epic account and accepting asset-specific marketplace terms.

## R4 machinery authoring — 2026-09-05

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
