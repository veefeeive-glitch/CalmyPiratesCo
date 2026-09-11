> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Calmy Pirates Co. — Unreal Engine migration

This is the Unreal Engine 5.8.2 continuation of the original Unity prototype.

Current planning baseline (2026-09-05): [general audit](../../GENERAL_AUDIT_2026-09-05.md),
[game roadmap](../../GAME_ROADMAP.md), and [ship roadmap](../../SHIP_ROADMAP.md).
These distinguish current source, historical test evidence and planned work.
The large ship remains outside the runtime pending explicit approval.

Latest gameplay update (2026-09-06): [Deckhand R2 / NPC helper handover](../../CREW_HELPER_R2_HANDOVER.md).
**H** recruits/dismisses a physical helper for solo heavy-cargo carrying; team pace
responds to separation and movement mismatch. **M** adjusts bodycam motion;
**V** toggles the saved comfort setting. Ground-aware procedural leg targets,
physics substeps and wave-relative buoyancy damping are implemented. This remains
a prototype: no paired hand constraints or moving large-ship simulation yet.

Previous character baseline: [Deckhand / bodycam handover](../../DECKHAND_CHARACTER_HANDOVER.md).
Original modular dock worker with physical limbs and procedural movement. **B**
stumbles; **V** toggles steady camera. Platform-only WaterLab remains the default.

**Current runtime: WaterLab, platform only (2026-09-05).** The default game/editor
map and `Scripts/start_game.ps1` now open `/Game/Maps/WaterLab`, not the ship.
See [WaterLab handover](../../WATERLAB_HANDOVER.md) and
[mechanics audit](../../WATERLAB_MECHANICS_AUDIT.md). R7 Blender is not imported.
The historical freighter sections below describe the preserved
optional regression map, not the current starting scene.

The Unity repository at `E:\Game\CalmyPiratesCo` remains an unchanged migration source. This native Unreal prototype now contains replicated character/cargo gameplay, safe overboard return, a repeatable three-crate delivery contract, three differentiated cargo behaviors, a recoverable leak/repair drill, and a first art-dressed freighter deck. It is not the complete game; a voyage loop, production character/audio/UI work and human playtesting remain ahead.

The historical dated project handover is `HANDOVER_2026-09-05.md`. For the current state and continuation order, use the audit and roadmaps above together with the newer Deckhand, WaterLab and R6 handovers; older ship revisions and percentages are not current acceptance.

Latest ship authoring update (2026-09-06): [R7 cargo/layout handover](../../OCEAN_FREIGHTER_R7_HANDOVER.md).
Master is R7-Cargo-C06, with a preserved R6 backup. 52 cargo walks + four controls
pass, 260/260 required walking attempts pass; broad suite remains 293/336.
17 closeups reviewed, with piping/support and wider art work explicitly open.
No R7 Unreal import, FBX/GLB update, packaged build or human acceptance yet.

Previous ship authoring update: [OceanFreighter R6 handover](../../OCEAN_FREIGHTER_R6_HANDOVER.md),
[execution checklist](../../OCEAN_FREIGHTER_R6_CHECKLIST.md), and [35-view review](../../OCEAN_FREIGHTER_R6_REVIEW.md).
R6 fixes five connection intents and adds medical/radio/thruster fittings. Verified:
98/98 geometry routes, 260/260 required directed Unreal walks; broad exploratory
suite 293/336 with 43 retained failures. User visual rating and multiplayer map
integration were still outstanding at that revision; R7 supersedes its master.

Previous ship authoring update: [OceanFreighter R5 handover](../../OCEAN_FREIGHTER_R5_HANDOVER.md).
Later [continuous traversal audit](../../OCEAN_FREIGHTER_TRAVERSAL_AUDIT.md) found blocked
connections despite the green local routes: 240/316 results, including exploratory paths.
Do not interpret the base lab PASS as whole-ship connectivity or production acceptance.
R5 adds three equipment families and an opt-in Unreal physical walking lab: 176 directed
walks plus four calibration controls pass. It is not a production map replacement or
multiplayer traversal acceptance. Repeat with `Scripts/run_ocean_physics.ps1`.
Use `Scripts/Open-OceanFreighter.ps1`, then the
**Ship Review** sidebar for zone selection and captured review notes.

## Product identity

Calmy Pirates Co. is a 2–4 player physical co-op comedy on a stylised modern coastal freighter. The first proof loop is:

```text
move -> grab -> carry -> throw -> impact -> temporary knockdown -> recovery
```

## Play the vertical slice

