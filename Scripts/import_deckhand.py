from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[1]
DEST='/Game/Characters/Deckhand'
tools=unreal.AssetToolsHelpers.get_asset_tools()
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert mesh_editor,'Run in full editor with -ExecutePythonScript; commandlet has no StaticMeshEditorSubsystem'
for f in sorted((ROOT/'SourceAssets/Generated/Deckhand').glob('SM_*.glb')):
    target=DEST+'/'+f.stem
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        mesh=unreal.load_asset(target)
    else:
        task=unreal.AssetImportTask();task.filename=str(f);task.destination_path=DEST;task.automated=True;task.save=True
        tools.import_asset_tasks([task])
        candidates=[unreal.load_asset(p) for p in task.imported_object_paths]
        mesh=next((a for a in candidates if isinstance(a,unreal.StaticMesh)),None)
    assert mesh,f
    if mesh.get_path_name().split('.')[0]!=target:
        assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0],target)
    mesh_editor.remove_collisions(mesh)
    assert mesh_editor.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.NDOP26)>=0
    settings=mesh_editor.get_nanite_settings(mesh);settings.enabled=False
    mesh_editor.set_nanite_settings(mesh,settings,True)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    unreal.log('DECKHAND_IMPORT '+target+' bounds='+str(mesh.get_bounds()))
assert all(unreal.EditorAssetLibrary.does_asset_exist(DEST+'/SM_Deckhand_'+n) for n in ['Torso','Head','UpperArmL','ForearmL','UpperArmR','ForearmR','ThighL','ShinL','ThighR','ShinR'])
unreal.log('DECKHAND_IMPORT_PASS 10')
unreal.SystemLibrary.quit_editor()
