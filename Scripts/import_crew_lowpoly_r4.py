"""Import only the new LP4 crew folder; no map/cargo/water or old asset writes."""
from pathlib import Path
import unreal, time
ROOT=Path(__file__).resolve().parents[1]
DEST='/Game/Characters/CrewLowPolyR4'
repair='LP4ReimportHeads' in unreal.SystemLibrary.get_command_line()
assert repair or not unreal.EditorAssetLibrary.does_directory_exist(DEST),'Preserve existing candidate'
tools=unreal.AssetToolsHelpers.get_asset_tools()
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
files=sorted((ROOT/'SourceAssets/Generated/CrewLowPolyR4').glob('SM_*.glb'))
assert len(files)==20
if repair:files=[f for f in files if f.stem.endswith('_Head')]
for f in files:
    task=unreal.AssetImportTask();task.filename=str(f);task.destination_path=DEST;task.automated=True;task.save=True
    task.replace_existing=repair
    tools.import_asset_tasks([task])
    candidates=[unreal.load_asset(p) for p in task.imported_object_paths]
    mesh=next((a for a in candidates if isinstance(a,unreal.StaticMesh)),None)
    assert mesh,f
    target=DEST+'/'+f.stem
    if mesh.get_path_name().split('.')[0]!=target:
        assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0],target)
    editor.remove_collisions(mesh)
    assert editor.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.NDOP26)>=0
    settings=editor.get_nanite_settings(mesh);settings.enabled=False;editor.set_nanite_settings(mesh,settings,True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log('LP4_IMPORT '+target+' '+str(mesh.get_bounds()))
for name in ('Deckhand','Misha'):
    head=unreal.load_asset(DEST+'/SM_'+name+'_Head')
    if repair:assert head.get_bounds().origin.z-head.get_bounds().box_extent.z < -4.9,'Neck correction missing'
unreal.log('LP4_IMPORT_PASS meshes='+str(len(files))+' maps_changed=0 old_assets_changed=0')
# Start the grace period at the first Slate tick, not before editor startup completes.
_quit_after=None
def finish(_dt):
    global _quit_after
    if _quit_after is None:_quit_after=time.monotonic()+12
    if time.monotonic()>=_quit_after:
        unreal.unregister_slate_post_tick_callback(_handle);unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(finish)
