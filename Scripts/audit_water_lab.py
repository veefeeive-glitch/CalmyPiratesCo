import unreal,json
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/Maps/WaterLab')
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if 'RAMP' in a.get_actor_label() or 'Handrail' in a.get_actor_label():
        unreal.log('WATER_RAMP_AUDIT '+str((a.get_actor_label(),a.get_actor_location(),a.get_actor_rotation(),a.get_actor_scale3d(),a.static_mesh_component.get_collision_enabled())))
