> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# R7 cargo-clearance patch — scoped close-up review

Reviewed 2026-09-06. Candidate C06 SHA256:
`8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93`.

All 17 final C06 images were visually inspected, not just counted. Render inputs:
`Saved/Verification/R7-Cargo-C06-Shots` (13) and
`Saved/Verification/R7-Cargo-C06-EngineShots` (4).
Closed Blender scene, 102-degree inspection camera, local QA fill light.
This is NOT final Unreal lighting or a live bodycam recording; some detail views
deliberately raise/aim the camera. No geometry hidden and no render changes saved.

| Views | Observation / limited disposition |
|---|---|
| mess | Complete dining set remains coherent after relocation; approach is clear, room is still sparse. |
| internal_turn, internal_headroom | Wider supported turn and enlarged overhead opening visible. Retained rails; utilitarian finish and rail junctions still need art polish. |
| hold1_entry, hold1_turn, hold2_entry | Expanded landings join flights/return galleries; exposed sides protected. No floating entrance text over the opening. |
| boat_lobby, officer_lobby | Wall moved away from return turn, continuous floor; stair/corridor approach unobstructed in these views. |
| laundry_boundary, medical_boundary, radio_boundary | Changed wall meets ceiling/outer wall; no newly visible furniture-through-wall intersection. Not proof of every appliance work position. |
| hold1_sign, hold2_sign | Upright legible text on dark backboard, two supports behind board; C04 low contrast and C05 coplanar post flicker corrected in C06. |
| C01_-1_aisle, C01_1_aisle | Cooler bases moved outboard; expanded walking strip, retained safety railing. |
| C01_-1_connections, C01_1_connections | Pipe/flange positions preserved within translated assembly, BUT visual piping/support defects remain (below). |

## Open findings, not waived by clearance PASS

- **R7-ART-01:** HX2 red/green piping appears to intersect/share an unmodelled junction in `C01_1_aisle` / `C01_1_connections`. Inspect individual evaluated curves and author a coherent separated circuit, then repeat both cooler cargo paths. Translating the complete assembly did not fix its existing internal routing.
- **R7-ART-02:** Upper ends of several thin pipe hangers appear to stop below the overhead structure in both connection closeups. Verify exact anchor/ceiling bounds and add real attachments; do not claim machinery attachment completeness from four pump-specific tests.
- **R7-ART-03:** Room fixtures, wall finishes, rail joints and emergency/engine lighting remain prototype-quality. These 17 scoped images are not a whole-ship art review.

Disposition: visual review supports publishing the **bounded clearance/layout
improvement**, with the above defects explicitly open. It does not accept overall
ship completion, new percentage, production export or multiplayer integration.
