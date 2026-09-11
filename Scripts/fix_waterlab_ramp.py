"""Scoped correction to our newly-created map; no old ship assets touched."""
import unreal,math
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
levels.load_level('/Game/Maps/WaterLab')
count=0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label()=='WaterLab_WADE_RAMP' or a.get_actor_label().startswith('RampHandrail'):
        a.set_actor_rotation(unreal.Rotator(pitch=-math.degrees(math.atan2(360,1200))),True);count+=1
    if isinstance(a,unreal.DirectionalLight):a.set_actor_rotation(unreal.Rotator(pitch=-28,yaw=-35),True)
assert count==3,count
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
labels={a.get_actor_label() for a in actors.get_all_level_actors()}
for y in (-305,305):
    for x in range(1600,2801,200):
        label='RampSupport_'+str(x)+'_'+str(y)
        if label in labels:continue
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(x,y,52-(x-1600)*.3))
        a.set_actor_label(label);a.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
        a.static_mesh_component.set_material(0,unreal.load_asset('/Game/Materials/M_WaterLab_Orange'))
        a.set_actor_scale3d(unreal.Vector(.08,.08,1.08))
assert levels.save_current_level()
unreal.log('WATERLAB_RAMP_FIXED pitch not roll')
