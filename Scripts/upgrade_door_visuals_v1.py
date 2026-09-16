"""Rebuild all 61 R7 interactive doors onto SM_WatertightDoor visuals; clear blue cube overrides."""
import hashlib
import shutil
from pathlib import Path

import unreal

root = Path(__file__).resolve().parents[1]
mp = root / 'Content/Maps/OceanFreighterR7.umap'
backup = root / 'SourceAssets/Backups/Before-DoorVisual-V1-20260913'
backup.mkdir(parents=True, exist_ok=True)
shutil.copy2(mp, backup / mp.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')

door_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyDoor')
mesh = unreal.load_asset('/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/StaticMeshes/SM_WatertightDoor')
blue = unreal.load_asset('/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_EquipmentBlue')
dark = unreal.load_asset('/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Machinery')
safety = unreal.load_asset('/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Safety')
assert door_cls and mesh and blue and dark and safety

doors = [a for a in actors.get_all_level_actors() if a.get_class() == door_cls]
assert len(doors) == 61, 'expected 61 doors, got %d' % len(doors)

for d in doors:
    d.set_editor_property('start_open', False)
    leaf = d.get_editor_property('leaf')
    front = d.get_editor_property('handle_front')
    back = d.get_editor_property('handle_back')
    leaf.set_static_mesh(mesh)
    leaf.set_material(0, blue)
    if leaf.get_num_materials() > 1:
        leaf.set_material(1, dark)
    if leaf.get_num_materials() > 2:
        leaf.set_material(2, safety)
    for h in (front, back):
        h.set_visibility(False, False)
        h.set_hidden_in_game(True)
    d.rebuild_geometry()

assert levels.save_current_level()
sha = hashlib.sha256(mp.read_bytes()).hexdigest()
unreal.log('DOOR_VISUAL_V1_PASS doors=%d mesh=%s map_sha256=%s backup=%s' % (
    len(doors), mesh.get_name(), sha, backup.as_posix()))
unreal.SystemLibrary.quit_editor()
