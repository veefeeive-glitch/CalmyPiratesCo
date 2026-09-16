"""Static playable R7 review map. No master, WaterLab or old assets modified."""
from pathlib import Path
import unreal,json,time
ROOT=Path(__file__).resolve().parents[1];SRC=ROOT/'SourceAssets/Generated/ShipR7Game'
META=json.loads((SRC/'export_manifest.json').read_text())
MAP='/Game/Maps/OceanFreighterR7';DEST='/Game/Ships/OceanFreighterR7'
assert not unreal.EditorAssetLibrary.does_asset_exist(MAP),'Never overwrite a user map'
tools=unreal.AssetToolsHelpers.get_asset_tools();editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
meshes=[]
for chunk in META['chunks']:
    name=chunk['name'];target=DEST+'/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(target):mesh=unreal.load_asset(target)
    else:
        task=unreal.AssetImportTask();task.filename=str(SRC/(name+'.glb'));task.destination_path=DEST;task.automated=True;task.save=True
        tools.import_asset_tasks([task]);mesh=next((unreal.load_asset(p) for p in task.imported_object_paths if isinstance(unreal.load_asset(p),unreal.StaticMesh)),None)
        assert mesh,name
        if mesh.get_path_name().split('.')[0]!=target:assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0],target)
    editor.remove_collisions(mesh)
    body=mesh.get_editor_property('body_setup');body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    body.set_editor_property('double_sided_geometry',True)
    settings=editor.get_nanite_settings(mesh);settings.enabled=False;editor.set_nanite_settings(mesh,settings,True)
    assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
    if '10_HULL' in name:
        # Export group includes stern/bow appendages: observed GLB X -63.644901..62.5m,
        # while the main Hull_Complete shell itself remains 122m.
        bounds=mesh.get_bounds()
        assert abs((bounds.origin.x-bounds.box_extent.x)-(-6364.4901))<2 and abs((bounds.origin.x+bounds.box_extent.x)-6250)<2,'Wrong GLB scale/pivot'
    meshes.append(mesh);unreal.log('SHIP_R7_IMPORTED '+target)

# New material instance graph: masked SingleLayerWater, exact longitudinal hull planform.
water_path=DEST+'/M_R7_Sea'
if unreal.EditorAssetLibrary.does_asset_exist(water_path):water=unreal.load_asset(water_path)
else:water=unreal.EditorAssetLibrary.duplicate_asset('/Game/Materials/M_WaterLab_Ocean',water_path)
assert water
water.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
lib=unreal.MaterialEditingLibrary
position=lib.create_material_expression(water,unreal.MaterialExpressionWorldPosition)
mask=lib.create_material_expression(water,unreal.MaterialExpressionCustom)
mask.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT1)
input_position=unreal.CustomInput();input_position.set_editor_property('input_name','P');mask.set_editor_property('inputs',[input_position])
stations=META['hull_stations_cm'];reduced=[stations[0]]
for i in range(1,len(stations)-1):
    a,b,c=stations[i-1:i+2]
    if abs((b[1]-a[1])/(b[0]-a[0])-(c[1]-b[1])/(c[0]-b[0]))>1.e-5:reduced.append(b)
reduced.append(stations[-1])
code=f'if(P.x < {stations[0][0]} || P.x > {stations[-1][0]}) return 1;\nfloat w=0;\n'
for (x0,w0),(x1,w1) in zip(reduced,reduced[1:]):
    code+=f'if(P.x >= {x0} && P.x <= {x1}) w=lerp({w0},{w1},(P.x-({x0}))/({x1-x0}));\n'
code+='return abs(P.y) >= w ? 1 : 0;'
mask.set_editor_property('code',code)
assert lib.connect_material_expressions(position,'',mask,'P')
assert lib.connect_material_property(mask,'',unreal.MaterialProperty.MP_OPACITY_MASK)
lib.recompile_material(water);assert unreal.EditorAssetLibrary.save_loaded_asset(water)

assert levels.new_level(MAP)
for mesh in meshes:
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector());a.set_actor_label(mesh.get_name())
    a.static_mesh_component.set_static_mesh(mesh);a.static_mesh_component.set_collision_profile_name('BlockAll')
    a.set_folder_path('R7/Imported_Static_Structure')

