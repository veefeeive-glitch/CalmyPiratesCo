# Ship model visual pass — 2026-09-13

Baseline (before cargo relocate / voyage G3b): `Saved/Verification/Ship-Visual-Baseline-20260913/` — **7 PNG** (overview, bridge, engine, 4 bodycam route points). Official `run_ship_r7_review.ps1` abslog path quirk; frames written by probe Photo().

Ship Host/Client/LateJoin network **PASS** in REGRESSION_SWEEP (route weather→engine return) — no P0 passage/collision softlock found on the probe route.

## Defect catalog (from baseline frames)

| ID | Severity | Evidence | Action this pass |
|---|---|---|---|
| R7-ART-01 | P1 | Engine frame `07-engine-camera-only.png`: green pipes appear to pierce supports without fittings | Backlog — Blender HX/engine master; no mass reimport |
| R7-ART-02 | P1 | Prior docs hangers; not isolated as collision block on route | Backlog |
| R7-VIS-GLARE | P1 | Engine worklight blown highlight on pump/ceiling | Backlog — tune `R7_Engine_Worklight` intensities carefully later |
| R7-VIS-FOG-SHADOW | P1 | Overview `01-ship-overview.png`: ghost silhouette in fog | Backlog lighting/fog |
| R7-ART-03 | P1 | Flat walls / placeholder door brown rect in engine corridor | Roadmap S2 |

**Stop:** no P0 geometry/collision edit without before/after + Door/Ship smoke. This pass records baseline and leaves ART to Blender dream-loop.

Map hash before cargo move will be captured in cargo relocate backup `SourceAssets/Backups/Before-CargoHold1-20260913`.
