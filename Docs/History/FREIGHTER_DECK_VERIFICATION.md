> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# FreighterDeck vertical-slice verification

Verified on 2026-09-04 with Unreal Engine 5.8.2 and Blender 5.2.1.

## Delivered

- `/Game/Maps/FreighterDeck` is the game, server, and editor default map and is explicitly cooked.
- The original exterior pass retains LeakTest's gameplay/collision layer and adds 84 `ART_` actors.
- Twelve original freighter modules are reproducibly authored in Blender and imported through Unreal Interchange.
- A second original Blender kit adds 21 modular interior meshes; its assembly pass adds 64 `INT_` actors including 19 explicit collision volumes, followed by floor-zone polish and a six-panel local cutaway roof.
- A third original Blender kit adds 12 ship-system/deck meshes and 117 `FS_` actors, including 49 collision volumes, a connected lower deck, ten physical stair steps and seven functional rooms.
- Five Kenney Factory Kit props and two Poly Haven assets are recorded in `ASSET_PROVENANCE.md`.
- The stable packaged executable is `Builds/Windows-Current/CalmyPiratesCo.exe` (657.3 MiB).

## Evidence

- Art import: `Saved/Logs/FreighterDeckImport-20260903-4.log` — `CALMY_FREIGHTER_DECK_PASS`.
- Lighting/material polish: `Saved/Logs/FreighterDeckPolish-20260904-3.log` — `CALMY_FREIGHTER_POLISH_PASS`.
- Editor capture: `Saved/Verification/FreighterDeck-Oblique-Final.png`.
- Automation: `Saved/Logs/Automation-20260904-000918.log` — 12/12 project tests passed.
- Editor network probe: `Saved/Verification/20260904-000940-Leak` — Host, Client, and LateJoin passed.
- Package: `Saved/Logs/Package-20260904-001136.log` — BuildCookRun succeeded.
- Packaged network probe: `Saved/Verification/20260904-001310-Leak` — Host, Client, and LateJoin passed.
- The packaged FreighterDeck build was also launched and visually inspected with its HUD, cargo, delivery zone, and leak objective active.
- Interior verification, including editor and packaged three-process traversal, is recorded in `FREIGHTER_INTERIOR_VERIFICATION.md`.
- Latest package: `Saved/Logs/Package-20260904-174455.log`; packaged two-deck network traversal: `Saved/Verification/20260904-174843-Interior` — Host, Client, and LateJoin passed.

This is technical vertical-slice evidence. It is not a substitute for human multiplayer, art-direction, performance, or fun validation.
