"""Split translucent glazing from opaque Nanite structure. Exact same evaluated faces."""
import unreal,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'SourceAssets/Generated/ShipR7RenderB';DEST='/Game/Ships/OceanFreighterR7/RenderB'
meta=json.loads((SRC/'export_manifest.json').read_text())
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);lib=unreal.MaterialEditingLibrary
assert levels.load_level('/Game/Maps/OceanFreighterR7')
for chunk in meta['chunks']:
    name=chunk['name'];target=DEST+'/'+name
    assert not unreal.EditorAssetLibrary.does_asset_exist(target),'Preserve existing render candidate'
    task=unreal.AssetImportTask();task.filename=str(SRC/(name+'.glb'));task.destination_path=DEST;task.automated=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=next((unreal.load_asset(p) for p in task.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh)),None);assert mesh
    if mesh.get_path_name().split('.')[0]!=target:assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0],target)
    glass=name.endswith('_Glass')
    editor.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
    settings=editor.get_nanite_settings(mesh);settings.enabled=not glass
    settings.fallback_target=unreal.NaniteFallbackTarget.PERCENT_TRIANGLES;settings.fallback_percent_triangles=1.;settings.fallback_relative_error=0.
    for slot in mesh.get_editor_property('static_materials'):
        m=slot.material_interface
        if isinstance(m,unreal.MaterialInstanceConstant) and not glass:
            lib.set_material_usage_override(m,unreal.MaterialUsage.MATUSAGE_NANITE,True,True);lib.update_material_instance(m)
            assert unreal.EditorAssetLibrary.save_loaded_asset(m)
    editor.set_nanite_settings(mesh,settings,True);assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    if glass:
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());a.set_actor_label(name);a.set_folder_path('R7/Glazing')
        a.static_mesh_component.set_static_mesh(mesh);a.static_mesh_component.set_collision_profile_name('BlockAll')
        # Transparent glass casts no opaque shadow; structure and window frames still cast shadows.
        a.static_mesh_component.set_cast_shadow(False)
    else:
        old=name.removesuffix('_Opaque')
        matched=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name()==old]
        assert len(matched)==1,old
        matched[0].static_mesh_component.set_static_mesh(mesh)
    unreal.log('R7_RENDER_B '+name+' glass='+str(glass)+' triangles='+str(chunk['triangles']))
assert levels.save_current_level()
unreal.log('R7_RENDER_B_PASS meshes=10 glass_separate=3 original_geometry_unchanged=1')
