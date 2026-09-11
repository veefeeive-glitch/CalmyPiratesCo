> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# WaterLab — 2026-09-05

Latest continuation: [Deckhand R2 / NPC / water](../../CREW_HELPER_R2_HANDOVER.md), 2026-09-06.
H provides solo assistance for heavy cargo; buoyancy now uses local bounds and
wave-relative vertical damping. The large ship and moving-platform gate remain separate.

## Current scope

User explicitly postponed the ship. Default map is now `/Game/Maps/WaterLab`:
a fixed 32 x 20 m platform, dry delivery zone, four cargo types and a 12 m
horizontal-run, 6 m wide ramp into calm water. No ship actor, cabin, hull,
compartment leak or interior-cutaway actor is created here. Old maps remain
available; no Blender master, exported R6 asset or Unity source was modified.

## Implemented

- `ACalmyOcean`: synchronized analytic wave field, displaced procedural surface,
  Single Layer Water material, four-point rigid-body lift and drag for free cargo.
  The physical surface has **no collision**. Floating is AddForceAtLocation, not
  an animation or a transform reset. A plain falling rigid body is the negative
  control in the network test.
- Server owns cargo state and outcomes. Clients apply matching forces between
  replicated body corrections for presentation. No new RPCs, remote services,
  firewall rules or tool installations were needed.
- CharacterMovement custom surface-swim mode: swept collision movement,
  horizontal acceleration/drag, vertical surface following, and shallow-water
  transition back to walking. Entering water releases held cargo. Swimming
  prevents grabs. This is surface swimming, not diving or water-volume swimming.
- Space jumps on dry ground. Camera pitch now affects throws (bounded +/-70 deg).
  Existing reservations, drop, handoff, impact, knockdown and recovery remain.
- WaterLab rescue is ocean-relative: outside 250 m radius or 18 m below mean sea
  level, instead of immediate rescue at the old tiny ship boundary. Dry spawn
  validation allows elevated platforms. Legacy maps retain their previous bounds.
- Four differentiated cargo objects and dry delivery contract remain available.
  Leak/repair/power/director and interior cutaway are preserved only in old labs:
  they require ship compartments, so they are not spawned on an open platform.

## Run and controls

```powershell
.\Scripts\start_game.ps1
.\Scripts\start_game.ps1 -Mode Host -Port 17777
.\Scripts\start_game.ps1 -Mode Client -Port 17777
```

WASD + mouse: first-person walking/swimming; Space: jump on land; E: grab;
G: drop; LMB: aimed throw; Q: handoff. Return from water along the orange-railed
ramp. Deliver crates into the green dry zone and let them settle. R restarts
a finished/failed contract for the host. Bulky cargo needs a free-handed helper,
so a solo player cannot complete the four-cargo cooperative contract.

The game launcher disables editor Python startup (same as the network probes),
avoiding the engine ToolsetRegistry/PythonTestRunner startup error in `-game`.
Editor authoring scripts and Blender/Unreal bridge configuration are unchanged.

## Verification and discovered defects

- Editor Development build PASS: `Saved/WaterLab-build-final.log`.
- Automation PASS 16/16: `Saved/Logs/Automation-20260905-211406.log`.
- Water three-role physical route PASS: `Saved/Verification/20260905-211417-Water`.
  Host, owning client, late join walk the ramp, swim and return using movement
  input (no pawn teleport); floating cargo stays near the moving surface and
  rescue count stays zero. The server also rejects an in-range grab while
  swimming on all three roles. Render-only camera poses are not route evidence.
- Cargo regression PASS: `Saved/Verification/20260905-210631-Cargo`.
  Initial route stalled behind the physically dropped exhausted crate. The
  probe now walks around that crate and the helper; no collision was disabled.
- Carry regression PASS: `Saved/Verification/20260905-204714-Carry`.
- Leak regression PASS: `Saved/Verification/20260905-204725-Leak`.
- Shift regression PASS: `Saved/Verification/20260905-211734-Shift` (deliveries,
  rescue, payouts, authorized restart and replicated next round).
- Legacy interior regression PASS: `Saved/Verification/20260905-211723-Interior`
  (both decks, stairs, rooms and local cutaway). This does not import R6.
- First Water route failed: Python positional Rotator rotated the ramp in roll,
  not pitch. Corrected with explicit `pitch=...`, saved through Unreal, then
  retested. `Scripts/fix_waterlab_ramp.py` also adds actual handrail supports.
- Three real GPU-rendered views inspected at
  `Saved/Verification/WaterLab-Visual/View1.png` through `View3.png`.
  The initial finite water edge was visible; the remote mesh boundary was
  subsequently extended towards the horizon without densifying the whole grid.
  A second review caught specular streaks on stretched triangles: wave normals
  and displacement now fade to a flat sea outside the playable 250 m radius.
  Physical waves inside the playable area are unaffected by this distant LOD.
  All three final captures were inspected after the correction; log:
  `Saved/WaterLab-visual-reviewed.log`.

Re-run:

```powershell
.\Scripts\run_automation.ps1
.\Scripts\run_network_smoke.ps1 -Suite Water -Port 17801
.\Scripts\run_network_smoke.ps1 -Suite Cargo -Port 17802
```

`-CalmyWaterProbe=Visual` is an opt-in non-Shipping render-only fixture for a
standalone WaterLab game. It captures three viewpoints and exits. Normal games
do not create this probe. It is separate from Host/Client/LateJoin acceptance.

## Important limitations / next implementation boundary

This is a testable calm-water foundation, not finished ocean or ship physics.
No CFD, breaking waves, wakes, foam/splash effects, underwater post-processing,
swimming animation, drowning, storm spectrum or dynamic ship has been added.
Current four force samples and capped bounds are tuned for crates; a large
ship needs explicit hull pontoons, mass/inertia/centre-of-mass and sea trials.
Platform is fixed and is not evidence for movement on a moving networked ship.
Body replication remains the existing prototype scheme, not production rollback
physics. Internet/latency/loss stress, packaged build and human co-op fun gate
are not certified by the loopback tests. Near-field surface tessellation is 4 m;
its interpolated visual height approximates the analytic physical sampler.

Inherited 8 m grab range is retained for compatibility with the existing labs;
short reach/aim targeting is a future tuning task. Floating lost crates can drift
until rescue bounds or contract restart; no salvage mechanic is implemented.

The ocean surface is built in BeginPlay; the editor viewport before Play is not
a live ocean preview. `create_water_lab.py` refuses to overwrite the existing map.
Use narrowly scoped Unreal scripts for future edits, not binary modifications.
