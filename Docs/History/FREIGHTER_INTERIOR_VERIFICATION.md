> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Freighter interior verification

Verified on 2026-09-04 with Unreal Engine 5.8.2 and Blender 5.2.1.

## Delivered

- Editable Blender source plus 21 original modular GLBs under `SourceAssets/Generated/FreighterInterior`.
- A second editable Blender source plus 12 original ship-system and deck GLBs under `SourceAssets/Generated/FullFreighter`.
- Physically connected bridge/weather deck and lower deck on `/Game/Maps/FreighterDeck`, with real stairwell openings and ten traversable stair steps.
- Seven lower-deck spaces: engine room, fuel/pump space, crew deck, mess, washroom/electrical room, workshop/cargo hold, and bilge/propeller-shaft space.
- Forty-nine new simple collision volumes; decorative meshes and superseded solid blockouts are non-colliding.
- Per-player cutaway driven by `ACalmyInteriorCutaway`: roof panels hide inside, and weather-deck/sea occluders hide only while that local player is below deck.
- Ship-bound policy includes the lower deck without suppressing genuine player/cargo overboard recovery.

## Evidence

- Blender generation: `Saved/Logs/FreighterInterior-20260904.log` — 21 meshes imported and 64 actors assembled.
- Polish: `Saved/Logs/FreighterInteriorPolish-20260904.log` — machinery stack moved outside, four floor zones and tuned lighting.
- Cutaway: `Saved/Logs/FreighterCutaway-20260904.log` — six local-visibility roof panels.
- Door/collision correction: `Saved/Logs/FreighterDoors-20260904-4.log` — decorative collision disabled and 800 cm entrance/bridge openings retained.
- Editor capture: `Saved/Verification/FreighterInterior-Final.png`.
- Automation: `Saved/Logs/Automation-20260904-010241.log` — 14/14 project tests passed.
- Editor network traversal: `Saved/Verification/20260904-010141-Interior` — Host, Client and LateJoin passed.
- Shift regression: `Saved/Verification/20260904-010303-Shift` — Host, Client and LateJoin passed.
- Package: `Builds/Windows-20260904-010434/CalmyPiratesCo.exe` — BuildCookRun succeeded and stable copy refreshed.
- Packaged network traversal: `Saved/Verification/20260904-010604-Interior` — Host, Client and LateJoin passed with `NETWORK_SMOKE_PASS`.
- Full-freighter Blender generation: `SourceAssets/Generated/FullFreighter/CalmyFullFreighter.blend` — 12 deterministic GLBs exported.
- Full-freighter assembly: `Saved/Logs/FullFreighter-20260904.log` — 12 meshes, 117 actors, 49 collision volumes, two decks and seven rooms.
- Clearance correction: `Saved/Logs/FullFreighterClearance-20260904-4.log` — legacy hull/sea/art collision removed and stair landing cleared.
- Lower-deck polish: `Saved/Logs/FullFreighterPolish-20260904.log` — three functional zones, route treatment and industrial lighting.
- Lower-deck capture: `Saved/Verification/FullFreighter-LowerDeck.png`.
- Current automation: `Saved/Logs/Automation-20260904-174120.log` — 14/14 project tests passed.
- Current editor network traversal: `Saved/Verification/20260904-174313-Interior` — Host, Client and LateJoin traversed both decks with `NETWORK_SMOKE_PASS`.
- Current Shift regression: `Saved/Verification/20260904-174143-Shift` — Host, Client and LateJoin passed.
- Current package: `Builds/Windows-20260904-174455/CalmyPiratesCo.exe` — BuildCookRun succeeded and stable copy refreshed.
- Current packaged network traversal: `Saved/Verification/20260904-174843-Interior` — Host, Client and LateJoin traversed both decks with `NETWORK_SMOKE_PASS`.

This is evidence for a complete traversable two-deck ship layout, not a claim of final production art. Animated doors/machinery, production PBR materials/decals/audio, character art, ship movement/buoyancy, human multiplayer quality and performance certification remain unfinished.