o=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyOcean'),unreal.Vector(0,0,720))
o.set_editor_property('surface_material',water);o.set_editor_property('foam_material',unreal.load_asset('/Game/Materials/M_WaterLab_FoamR3'))
o.set_editor_property('dry_hull_stations',[unreal.Vector2D(*v) for v in META['hull_stations_cm']])
o.set_editor_property('dry_hull_vertical',[unreal.Vector2D(*v) for v in META['hull_vertical_cm']])
o.set_actor_label('R7_Static_Hull_Dry_Interior_Sea')
for i,(x,y) in enumerate([(-2400,850),(-2550,850),(-2700,850),(-2850,850)]):
    p=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(x,y,1155));p.set_editor_property('tags',[unreal.Name('P'+str(i))])
npc=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyCrewHelper'),unreal.Vector(-2370,-820,1155),unreal.Rotator(yaw=0))
npc.set_actor_label('R7_Misha_Waits_On_Opposite_Side_Deck')
# HOLD 1 stowage (floor), delivery stays on weather deck green zone.
HOLD1=[(-1150,-420,320),(-950,-420,320),(-750,-420,320),(-550,-420,320)]
for i,kind in enumerate([unreal.CalmyCargoKind.STANDARD,unreal.CalmyCargoKind.UNSTABLE,unreal.CalmyCargoKind.FRAGILE,unreal.CalmyCargoKind.BULKY]):
    x,y,z=HOLD1[i]
    c=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyCargo'),unreal.Vector(x,y,z))
    c.set_editor_property('cargo_id',i+1);c.set_editor_property('kind',kind);c.set_actor_label('R7_Hold1_Cargo_'+str(i+1))
shift=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyShift'),unreal.Vector(0,-830,1130))
shift.delivery_zone.set_box_extent(unreal.Vector(250,90,140));shift.set_actor_label('R7_DeckTask_Delivery')
mark=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(0,-830,1060))
mark.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
mark.set_actor_scale3d(unreal.Vector(5,1.8,.02));mark.static_mesh_component.set_material(0,unreal.load_asset('/Game/Materials/M_WaterLab_Delivery'))
mark.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION);mark.set_actor_label('R7_Delivery_Mark')

sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,5000),unreal.Rotator(pitch=-35,yaw=-35))
sun.light_component.set_editor_property('intensity',4);sun.light_component.set_editor_property('atmosphere_sun_light',True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,-1000))
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,4000));sky.light_component.set_editor_property('real_time_capture',True)
def light(name,p,intensity=600,radius=500):
    a=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*p));a.set_actor_label(name)
    comp=a.point_light_component;comp.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
    comp.set_editor_property('intensity',intensity);comp.set_editor_property('attenuation_radius',radius)
    comp.set_editor_property('cast_shadows',True);a.set_folder_path('R7/Review_Lighting')
for r in META['rooms']:
    light('R7_Room_'+r['name'],((r['x0']+r['x1'])*50,-r['side']*(r['width']+1.2)*50,(r['floor']+2.15)*100),950,550)
for z in (10.67,13.42,16.17,18.92):
    for x in (-49,-42,-35):light('R7_Corridor', (x*100,0,(z+2.15)*100),700,430)
for z in (2.08,4.58,7.33):
    for x in (-50,-40,-28):
        for y in (-430,430):light('R7_Engine_Worklight',(x*100,y,(z+2.0)*100),1100,600)
for x in (-1200,0,2400,3600,4800):
    for z in (400,680,990):light('R7_Hold_Worklight',(x,0,z),1800,1000)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyGameMode'))
assert levels.save_current_level()
unreal.log('SHIP_R7_MAP_PASS map='+MAP+' meshes='+str(len(meshes))+' static=1 dry_hull=1')
_after=None
def finish(_dt):
    global _after
    if _after is None:_after=time.monotonic()+15
    if time.monotonic()>=_after:
        unreal.unregister_slate_post_tick_callback(_handle);unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(finish)
