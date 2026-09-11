# Repository rules

- Public title: **Calmy Pirates Co.**
- Engine baseline: Unreal Engine `5.8.2` until an approved migration is recorded.
- This is a modern coastal-freighter co-op comedy, not a fantasy pirate game.
- Host/server owns gameplay outcomes; clients submit validated intent and own input, camera, and cosmetic feedback.
- Preserve explicit state machines: carryable `Free -> Reserved -> Held -> Thrown -> Free`; player `Controlled -> KnockedDown -> Recovering -> Controlled`.
- Every RPC must document caller, receiver, validation, duplicate behavior, late join, and disconnect behavior.
- Greybox and measurable play value come before bulk asset import or presentation polish.
- Do not expose Unreal MCP outside loopback or change Windows Firewall rules.
- Do not hand-edit `.uasset` or `.umap` binary files.
- Do not claim a human fun gate passed without the required human players.
- Never push, merge, rewrite history, delete project roots, or enable bypass permissions autonomously.
- A gameplay change is incomplete without relevant automation tests and Host/Client evidence.

## Continuity and documentation

- Start continuation with HANDOVER.md, Docs/Handover/CHANGELOG.md and the relevant Current document. Inspect actual code/asset paths: the newest Blender candidate is not necessarily in Unreal.
- After each material task update HANDOVER.md and Docs/Current/STATUS.md, add a dated CHANGELOG entry and update affected Current/roadmap sections. Include implemented, unfinished/failed work, runtime versus authoring versions, test results/evidence and the next safe step. Metadata is not gameplay PASS or human acceptance.
- Keep substantial reports in Docs/Changes. Frozen Evidence and historical reports stay immutable; corrections belong in current documents or a new report.
- Use repository-relative links inside documents. Do not move UE assets, Blender dependencies or root-relative scripts just to tidy folders; preserve compatibility and maintain indexes.
- Run Scripts/Maintenance/Update-HandoverInventory.ps1 after changes and Scripts/verify_project_docs.ps1 before handoff. Before replacing an imported inventory on another device, use -Verify and investigate differences. This is not a background progress tracker, sync or backup service.
- Check Git tracked files, commits and remote before relying on Git. Do not commit/push or alter publishing merely to satisfy this checklist without an explicit relevant request.
