---
name: calmy-dream-loop
description: Iteratively improve Calmy Pirates Co ship visuals using reference images, Blender and Unreal runtime screenshots with independent visual review and traversal regression checks. Use for dream-loop work on this project, not generic game creation.
---
# Calmy visual iteration

Project: E:/Game/CalmyPiratesCoUE. Read its AGENTS.md and current Docs before acting. This is a modern coastal freighter first-person co-op game; retain its low-poly art direction and existing Unreal/Blender pipeline. Do not migrate to Three.js or replace gameplay with a presentation scene.

This project-specific adaptation is inspired by https://github.com/achimala/dream-loop (concept/build/critic loop). It is not an upstream installation or evidence of upstream Unreal support.

## Choose a bounded slice

Select one room or a small explicitly requested group of views. Preserve current assets, collision and routes. Establish baseline file hashes and a backup before changing assets. Read current import scripts: generated maps may discard component material overrides. Do not run one-shot scripts over a later revision.

Capture or verify a current runtime baseline. Use imagegen to improve that screenshot into a feasible target, preserving camera, layout and low-poly style; label it as a concept, never as gameplay. Confirm a substantial style/layout change with the user; ordinary improvements within an already approved direction need not block implementation.

## Build, see, critique

For each round, record concrete defects and change the highest-impact causes first: shape/layout, light/exposure, material readability, then details. Use Blender for geometry and Unreal APIs for engine assets, not binary editing. Do not introduce clutter or fake collision-free obstacles into circulation paths merely to match a concept.

Use actual Unreal screenshots at repeatable camera positions; review several nearby and walking views, not just one staged hero frame. Keep generated images and real screenshots in separate clearly labeled directories. If a camera-only image is used, do not count it as physical traversal.

Use one independent visual-critic subagent for a bounded read-only review when available. Supply target, candidate, nearby views, constraints and previous directives, not the desired score. While the critic works, perform useful local regression/evidence work. Ask for concrete defects, LANDED/PARTIAL/NOT DONE for prior directives, and separate layout, light, material and detail judgments. Never interpret a critic score as human approval or percent completion of the ship.

Preserve required physical clearances, player camera, cargo/NPC routes and server authority. Changes to gameplay/collision require relevant automation and Host/Client evidence. For a material/light-only pass on the existing map, repeat the bounded runtime walking route and check other areas for exposure regressions. Report actual measured FPS only when captured; a 60 fps cap is not a performance result.

## Stop and hand off

Agree or state a bounded first pass (normally one or two rounds on a selected slice); do not silently launch an unbounded token loop. If repeated tweaks do not solve a defect, diagnose the structural cause before another round. Preserve failed trials as diagnostics, not PASS evidence. Never disable reflections/physics/actor ticks or teleport the pawn to conceal a defect.

Record source hashes, changes, real screenshots, test outcomes and remaining defects in a dated report. Keep frozen evidence immutable and update current project docs with the new report link. Distinguish UE post-import overrides from Blender master changes. Offer the actual game for user review, with solo pause when requested. A completed iteration is not a finished ship.
