"""New platform-only sandbox; existing freighter maps and Blender source untouched."""
import unreal,math
MAP='/Game/Maps/WaterLab'
if unreal.EditorAssetLibrary.does_asset_exist(MAP):raise RuntimeError('WaterLab exists; preserve edits, use a scoped update script')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets=unreal.AssetToolsHelpers.get_asset_tools()
lib=unreal.MaterialEditingLibrary
def material(name,color):
    path='/Game/Materials/'+name
    if unreal.EditorAssetLibrary.does_asset_exist(path):return unreal.load_asset(path)
    m=assets.create_asset(name,'/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
    n=lib.create_material_expression(m,unreal.MaterialExpressionVectorParameter)
    n.set_editor_property('parameter_name','Tint');n.set_editor_property('default_value',unreal.LinearColor(*color,1))
    lib.connect_material_property(n,'',unreal.MaterialProperty.MP_BASE_COLOR)
    return m
def scalar(m,v,prop):
    n=lib.create_material_expression(m,unreal.MaterialExpressionConstant);n.set_editor_property('r',v)
    lib.connect_material_property(n,'',prop);return n
water=material('M_WaterLab_Ocean',(.018,.09,.115))
water.set_editor_property('shading_model',getattr(unreal.MaterialShadingModel,next(x for x in dir(unreal.MaterialShadingModel) if 'WATER' in x)))
scalar(water,.14,unreal.MaterialProperty.MP_ROUGHNESS)
scalar(water,.5,unreal.MaterialProperty.MP_SPECULAR)
scalar(water,.18,unreal.MaterialProperty.MP_OPACITY)
out=lib.create_material_expression(water,unreal.MaterialExpressionSingleLayerWaterMaterialOutput)
for name,color in [('ScatteringCoefficients',(.0004,.0014,.0018)),('AbsorptionCoefficients',(.005,.0018,.0008))]:
    n=lib.create_material_expression(water,unreal.MaterialExpressionConstant3Vector)
    n.set_editor_property('constant',unreal.LinearColor(*color,1))
    if not lib.connect_material_expressions(n,'',out,name):raise RuntimeError('Water input connection: '+name)
deck=material('M_WaterLab_Deck',(.14,.19,.22));orange=material('M_WaterLab_Orange',(.9,.35,.03));green=material('M_WaterLab_Delivery',(.05,.5,.18))
for m in (water,deck,orange,green):lib.recompile_material(m);unreal.EditorAssetLibrary.save_loaded_asset(m)
levels.new_level(MAP)
cube=unreal.load_asset('/Engine/BasicShapes/Cube')
def box(name,p,d,m,rotation=None,collision=True):
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*p),rotation or unreal.Rotator())
    a.set_actor_label(name);a.static_mesh_component.set_static_mesh(cube);a.static_mesh_component.set_material(0,m)
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in d)))
    if not collision:a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return a
box('WaterLab_FIXED_PLATFORM_NO_SHIP',(0,0,-35),(3200,2000,70),deck)
for y in (-1000,1000):
    for x in range(-1500,1501,300):box('PlatformRailPost',(x,y,55),(9,9,110),orange)
    for z in (55,110):box('PlatformSideRail',(0,y,z),(3200,8,8),orange)
box('PlatformAftRail',(-1600,0,100),(10,2000,12),orange)
for y in (-650,650):box('PlatformRampSideRail',(1600,y,95),(10,700,12),orange)
# Wide ramp from top Z=0 to submerged Z=-360. No invisible water collision.
length=math.hypot(1200,360)
box('WaterLab_WADE_RAMP',(2200,0,-202),(length,600,44),deck,unreal.Rotator(pitch=-math.degrees(math.atan2(360,1200))))
for y in (-305,305):box('RampHandrail',(2200,y,-75),(length,8,8),orange,unreal.Rotator(pitch=-math.degrees(math.atan2(360,1200))))
for y in (-305,305):
    for x in range(1600,2801,200):box('RampSupport_'+str(x)+'_'+str(y),(x,y,52-(x-1600)*.3),(8,8,108),orange)
box('DeliveryMark',(-900,600,1),(700,550,2),green,collision=False)
shift=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyShift'),unreal.Vector(-900,600,120))
shift.delivery_zone.set_box_extent(unreal.Vector(350,275,150));shift.set_actor_label('WaterLab_DryDelivery')
for i,(x,y) in enumerate([(-600,-300),(-600,0),(-600,300),(-300,-300)]):
    p=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(x,y,95));p.set_editor_property('tags',[unreal.Name('P'+str(i))])
for i,kind in enumerate([unreal.CalmyCargoKind.STANDARD,unreal.CalmyCargoKind.UNSTABLE,unreal.CalmyCargoKind.FRAGILE,unreal.CalmyCargoKind.BULKY]):
    c=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyCargo'),unreal.Vector(200+i*250,-550,65))
    c.set_editor_property('cargo_id',i+1);c.set_editor_property('kind',kind);c.set_actor_label('WaterLab_Cargo_'+str(i+1))
    c.mesh.set_material(0,unreal.load_asset('/Game/Materials/M_Cargo'))
o=actors.spawn_actor_from_class(unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyOcean'),unreal.Vector(0,0,-220))
o.set_editor_property('surface_material',water);o.set_actor_label('WaterLab_Ocean_PhysicalField')
sun=actors.spawn_actor_from_class(unreal.DirectionalLight,unreal.Vector(0,0,2000),unreal.Rotator(pitch=-28,yaw=-35))
sun.light_component.set_editor_property('intensity',5);sun.light_component.set_editor_property('atmosphere_sun_light',True)
actors.spawn_actor_from_class(unreal.SkyAtmosphere,unreal.Vector(0,0,-1000))
sky=actors.spawn_actor_from_class(unreal.SkyLight,unreal.Vector(0,0,1000));sky.light_component.set_editor_property('real_time_capture',True)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyGameMode'))
assert levels.save_current_level()
unreal.log('CALMY_WATERLAB_CREATED '+MAP)
