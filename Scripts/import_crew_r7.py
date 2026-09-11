"""New R7 assets only. Keeps maps, R5/R6 and all gameplay settings untouched.

Run from the editor's Python, then flip the two mesh paths in
CalmyCrewPresentation.cpp and CrewLowPolyTests.cpp from CrewR6B to CrewR7.
"""
from pathlib import Path
import unreal, time
ROOT = Path(__file__).resolve().parents[1]; DEST = '/Game/Characters/CrewR7'
assert not unreal.EditorAssetLibrary.does_directory_exist(DEST), 'Preserve candidate before import'
tools = unreal.AssetToolsHelpers.get_asset_tools()
editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
files = sorted((ROOT / 'SourceAssets/Generated/CrewR7').glob('SM_*.glb')); assert len(files) == 20
for f in files:
    task = unreal.AssetImportTask(); task.filename = str(f); task.destination_path = DEST
    task.automated = True; task.save = True
    tools.import_asset_tasks([task])
    mesh = next((a for p in task.imported_object_paths if isinstance(a := unreal.load_asset(p), unreal.StaticMesh)), None)
    assert mesh, f
    target = DEST + '/' + f.stem
    if mesh.get_path_name().split('.')[0] != target:
        assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0], target)
    editor.remove_collisions(mesh)
    assert editor.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.NDOP26) >= 0
    settings = editor.get_nanite_settings(mesh); settings.enabled = False
    editor.set_nanite_settings(mesh, settings, True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log('CREW_R7_IMPORT ' + target)
unreal.log('CREW_R7_IMPORT_PASS meshes=20 maps_changed=0')
_quit_after = None
def finish(_dt):
    global _quit_after
    if _quit_after is None: _quit_after = time.monotonic() + 12
    if time.monotonic() >= _quit_after:
        unreal.unregister_slate_post_tick_callback(_handle); unreal.SystemLibrary.quit_editor()
_handle = unreal.register_slate_post_tick_callback(finish)
