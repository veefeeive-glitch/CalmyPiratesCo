> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# WaterLab mechanics audit and implementation plan — 2026-09-05

2026-09-06 addendum: [Deckhand R2](../../CREW_HELPER_R2_HANDOVER.md) implements NPC-heavy-carry
assistance and team pace, camera preferences, stricter physical-joint verification,
local-bounds buoyancy and wave-relative drag. Original findings below remain historical;
the full solo contract and moving vessel/proxy are still not accepted.

Request: keep ship out of the game, use a platform above water, prepare water
physics and revise existing mechanics. R6 Blender master remains unchanged.

| Existing mechanic | Finding | Current treatment |
|---|---|---|
| First-person movement | Camera-relative input exists; no jump/swimming | Add jump and surface swimming through CharacterMovement |
| Carry / reservation / drop | Server-authoritative state machine is valid | Preserve; entering water releases cargo; swimming needs free hands |
| Throw / impact / knockdown | Throws only along actor yaw | Use bounded view pitch; retain server authority and lifecycle |
| Bulky / unstable / fragile | Differentiated co-op cargo, useful regression | Retain on platform; buoyancy applies only to free simulated cargo |
| Handoff | Validated owner RPC and cooldown | Retain, disallow grabbing while swimming |
| Delivery / timed shift / restart | Suitable test objective | Keep separate dry delivery zone on platform |
| Overboard rescue | Fixed small-ship coordinates and instant sea reset | WaterLab uses ocean-relative outer/depth failsafe instead |
| Ocean | Non-colliding decorative cube | Shared wave sampler + visible displaced mesh + multi-point buoyancy/drag |
| Leak / repair / power / director | Compartment model, not open ocean | Preserve in legacy labs; do not spawn on the platform |
| Interior cutaway | Specific to old freighter architecture | Preserve in FreighterDeck only |

Implementation gate: new WaterLab default map without any ship actor; automation,
actual Host/Client/LateJoin swimming/buoyancy evidence, legacy gameplay regression,
and visual inspection. No new RPC outcomes from clients, no firewall changes.

Water scope: game-oriented analytic waves and rigid-body buoyancy, not CFD,
breaking waves, hull hydrodynamics, deep diving or finished ship physics.
CPU forces and visible displacement must use the same server-time wave function.
Future hull uses multiple force sample locations, not a single Z bobbing offset.
The current platform is fixed, intentionally not a boat substitute.

## Completed and remaining

The planned WaterLab, wave forces, swimming, aimed throw, jump binding, dry
delivery zone and ocean-relative rescue are implemented. See
[handover](../../WATERLAB_HANDOVER.md) for build, automation and three-role evidence.
Ship-specific mechanics were deliberately retained in legacy maps, not globally
deleted or presented as meaningful on an open platform. No new tools were needed.

Still prototype-level: eight-metre nearest-visible-crate grab targeting, cargo
body replication, tilt-based knockdown, four-person loopback sessions and debug
HUD. These are explicitly not claimed to have been rebuilt into production
systems. Future work: shorter aim-based reach, salvage/throw-to-water objectives,
swim audiovisual feedback, adverse-network tests and mass-calibrated hull physics
when the user authorizes bringing the ship back.