Run `Scripts\start_game.ps1` for the platform and water. To explicitly play the
previous vertical slice, run `Scripts\start_game.ps1 -MapName FreighterDeck`.
FreighterDeck preserves the cargo/shift/emergency actors and collision from
LeakTest with a non-colliding art pass. The following legacy controls and layout
notes refer to that map. `/Game/Maps/DeckTest` remains the original regression map.

- First-person control: mouse looks around; WASD movement is relative to the camera. Deckhand replaces the old greybox body; its owner sees limbs while head/torso are hidden to avoid camera occlusion. Other players see the complete character.
- E: grab the nearest visible free crate within the inherited eight-metre interaction range.
- G: drop. Left mouse: throw along camera aim with bounded vertical pitch.
- Bring all three crates around the equipment block into the green delivery zone, then drop them. The whole crate must fit inside and settle for 0.75 seconds. Accepted cargo turns green and cannot be picked up again.
- Each contract has a server-owned five-minute clock. A successful shift pays a base `$1200` plus a speed bonus, minus rescue and damaged-cargo penalties, and awards a one-to-three-star rating. Expiry fails the contract and pays nothing; clients only display the replicated result.
- After 3/3 deliveries, the host (or solo player) presses R for a fresh shift. A remote client cannot restart the host's round.
- Falling through the open gangway returns the player to a free deck position. Held cargo returns to its original place; thrown/lost cargo also returns. The HUD counts individual rescued players/crates, not unique incidents.
- Fast thrown cargo temporarily knocks a player down, then recovers automatically (1.2 seconds down, 0.7 seconds recovering).

For manual multiplayer, set the editor Play options to **2 players / Play As Listen Server**, then Play. A four-player maximum is implemented; the automated probe uses host, owning client, and late joiner. Internet matchmaking is not implemented.

The character uses Enhanced Input and Unreal Character Movement prediction/correction. Cargo outcomes and lifecycle timers are server-owned. The old tilt-only presentation has been superseded by Deckhand's local articulated physical limbs; movement still uses the capsule, not a fully replicated active ragdoll. Cargo physics use standard movement replication, not a finished networked-physics solution.

## Freighter interior

The optional legacy FreighterDeck is a physically connected two-deck ship, not the current default and not OceanFreighter R6. The original 21-module bridge/interior kit is extended by 12 new Blender-authored ship-system modules and a replacement weather deck with real stairwell and gangway openings. The lower deck contains seven readable spaces: engine room, fuel/pump space, crew bunks and lockers, mess, washroom/electrical room, workshop/cargo hold, and bilge/propeller-shaft space. Ten physical stair steps connect the decks; 49 new simple collision volumes define the expanded layout independently from decorative meshes.

Six upper-interior roof panels are visible from the exterior and hide only for the local player who enters the ship. While that player is below deck, the weather deck and sea occluder also hide locally, so one split/network player cannot change another player's view. Legacy solid deck, hull, bridge and sea collision is disabled; server rescue bounds include the lower deck while still rejecting genuine overboard positions.

```powershell
& .\Scripts\run_network_smoke.ps1 -Suite Interior -Port 17790
```

The Interior probe uses real movement for host, owning client, and late joiner. It verifies the bridge, aft collision, physical stairs, lower-deck cutaway, engine room, crew deck and cargo hold.

## CargoLab: cooperative cargo behaviors

Open `/Game/Maps/CargoLab` for the cargo regression lab. WaterLab is the default;
H can recruit an NPC for heavy cargo when playing solo.

