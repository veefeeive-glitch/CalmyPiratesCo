"""Import original fly and append one saved swarm to each playable map via UE APIs."""
from pathlib import Path
import unreal, json, hashlib, shutil, time
ROOT=Path(__file__).resolve().parents[1]
DEST='/Game/Creatures/FlyR2'
assets=unreal.EditorAssetLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
report={}
mesh_path=DEST+'/SM_Fly_R2_Corrected'
if not assets.does_asset_exist(mesh_path):
    t=unreal.AssetImportTask();t.filename=str(ROOT/'SourceAssets/Generated/FlyR2/SM_Fly_R2_Corrected.glb');t.destination_path=DEST;t.automated=True;t.save=True
    tools.import_asset_tasks([t])
    mesh=next((a for p in t.imported_object_paths if isinstance(a:=unreal.load_asset(p),unreal.StaticMesh)),None)
    assert mesh, 'No fly mesh imported'
    if mesh.get_path_name().split('.')[0]!=mesh_path:assert assets.rename_asset(mesh.get_path_name().split('.')[0],mesh_path)
else:mesh=unreal.load_asset(mesh_path)
editor.remove_collisions(mesh)
settings=editor.get_nanite_settings(mesh);settings.enabled=False;editor.set_nanite_settings(mesh,settings,True)

# One lightweight material for both wings. WPO uses authored red vertex mask.
lib=unreal.MaterialEditingLibrary
wing_path=DEST+'/M_Fly_WingMotion'
mat=unreal.load_asset(wing_path) if assets.does_asset_exist(wing_path) else tools.create_asset('M_Fly_WingMotion',DEST,unreal.Material,unreal.MaterialFactoryNew())
lib.delete_all_material_expressions(mat)
mat.set_editor_property('two_sided',True)
color=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);color.constant=unreal.LinearColor(.42,.64,.69)
lib.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
rough=lib.create_material_expression(mat,unreal.MaterialExpressionConstant);rough.r=.27;lib.connect_material_property(rough,'',unreal.MaterialProperty.MP_ROUGHNESS)
clock=lib.create_material_expression(mat,unreal.MaterialExpressionTime)
wave=lib.create_material_expression(mat,unreal.MaterialExpressionSine);wave.set_editor_property('period',.065)
assert lib.connect_material_expressions(clock,'',wave,'')
mask=lib.create_material_expression(mat,unreal.MaterialExpressionVertexColor)
mul=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply)
lib.connect_material_expressions(wave,'',mul,'A');lib.connect_material_expressions(mask,'R',mul,'B')
amplitude=lib.create_material_expression(mat,unreal.MaterialExpressionConstant3Vector);amplitude.constant=unreal.LinearColor(0,0,.65)
displacement=lib.create_material_expression(mat,unreal.MaterialExpressionMultiply)
lib.connect_material_expressions(mul,'',displacement,'A');lib.connect_material_expressions(amplitude,'',displacement,'B')
lib.connect_material_property(displacement,'',unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
lib.recompile_material(mat);assert assets.save_loaded_asset(mat)
def solid(name,color,roughness):
    path=DEST+'/'+name
    if assets.does_asset_exist(path):return unreal.load_asset(path)
    result=tools.create_asset(name,DEST,unreal.Material,unreal.MaterialFactoryNew())
    c=lib.create_material_expression(result,unreal.MaterialExpressionConstant3Vector);c.constant=unreal.LinearColor(*color)
    lib.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
    r=lib.create_material_expression(result,unreal.MaterialExpressionConstant);r.r=roughness
    lib.connect_material_property(r,'',unreal.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(result);assert assets.save_loaded_asset(result)
    return result
solids={'Chitin':solid('M_FlyR2_Chitin',(.035,.022,.012),.36),'Amber':solid('M_FlyR2_Amber',(.31,.13,.025),.48),'Eyes':solid('M_FlyR2_Eyes',(.48,.017,.011),.24)}
for material in [mat,*solids.values()]:
    lib.set_material_usage(material,unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    lib.recompile_material(material);assert assets.save_loaded_asset(material)
for i,slot in enumerate(mesh.static_materials):
    if 'Wing' in str(slot.material_slot_name):mesh.set_material(i,mat)
    else:
        key=next(k for k in solids if k in str(slot.material_slot_name))
        mesh.set_material(i,solids[key]) # COLOR_0 is motion data, not a base-color multiplier.
assert assets.save_loaded_asset(mesh)

backup=ROOT/'SourceAssets/Backups/Before-Fly-R2-20260910'
backup.mkdir(parents=True,exist_ok=True)
for name,center in [('WaterLab',(-260,-320,155)),('OceanFreighterR7',(-3470,-220,2050))]:
    source=ROOT/f'Content/Maps/{name}.umap';dest=backup/source.name
    if not dest.exists():shutil.copy2(source,dest)
    before=hashlib.sha256(source.read_bytes()).hexdigest()
    assert levels.load_level('/Game/Maps/'+name)
    existing=[a for a in actors.get_all_level_actors() if a.actor_has_tag('FlyR2_Gameplay')]
    assert len(existing)<=1
    swarm=existing[0] if existing else actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.FlySwarmMob'),unreal.Vector(*center))
    swarm.set_actor_label('FlySwarm_Gameplay_R2')
    swarm.set_editor_property('tags',[unreal.Name('FlyR2_Gameplay')])
    for key,value in dict(fly_mesh=mesh,mesh_scale=1.0,fly_count=32,spawn_radius=85.0,roam_radius=145.0,danger_radius=420.0,cruise_speed=55.0,escape_speed=160.0,simulation_hz=25).items():swarm.set_editor_property(key,value)
    swarm.set_actor_location(unreal.Vector(*center),False,False)
    assert levels.save_current_level()
    assert swarm.get_editor_property('fly_mesh')==mesh
    report[name]={'position_cm':center,'flies':32,'mesh':mesh_path,'before_sha256':before,'after_sha256':hashlib.sha256(source.read_bytes()).hexdigest()}
(ROOT/'Saved/Verification/FlyR2-install.json').write_text(json.dumps(report,indent=2))
unreal.log('FLY_R2_INSTALL_PASS maps=2 flies_per_map=32 mesh='+mesh_path)
end=time.monotonic()+10
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
