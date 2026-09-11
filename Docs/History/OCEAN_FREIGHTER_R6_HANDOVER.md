> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# OceanFreighter R6 handover — 2026-09-05

## Current truth

Authoritative Blender master: `SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend`.
SHA256: `9ceafe214b04051c293734a81c31ab81e7daf109cfa73f82a8525a38ab759fdd`.
It is the byte-identical reviewed R6 candidate, not a re-saved variant.
User's last rating was 22%; 30% has NOT been claimed or accepted.
Water remains absent. Unreal gameplay assets/maps were not replaced this pass.

## Implemented after the R5 traversal audit

1. Both Boat exits connect through supported openings in the old hull rail to
   the outside stair towers. Upper-level and exposed tower guards are preserved.
2. Ground-level tower end guards now permit forward/aft weather-deck movement,
   retaining the outboard guard segments. Primary forward route uses Y=±8.5 m;
   the 7.9 m diagnostic approach remains unsuitable beside the door/vent.
3. Aft side galleries are widened with bracketed plating and replacement edge
   rails, joining a transverse stern walkway at X=-55.8 m. Mooring bollards,
   fairlead rollers and chain stoppers were repositioned; the winch foundation
   was shortened. This avoids the aft wall, mooring fittings and central uptake.
4. Lower/tween engine galleries have 1.40 m-wide supported aft crosswalks with
   void-side guards. The exhaust trunk rises to Z=9.95 m over the tween route,
   instead of intersecting a player's torso. Preferred gallery Y=±4.7 m.
5. Bow-thruster placeholder box replaced with cylindrical ribbed motor, end
   covers, bolts, terminal box and support cradle. Axial package shortened to
   keep the service apron at X=51.4 m clear before the X=52 bulkhead doorway.
6. Medical room: distinct cot with castors and rails, treatment trolley, aid
   case and wall cabinet. Cabinet was moved off a window during close-up QA.
7. Radio room: dedicated worktop, receivers/displays, tuning knobs, keys,
   microphone and logbook block. New models are original authored geometry;
   no new downloads, paid assets, plugins, programs or credentials were needed.
8. Ship Review panel: six R6 review locations added (26 total).

## Evidence and limits

- `Saved/Verification/OceanFreighter-R6-Reviewed/geometry-routes.json`: 98/98
  sampled geometry routes, zero deck-fitting room intrusions, four pump assembly
  checks passing. R6 was added to the assembly-check revision filter; an empty
  list is explicitly rejected by the publication gate.
- `Saved/OceanPhysics/R6-traversal3.json`: actual Unreal CharacterMovement +
  Chaos, all visible evaluated meshes/curves, no teleporting during routes.
  260/260 required directed walks pass (base, new, usable-room and continuous
  connections). Four calibration controls pass, including a blocking wall.
- Broad audit: 293/336 results pass. 43 failed exploratory walks are retained:
  32 room-perimeter attempts through furnishing bands, four old bow-centreline
  journeys through the thruster, four old stern journeys through uptake/fittings,
  three 7.9 m exit-lane attempts. These are not 43 distinct architectural defects.
  Working alternatives are tested, but arbitrary free roaming is NOT certified.
- `remaining-contacts.json` maps every failure to the exact named mesh and
  position; `route-input-snapshot.json` freezes the 166 route definitions.
- `acceptance.json` holds the publication gate result and exact input hash.
- `shots.json`: 35 hash-bound renders, all visually inspected. Detailed notes:
  `OCEAN_FREIGHTER_R6_REVIEW.md`. Render lights are inspection-only, no geometry
  cutaway or hidden ship parts used. Images are not gameplay graphics evidence.
- Earlier attempts remain: R6-traversal1 (271/324), R6-traversal2 (285/336),
  preview1 and Final render folders. Do not confuse those with Reviewed.
- No C++/network behavior changed. This isolated physics lab does not substitute
  for production OceanFreighter map integration, moving-ship or multiplayer tests.

## Build and preservation

- R5 frozen backup: `SourceAssets/Generated/OceanFreighter/Backup-20260905-R5`.
  Unsaved open R5 GUI session was additionally preserved as
  `OpenSessionBeforeR6-194014.blend` before loading R6.
- `Scripts/ocean_freighter_circulation.py` is the R6 authoring module, invoked by
  the full generator after R3/R4/R5. Full generator defaults to R6-Candidate.
- Fast iteration: launch Blender background with the frozen R5 backup and
  `Scripts/build_ocean_r6_from_r5.py`. It verifies R5 SHA, loads only primitive
  function definitions via AST, reuses existing materials, then applies R6.
  Do NOT apply the module to an already modified R6 scene.
- Full first build was stopped only in our headless worker because unfiltered
  Boolean operations over every rail were unnecessarily slow. The module now
  filters overlapping objects first. GUI Blender was not terminated.
- Current candidate was built by the fast, hash-pinned path. Full from-scratch
  generator with final R6 module has not been rerun end-to-end.
- `Scripts/verify_ocean_r6_acceptance.py` is read-only. `publish_ocean_r6.py`
  verifies exact visual approval, physical and geometry evidence, R5 backup and
  unchanged master; copies exact candidate bytes and writes the manifest.
  Do not reuse its pinned visual approval for a later changed candidate.

## User review / next work

R6 is opened in Blender at the lower external exit. Hover the viewport and use
Shift + grave-accent, then WASD. Enter keeps the view; Esc cancels the movement.
N → Ship Review selects areas and saves a note + viewport image with coordinates.
This is Blender walking, not an interactive Unreal collision session.

Next priorities after user feedback: believable vent/winch/crane details, less
repetitive cabins, finished sanitary and galley fixtures, radio wiring and chart
detail, local lighting, missing service clutter/room-specific equipment. Preserve
accepted routes when adding props. Review actual close-up images, not mesh counts.
Then build the production UE collision/asset route, test full first-person free
roaming and multiplayer on this model. Do not call the whole ship complete.
