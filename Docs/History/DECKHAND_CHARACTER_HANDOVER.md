> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Deckhand / bodycam character — 2026-09-05

Superseded in part by [Deckhand R2 / NPC helper](../../CREW_HELPER_R2_HANDOVER.md), 2026-09-06:
ground-aware leg targets, distance-driven gait/camera, saved M/V settings and strict
8 cm joint test with substepping. The results below are historical, not the R2 acceptance.

## What is implemented

An original playable stylised dock worker replaces the hidden grey cylinder:
yellow flotation vest, radio, chest camera, helmet, moustache, gloves, knee pads
and oversized rubber boots. Approximately 1.8 m tall. Ten separately pivoted
Blender-authored parts, nine physical joints; no downloaded character or paid
service. Source: `SourceAssets/Generated/Deckhand/Deckhand_Master.blend`.
Reproducible generator: `Scripts/create_deckhand.py` (isolated background Blender;
does not load or overwrite the ship). Original authoring preview:
`SourceAssets/Generated/Deckhand/Deckhand_Review.png`.

`UCalmyCrewPresentation` creates nine simulated Chaos parts around a kinematic
torso. Angular motors drive a procedural idle, loose walking arm swing, bent
carrying arms, swimming arm motion, collapse and recovery. Soft drive during
knockdown lets limbs react physically to gravity, inertia and the static deck.
There are no authored skeletal animation clips or imported skeleton in this pass.

This is a **hybrid articulated puppet**, not an unrestricted whole-body ragdoll:
CharacterMovement's capsule still controls gameplay collision and network position.
The torso is guided; limbs cannot push cargo or other players. Their physical
poses are local presentation, not authoritative replicated bone transforms.
The replicated lifecycle and server movement determine the gameplay outcome.
Server cargo impacts now apply bounded knockback. Hard landings above 900 cm/s
can trigger knockdown; this threshold still needs human gameplay tuning.

## Camera and controls

- Default chest-height bodycam: FOV 102, spring-filtered acceleration/strafe/step
  motion, landing response, bounded fall tilt and vignette/chromatic fringe.
- Mouse aim remains responsive; camera shake does not supply gameplay impulses.
  No motion blur. Sphere sweep keeps the local camera away from static walls.
- Owner sees arms/trousers/boots. Its own head and vest/camera housing are hidden
  to avoid occlusion; other players see the complete model and the full shadow.
- **V** toggles a steady FOV-92 camera without dynamic shake, chromatic fringe or
  roll. This is session-local, not a saved accessibility preference yet.
- **B** deliberately stumbles, releases held cargo, falls and gets back up.
  Server validates ownership, command sequence/rate, grounded controlled state,
  absence of swimming and a five-second cooldown. Replays cannot add a stumble.
- WASD/mouse, Space, E, G, LMB, Q and contract restart R retain existing bindings.

## Integration and import

Runtime is still platform-only `/Game/Maps/WaterLab`. The ship is not imported or
edited. Character assets live under `/Game/Characters/Deckhand`, explicitly added
to always-cook directories because presentation loads them by stable path.

Run `Scripts/import_deckhand.py` in the **full editor** with `-ExecutePythonScript`,
not the Python commandlet. The commandlet has no StaticMeshEditorSubsystem and
the deprecated library silently cannot create collisions there. The importer
asserts subsystem and collision creation, generates 26-DOP simple collision,
disables unnecessary Nanite on these small moving meshes, and saves via Unreal.
It exits its own editor at completion. Run it in a dedicated authoring process,
not an editor with unsaved user work. Existing mesh geometry is preserved.

Runtime bodies may initialize asynchronously. Joint binding explicitly waits for
all ten valid body instances and verifies all nine constraints. Initial failure
captures showed limbs falling away before that gate was added; this is fixed,
not classified as intended slapstick. Rig resets its presentation after a rescue
teleport rather than dragging constraints across the world.

## Evidence

- Editor Development compile: `Saved/Deckhand-build9.log`.
- Camera bounds/settling automation at 30/60/120 Hz and hitch-sized deltas:
  PASS 17/17, `Saved/Logs/Automation-20260905-220902.log`.
- Carry/real cargo hit/recovery/late join/disconnect regression PASS:
  `Saved/Verification/20260905-215902-Carry`.
- Water walk/swim/ramp exit/buoyancy/in-water grab rejection PASS on three roles:
  `Saved/Verification/20260905-220447-Water`.
- Dedicated character scenario PASS on Host, Client and LateJoin:
  `Saved/Verification/20260905-220850-Character`. It exercises walking, physical
  joint separation, replicated stumble/recovery, duplicate rejection, camera
  comfort toggle and late-join counters/physical presentation.
  Maximum observed joint separation: host 1.82 cm, client 1.53 cm. The earlier
  client test ran into the host's capsule; its path now uses a separate lane,
  preserving player collision instead of teleporting through the obstruction.
- Five GPU-rendered review frames in `Saved/Verification/Deckhand-Visual`:
  standing, walk, physical fall, recovery and owner bodycam look-down.
  These are visual evidence, separate from the three-role networking test.
  Final five views inspected: `Saved/Deckhand-visual7.log`, max joint gap 3.20 cm
  during the render run. The owner view no longer looks through the vest housing.

```powershell
.\Scripts\start_game.ps1
.\Scripts\run_automation.ps1
.\Scripts\run_network_smoke.ps1 -Suite Character -Port 17805
.\Scripts\run_network_smoke.ps1 -Suite Carry -Port 17806
.\Scripts\run_network_smoke.ps1 -Suite Water -Port 17801
```

## Remaining quality gates

Human judgement of humour/comfort is not replaced by automated checks. This is
not photoreal Bodycam rendering, not a shipped networked active-ragdoll system,
and not a finished hero with retopology, skeletal skinning, foot IK, authored
get-up clips, sounds, facial animation or selectable outfits. Physical joint
poses may differ among peers. WAN latency/loss, dedicated-server packaging,
performance budgets and character interaction on a moving ship remain untested.
The earlier approximate swim/cargo physics limitations remain in force.
