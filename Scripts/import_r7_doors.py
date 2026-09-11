import unreal,json,hashlib,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1];src=root/'SourceAssets/Generated/ShipDoorsR1';dest='/Game/Ships/OceanFreighterR7/DoorsR1'
assets=unreal.EditorAssetLibrary;lib=unreal.MaterialEditingLibrary
mp=root/'Content/Maps/OceanFreighterR7.umap'
assert hashlib.sha256(mp.read_bytes()).hexdigest()=='cc86d82e9304ef95185289a945847a0d9fd92dd89bdc50bd3089c963298ef244'
backup=root/'SourceAssets/Backups/Before-DoorsR1';backup.mkdir(parents=True,exist_ok=True);shutil.copy2(mp,backup/mp.name)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
old=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='SM_R7_30_SUPERSTRUCTURE')
c=old.static_mesh_component;original={}
# Match by original mesh-slot material name, preserving current actor overrides exactly.
for i,slot in enumerate(c.static_mesh.get_editor_property('static_materials')):original[slot.material_interface.get_name()]=c.get_material(i)
name='SM_R7_30_SUPERSTRUCTURE_DoorsR1';task=unreal.AssetImportTask();task.filename=str(src/(name+'.glb'));task.destination_path=dest;task.automated=True;task.save=True
existing=dest+'/'+name+'/StaticMeshes/'+name
if assets.does_asset_exist(existing):mesh=unreal.load_asset(existing)
else:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=next(unreal.load_asset(p) for p in task.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh))
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem);editor.remove_collisions(mesh)
body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE);body.set_editor_property('double_sided_geometry',True)
settings=editor.get_nanite_settings(mesh);settings.enabled=True;settings.fallback_target=unreal.NaniteFallbackTarget.PERCENT_TRIANGLES;settings.fallback_percent_triangles=1.;settings.fallback_relative_error=0.
editor.set_nanite_settings(mesh,settings,True);assert assets.save_loaded_asset(mesh)
c.set_static_mesh(mesh)
for i,slot in enumerate(mesh.get_editor_property('static_materials')):
    assert slot.material_interface.get_name() in original,slot.material_interface.get_name()
    c.set_material(i,original[slot.material_interface.get_name()])
blue=original['MAT_Machinery_Blue'];steel=original['MAT_Steel_PBR']
cls=unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyDoor');assert cls
fitted={int(r[0]):r for r in json.loads((src/'runtime-swings.json').read_text())} if (src/'runtime-swings.json').exists() else {}
for d in json.loads((src/'doors.json').read_text())['doors']:
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*d['pivot_cm']),unreal.Rotator(yaw=d['yaw']))
    a.set_actor_label('Door_'+str(d['id'])+'_'+d['name']);a.set_folder_path('R7/InteractiveDoors')
    a.set_editor_property('door_id',d['id']);a.set_editor_property('width_cm',d['width_cm']);a.set_editor_property('height_cm',d['height_cm'])
    a.get_editor_property('leaf').set_material(0,blue)
    for prop in ['handle_front','handle_back']:a.get_editor_property(prop).set_material(0,steel)
    if d['id'] in fitted:
        _,ok,x,y,z,yaw,sign,angle=fitted[d['id']];assert ok=='1'
        a.set_actor_location_and_rotation(unreal.Vector(float(x),float(y),float(z)),unreal.Rotator(yaw=float(yaw)),False,False)
        a.set_editor_property('swing_sign',float(sign));a.set_editor_property('open_angle',float(angle))
    # Post-edit reruns native construction with final dimensions.
    a.rebuild_geometry()
assert len([a for a in actors.get_all_level_actors() if a.get_class()==cls])==61
assert levels.save_current_level()
unreal.log('DOORS_IMPORT_PASS doors=61 removed_static_leaves=61 preserved_override_slots='+str(len(original)))
unreal.SystemLibrary.quit_editor()
