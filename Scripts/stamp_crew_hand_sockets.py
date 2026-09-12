"""Stamp Wrist/Palm/Tip sockets on CrewR8 forearm static meshes (Editor Python).

Uses FCalmyCrewHand locals from Core/CrewHand.h. Does not create new finger meshes
or continuous skin — only named sockets on existing baked glove geometry.

Safe to re-run. Paths: /Game/Characters/CrewR8/SM_{Deckhand|Misha}_Forearm{L|R}
"""
from __future__ import annotations

import unreal

VARIANTS = ("Deckhand", "Misha")
SIDES = ("L", "R")
SOCKETS = {
    "Wrist": unreal.Vector(0.0, 0.0, -24.0),
    "Palm": unreal.Vector(0.0, 0.0, -33.0),
    "Tip": unreal.Vector(2.0, 0.0, -36.0),
}


def _ensure_socket(mesh: unreal.StaticMesh, name: str, local: unreal.Vector) -> None:
    existing = mesh.find_socket(name)
    if existing is not None:
        mesh.remove_socket(existing)
    # Outer must be the mesh or the socket is transient and save drops it.
    socket = unreal.StaticMeshSocket(mesh)
    socket.set_editor_property("socket_name", unreal.Name(name))
    socket.set_editor_property("relative_location", local)
    socket.set_editor_property("relative_rotation", unreal.Rotator(0, 0, 0))
    socket.set_editor_property("relative_scale", unreal.Vector(1, 1, 1))
    mesh.add_socket(socket)
    mesh.modify()


def stamp_forearm(variant: str, side: str) -> int:
    path = f"/Game/Characters/CrewR8/SM_{variant}_Forearm{side}"
    mesh = unreal.load_asset(path)
    assert mesh and isinstance(mesh, unreal.StaticMesh), path
    for name, local in SOCKETS.items():
        _ensure_socket(mesh, name, local)
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        unreal.log_warning(f"CREW_HAND_SOCKETS_SAVE_FAIL {path} (in-session sockets may not persist)")
    elif not unreal.EditorAssetLibrary.save_asset(path):
        unreal.log_warning(f"CREW_HAND_SOCKETS_SAVE_ASSET_FAIL {path}")
    unreal.log(f"CREW_HAND_SOCKETS {path} count={len(SOCKETS)}")
    return len(SOCKETS)


def main() -> None:
    total = 0
    for variant in VARIANTS:
        for side in SIDES:
            total += stamp_forearm(variant, side)
    unreal.log(f"CREW_HAND_SOCKETS_PASS total={total}")


main()
