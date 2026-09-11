> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Ocean freighter design basis

> Current Blender arrangement: see `OCEAN_FREIGHTER_R5_HANDOVER.md` and its exact-file
> geometry/visual/isolated UE physics evidence. The verification history below describes earlier iterations;
> the old assertion that all model P1/P2 findings were closed is superseded by the
> close-up audits and must not be treated as current production acceptance.

`Calmy Ocean Freighter` is an unfinished game-oriented general-arrangement study, not a game-ready asset or class-approved construction drawing.

## Principal dimensions

- Length overall: 122.0 m
- Moulded beam: 20.4 m
- Hull depth to weather deck: 10.5 m
- Approximate design draft reference: 7.2 m
- Internal levels: tank top, lower deck, tween deck, weather deck
- Aft superstructure: boat deck, accommodation deck, officer deck, bridge deck
- Play-space target: 2–12 players, with redundant vertical routes planned for later passes

The scale is based on the proportions of real 120 m general-cargo vessels, widened slightly for readable first-person co-op circulation. The arrangement follows the real-world separation of cargo, machinery, accommodation, service and control spaces, but it is intentionally not represented as a certified naval architecture design.

## General arrangement

- Aft: steering gear, shaft tunnel, two-level engine room, workshop, electrical room and auxiliary machinery.
- Midships: two large cargo holds with tween-deck perimeter routes, hatch coamings and two deck cranes.
- Forward: bosun store, chain locker, bow-thruster room, forepeak and mooring deck.
- Aft superstructure: galley, mess, cold/dry stores, crew cabins, officers' cabins, laundry, medical room, radio room, ship office and wheelhouse.
- Exterior: continuous side walkways, railings, mooring stations, anchors, lifeboats, vents, radar mast, navigation lights and exhaust stack.

## Source layout

- Blender master: `SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend`
- Rebuild script: `Scripts/create_ocean_freighter.py`
- Geometry audit: `Scripts/audit_ocean_freighter.py`
- Build manifest: `SourceAssets/Generated/OceanFreighter/build_manifest.json`
- Preview: `Saved/Verification/OceanFreighter-Blender-Overview.png`
- Structural cutaway: `Saved/Verification/OceanFreighter-Blender-Cutaway.png`
- Stern review: `Saved/Verification/OceanFreighter-Blender-Stern.png`
- Stair/access review: `Saved/Verification/OceanFreighter-Blender-Access.png`
- Forecastle review: `Saved/Verification/OceanFreighter-Blender-Bow.png`
- Cargo-crane close-up: `Saved/Verification/OceanFreighter-Blender-Crane.png`
- Furnished-cabin review: `Saved/Verification/OceanFreighter-Blender-Cabin.png`
- Engine-room review: `Saved/Verification/OceanFreighter-Blender-EngineRoom.png`
- Hold-access review: `Saved/Verification/OceanFreighter-Blender-HoldAccess.png`
- Engine-access opening review: `Saved/Verification/OceanFreighter-Blender-EngineAccess.png`
- Pre-correction audit: `Saved/Audits/OceanFreighter-ProductionAudit-20260904-PreCorrection.md`
- Post-correction audit: `Saved/Audits/OceanFreighter-ProductionAudit-20260904-PostCorrection.md`
- Walking controls: `SourceAssets/Generated/OceanFreighter/WALKTHROUGH_RU.md`
- One-command GUI/MCP launcher: `Scripts/Open-OceanFreighter.ps1`

The Blender scene remains the authority until the large-vessel blockout and traversal review is accepted. Unreal import is deliberately deferred so the existing playable map is not replaced with an unreviewed hull.

## Verification

- Blender 5.2.1 generation log: `Saved/Logs/OceanFreighter-Blender-20260904.log`
- Deterministic marker: `CALMY_OCEAN_FREIGHTER_PASS`
- Current user-approved 20% production baseline: 3,090 mesh objects, 93,492 vertices and 78,636 polygons. Object count is recorded for regression checks, not used as a completion percentage.
- First-person C++ build: passed with Unreal Engine 5.8.2.
- Automation after the final camera-relative/world-relative movement split: `Saved/Logs/Automation-20260904-183532.log` — 15/15 passed.
- Editor first-person multiplayer regression: `Saved/Verification/20260904-183533-Interior` — Host, Client and LateJoin passed with `NETWORK_SMOKE_PASS`.
- Windows package: `Builds/Windows-20260904-183719/CalmyPiratesCo.exe`, promoted to `Builds/Windows-Current`.
- Packaged first-person multiplayer regression: `Saved/Verification/20260904-183848-Interior` — Host, Client and LateJoin passed with `NETWORK_SMOKE_PASS`.
- First geometry-polish pass: water removed; main deck plates, transverse decks, bulkheads and railings follow the interpolated hull envelope. Propeller, rudder, bulbous bow, deck seams, ventilators and fire stations added.
- Second geometry-polish pass: hatch stiffeners, external accommodation stairs, bridge wings, engine-room catwalks, colour-coded service mains, valves, windlasses and fairleads added.
- Geometry audit: `Saved/Logs/OceanFreighter-Audit-20260904.log` — `water=0`, 12 hull-following main deck plates and 0.044 m minimum retained envelope clearance.
- Historical correction pass: placeholder rotor replaced by a swept five-blade propeller, spinner, stern skeg and faired rudder; temporary bow-thruster side circles removed; enclosed lifeboats and working davits added; twin four-flight access towers added; accommodation corridor walls rebuilt with real door openings and additional room furniture.
- Historical build log: `Saved/Logs/OceanFreighter-25Percent-20260904.log` — retained for traceability; its old percentage estimate is superseded by the current user assessment.
- Progress is now fixed at the user's 20% production baseline, regardless of mesh count.
- General-audit correction pass: lower and tween decks were opened into full-depth holds; main and emergency engine-room routes, protected hold ladders and real deck openings were added. Frames now contain parked-open door leaves and wheel handles. Machinery gained bilge/fire pumps, compressors, air bottles, heat exchangers, purifiers, manifolds, controls and twin-ram steering gear. Accommodation gained sanitary fixtures and 23 inspection lights. Hidden export collections contain 24 `UCX_` collision modules and four LOD/HLOD proxies.
- Pre-correction audit: 2 P0, 6 P1 and 1 P2 finding open. Post-correction audit: all model-level P1 and P2 findings closed; Unreal integration remains explicitly partial until import and in-engine collision validation.
