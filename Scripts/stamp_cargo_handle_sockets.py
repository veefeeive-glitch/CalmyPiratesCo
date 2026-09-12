"""Stamp Grip_* sockets on existing Cargo R3 static meshes (Editor Python).

Does not reimport GLBs or invent new art. Socket locals match FCalmyCargoGrip::HandleLocal
converted through the runtime CargoVisual relative offset:
  RelZ = -50 - mesh.bounds.min.z
  SocketLocal = HandleLocal - (0, 0, RelZ)

Run once from Unreal Editor Python (or Stamp-HeroG1Sockets.ps1). Safe to re-run:
existing sockets with the same names are updated in place.
"""
from __future__ import annotations

import unreal

KINDS = ("Standard", "Bulky", "Unstable", "Fragile")
# Mirrors Source/CalmyPiratesCo/Public/Core/CargoGrip.h HandleLocal.
HANDLES = {
    "Standard": {
        "Grip_Near_L": unreal.Vector(-48.0, -42.0, 18.0),
        "Grip_Near_R": unreal.Vector(-48.0, 42.0, 18.0),
        "Grip_Far_L": unreal.Vector(48.0, 42.0, 18.0),
        "Grip_Far_R": unreal.Vector(48.0, -42.0, 18.0),
    },
    "Bulky": {
        "Grip_Near_L": unreal.Vector(-52.0, -40.0, 26.0),
        "Grip_Near_R": unreal.Vector(-52.0, 40.0, 26.0),
        "Grip_Far_L": unreal.Vector(52.0, 40.0, 26.0),
        "Grip_Far_R": unreal.Vector(52.0, -40.0, 26.0),
    },
    "Unstable": {
        "Grip_Near_L": unreal.Vector(-44.0, -36.0, 14.0),
        "Grip_Near_R": unreal.Vector(-44.0, 36.0, 14.0),
        "Grip_Far_L": unreal.Vector(44.0, 36.0, 14.0),
        "Grip_Far_R": unreal.Vector(44.0, -36.0, 14.0),
    },
    "Fragile": {
        "Grip_Near_L": unreal.Vector(-46.0, -40.0, 10.0),
        "Grip_Near_R": unreal.Vector(-46.0, 40.0, 10.0),
        "Grip_Far_L": unreal.Vector(46.0, 40.0, 10.0),
        "Grip_Far_R": unreal.Vector(46.0, -40.0, 10.0),
    },
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


def stamp_kind(kind: str) -> int:
    path = f"/Game/Cargo/R3/SM_Cargo_{kind}"
    mesh = unreal.load_asset(path)
    assert mesh and isinstance(mesh, unreal.StaticMesh), path
    bounds = mesh.get_bounding_box()
    rel_z = -50.0 - float(bounds.min.z)
    count = 0
    for name, handle in HANDLES[kind].items():
        local = unreal.Vector(handle.x, handle.y, handle.z - rel_z)
        _ensure_socket(mesh, name, local)
        count += 1
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh):
        unreal.log_warning(f"CARGO_GRIP_SOCKETS_SAVE_FAIL {path} (in-session sockets may not persist)")
    elif not unreal.EditorAssetLibrary.save_asset(pkg_path):
        unreal.log_warning(f"CARGO_GRIP_SOCKETS_SAVE_ASSET_FAIL {path}")
    unreal.log(f"CARGO_GRIP_SOCKETS {path} count={count} rel_z={rel_z:.3f}")
    return count


def main() -> None:
    total = sum(stamp_kind(kind) for kind in KINDS)
    unreal.log(f"CARGO_GRIP_SOCKETS_PASS total={total}")


main()
