"""Read-only collision audit around the generated interior entrance."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Maps/FreighterDeck")
for actor in actors.get_all_level_actors():
    if not isinstance(actor, unreal.StaticMeshActor):
        continue
    component = actor.static_mesh_component
    if component.get_collision_enabled() == unreal.CollisionEnabled.NO_COLLISION:
        continue
    origin, extent = actor.get_actor_bounds(False, False)
    crosses_stair = origin.x + extent.x >= -930 and origin.x - extent.x <= -700 and origin.y + extent.y >= -50 and origin.y - extent.y <= 50 and origin.z + extent.z >= -270 and origin.z - extent.z <= -80
    if crosses_stair:
        unreal.log(f"CALMY_COLLISION_AUDIT label={actor.get_actor_label()} origin={origin} extent={extent} scale={actor.get_actor_scale3d()}")
unreal.log("CALMY_COLLISION_AUDIT_PASS")
