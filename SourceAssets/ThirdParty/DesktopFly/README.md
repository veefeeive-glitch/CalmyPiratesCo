# DesktopFly / FlyWire provenance

- Upstream: https://github.com/DenisSergeevitch/desktop-fly
- Pinned commit: `32b00011e83c3dc85fa3ea0b3934155b04f1635d`.
- `circuit.json` is an unmodified copy of upstream `data/circuit.json`.
- SHA256: `da53640e4dc7071c26892870ec5b28984c4cfc74c1c44d3a3cde0a2913579d0f`.
- Anatomy: FlyWire Codex FAFB v783, 668 neurons, 18,968 directed connection rows.
- Code adaptation: `Sim.swift`, `windows/src/sim.js`; MIT, copyright (c) 2026 Denis Shiryaev. Full notice in `LICENSE-MIT.txt`.
- Source and generated connectome data: **CC BY-NC 4.0**, attribution and noncommercial use. See `DATA_LICENSE.md`. MIT on the simulation code does not relicense the dataset.
- Dorkenwald et al., *Neuronal wiring diagram of an adult brain*, Nature 634, 124–138 (2024), https://doi.org/10.1038/s41586-024-07558-y.
- Schlegel et al., *Whole-brain annotation and multi-connectome cell typing of Drosophila*, Nature 634, 139–152 (2024), https://doi.org/10.1038/s41586-024-07686-5.

The generated `.inl` embeds this data in the game module, including packaged binaries. Commercial distribution needs appropriate rights or a replacement dataset; it is not made commercial by conversion to C++. Preserve notices with redistribution. No MaleCNS graph or brain visualization point cloud is used.

Implementation and model differences: [Fly Swarm guide](../../../Docs/Changes/FLY_SWARM_2026-09-10.md).