- **Bulky / yellow / wider crate:** requires another controlled, free-handed crew member (or the carrier's NPC) within 3 metres and clear line of sight. Coordinated carry speed is 143–260 cm/s instead of 450; separation and velocity mismatch reduce the replicated team pace. Cannot be thrown. Losing the helper for one second forces a drop. This is one authoritative holder plus proximity assistance, not two physically attached hands or a finished two-person animation.
- **Unstable / blue turning red:** loses 20 stability points per second while held. At zero it drops and cannot be picked up until at least 25. Resting free cargo recovers 25 points per second. **Q** transfers it to the nearest eligible teammate within 2.5 metres and restores 35 points; the server enforces a two-second cargo-wide handoff cooldown and command replay protection.
- **Fragile / turquoise turning dark red:** sufficiently hard server collisions damage integrity, with a short synthesized local impact cue and changed tint. Soft drops do not damage it. Damage survives an overboard reset. Damaged deliveries count once in the shift outcome; they are accepted, so a mistake does not make the contract unwinnable. A new shift restores condition.

The HUD shows the held or nearest eligible cargo's handling hint and current stability/integrity. Late join uses current condition/holder snapshots, not replayed historical sounds.

```powershell
& .\Scripts\start_game.ps1 -Mode Host -MapName CargoLab
& .\Scripts\start_game.ps1 -Mode Client -MapName CargoLab
& .\Scripts\run_network_smoke.ps1 -Suite Cargo -Port 17781
```

The Cargo probe uses actual host/client movement and gameplay RPCs, including a real thrown-crate collision against the equipment block. It has no fixture teleports or test-only RPCs. It verifies all three behaviors and one damaged delivery, not a human playtest or a full three-special-cargo shift.

## LeakTest: first shared emergency

Open `/Game/Maps/LeakTest` (or use `start_game.ps1 -MapName LeakTest`). This separate, solo-playable map uses standard crates. Existing maps are unchanged.

- The host selects a session seed and schedules the first leak warning after 18–24 seconds. A five-second warning precedes the leak. The valve near the rear/right rail changes from green to yellow to red; the lower HUD gives the next action and current schedule.
- Water rises by 2 percentage points per second, capped at 100. At 30%, emergency power dims and desaturates the scene while red valve and blue delivery task lights preserve the next actions. At 50%, all deck movement is 30% slower, including cargo carrying. This is a gameplay meter and a simple blue indicator, not simulated flooding.
- Hold **F** within 1.8 metres of the red valve, with empty hands, grounded and stationary. Clear line of sight is required. One player repairs at 20 points per second; another player is not mandatory.
- Releasing F, moving, carrying cargo, falling, knockdown or disconnect pauses repair and frees the valve. Partial work remains for a teammate. Lost heartbeats expire after 0.65 seconds of server time.
- At 100% repair the leak seals; water drains at 10 points per second. Normal speed returns below 50% and normal lighting below 30%. A 45–60 second cooldown begins after the deck is dry. At most two leaks may start in one contract, and an active warning/leak/drain can never stack with another event. Contract completion stops the director; R starts a fresh generation with cleared water, worker, repair, power and schedule state.

The server owns the director seed, event serial, schedule/cooldown, water, repair time, exclusive worker, phase and the derived power consequence. Clients submit only intent through their owned pawn; sequence/rate, round, world, range, visibility and player-state checks reject invalid requests. Late join receives one current snapshot and presents its lighting locally. `-CalmyEmergencySeed=<positive integer>` provides a reproducible developer run; ordinary sessions use a host-selected seed.

```powershell
& .\Scripts\start_game.ps1 -Mode Solo -MapName LeakTest
& .\Scripts\run_network_smoke.ps1 -Suite Leak -Port 17783
```

The Leak probe uses three real processes, normal movement and gameplay RPCs with deterministic seed 203. A repairing client disconnects, the late joiner receives the active director/power snapshot and resumes, the host delivers all three crates, then restarts the shift and all roles observe the new schedule. No network-probe teleports or direct gameplay-state assignments. This is technical evidence, not a three-human stress or fun test.

## Verification

```powershell
& 'E:\Games\UE_5.8\Engine\Build\BatchFiles\Build.bat' CalmyPiratesCoEditor Win64 Development 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -WaitMutex

& 'E:\Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'E:\Game\CalmyPiratesCoUE\CalmyPiratesCo.uproject' -unattended -nop4 -NullRHI -NoSound '-ExecCmds=Automation RunTests CalmyPiratesCo' '-TestExit=Automation Test Queue Empty' '-abslog=E:\Game\CalmyPiratesCoUE\Saved\Logs\GameplayTests.log'

& .\Scripts\run_network_smoke.ps1
```

Close Unreal normally before a C++ build; Live Coding can block an external build. The network probe launches separate hidden game processes bound to loopback, uses normal gameplay RPCs and movement, and keeps per-run logs under `Saved\Verification`. It checks duplicate rejection, contested/out-of-range grabs, replicated carrying, throw impact, recovery, late-join state, and disconnect release. Its test subsystem is disabled in Shipping builds.

Current verification and packaging helpers:

```powershell
& .\Scripts\run_automation.ps1
& .\Scripts\run_network_smoke.ps1 -Suite Carry
& .\Scripts\run_network_smoke.ps1 -Suite Shift -Port 17779
& .\Scripts\run_network_smoke.ps1 -Suite Cargo -Port 17781
& .\Scripts\run_network_smoke.ps1 -Suite Leak -Port 17783
& .\Scripts\run_network_smoke.ps1 -Suite Interior -Port 17790
& .\Scripts\package_windows.ps1
```

The automation wrapper checks individual project test results because an engine process exit of 0 alone does not prove passing tests. The Shift probe performs actual client movement/grab/throw/drop and server delivery, with two clearly logged server-position fixtures to force held-player and thrown-cargo falls. It also checks completed-contract late join, unauthorized/duplicate/stale restart and the new round's snapshot. Fixtures are opt-in and disabled in Shipping.

`package_windows.ps1` creates a new timestamped `Builds/Windows-*` archive; it does not overwrite earlier archives. After a successful package it also refreshes `Builds/Windows-Current`, the stable path intended for packaged launches and network probes. Windows Firewall rules are path-based, so using that stable copy avoids a new security prompt for every timestamped archive. Run `Scripts/install_dev_firewall_rule.ps1` once and approve its one UAC prompt to allow only the stable game executable on Private networks; it does not enable Public-network access. On this workstation the rule is installed and verified. MCP, AllToolsets, and the project's Python/editor-scripting references are restricted to the Editor target. The engine can still stage a Python plugin descriptor through built-in dependencies; this is not an active Python/MCP control endpoint. GameFeatures is explicitly enabled as a normal gameplay-runtime dependency so its shared Asset Manager rule also resolves in packaged games. Cooking disables its own MCP auto-start to avoid competing with the open editor's port.

Latest verified Development package is `Builds/Windows-20260904-183719/CalmyPiratesCo.exe`, promoted to `Builds/Windows-Current`. FreighterDeck is cooked and launches as the default map. Fifteen automation tests pass, including the first-person camera contract. Editor and packaged Host/Client/LateJoin Interior probes verify traversal of both decks, per-player cutaway visibility and collision; the Shift regression verifies the replicated contract, rescue and economy flow. Earlier Leak probes continue to cover the emergency director. This is a local vertical-slice build, not a Shipping release or a human playtest.

To open visible standalone windows locally:

```powershell
& .\Scripts\start_game.ps1 -Mode Solo
# Or two separate windows on this PC:
& .\Scripts\start_game.ps1 -Mode Host
& .\Scripts\start_game.ps1 -Mode Client
```

Add `-PackagedExe 'E:\Game\CalmyPiratesCoUE\Builds\Windows-Current\CalmyPiratesCo\Binaries\Win64\CalmyPiratesCo.exe'` to use the stable packaged build. `run_network_smoke.ps1` accepts the same argument; use the inner executable, not the bootstrap launcher. UE 5.8 archives this project directly at the archive root (no extra Windows directory). Network tests still bind to loopback; the optional one-time firewall helper affects only the stable executable and Private networks. These are local development launchers, not Internet matchmaking.

`Scripts\create_deck_test.py` builds the initial assets through Unreal APIs and refuses to overwrite an existing map. Do not edit `.umap` or `.uasset` files as text.

`Scripts\create_shift_test.py` uses the same base generator for a separate map, adds the delivery zone, equipment chokepoint and open gangway, and leaves the original DeckTest asset untouched.

`Scripts\create_cargo_lab.py` duplicates ShiftTest through Unreal APIs into a new CargoLab asset and assigns the three cargo kinds. Existing maps are never overwritten.

`Scripts\create_leak_test.py` creates a separate LeakTest asset and repair valve through Unreal APIs. It refuses to overwrite an existing map.

`Scripts\create_freighter_kit.py`, `create_freighter_deck.py`, and `polish_freighter_deck.py` provide a repeatable Blender-to-Unreal art pipeline for the default map. The generated kit contains twelve ship modules; the map adds 84 art actors while retaining the tested gameplay layer.

`Scripts\create_freighter_interior_kit.py`, `build_freighter_interior.py`, `polish_freighter_interior.py`, and `add_freighter_cutaway.py` extend that pipeline with 21 editable interior modules, explicit collision and the local cutaway roof system.

`Scripts\create_full_freighter_kit.py`, `build_full_freighter.py`, `fix_full_freighter_layout.py`, and `polish_full_freighter.py` add the connected lower deck, replacement opening deck, 12 editable system/room modules, 49 collision volumes, industrial zone treatment and below-deck local cutaway.

`Scripts\create_ocean_freighter.py` creates the separate 122 m `Calmy Ocean Freighter` Blender master. The current model uses the user's 20% production baseline after a full model audit. It now contains four internal deck levels, full-depth cargo holds, redundant machinery-space and hold access, open door leaves, lit accommodation, sanitary rooms, propulsion auxiliaries, bridge, cranes, deck fittings, 24 collision proxies and four LOD/HLOD proxies. It opens on a human-eye walkthrough camera; use `Shift` + `` ` `` and `W/A/S/D` in the 3D viewport. See `MEGA_FREIGHTER_DESIGN.md`. Blender remains authoritative until modular Unreal import and first-person collision validation are complete.

See `VERIFICATION.md` for the exact tested build and logs. Still unvalidated: two-human play value, four-player stress/soak, packet loss/high latency, gamepad input, production art/animation, and the rest of the Unity content.

The project-local `.codex\config.toml` connects Codex to Unreal MCP on port `8000` and Blender MCP on port `9876`. Keep the corresponding desktop application open while using its tools.
