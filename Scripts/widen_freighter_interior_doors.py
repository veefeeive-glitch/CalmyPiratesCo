"""Widen generated interior collision openings after live traversal verification."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Maps/FreighterDeck")
changed = 0
visuals_disabled = 0
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label == "Safety_Rail_End":
        actor.set_is_temporarily_hidden_in_editor(True)
        actor.set_actor_hidden_in_game(True)
        if isinstance(actor, unreal.StaticMeshActor):
            actor.static_mesh_component.set_collision_profile_name("NoCollision")
            actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label == "ART_Bridge" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_visibility(False)
        actor.static_mesh_component.set_hidden_in_game(True)
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label.startswith("INT_") and not label.startswith("INT_COLLISION_") and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        visuals_disabled += 1
    if label.startswith("INT_COLLISION_Entrance_") or label.startswith("INT_COLLISION_Bridge_"):
        sign = -1 if actor.get_actor_location().y < 0 else 1
        location = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(location.x, sign * 550, location.z), False, True)
        actor.set_actor_scale3d(unreal.Vector(0.14, 3.0, 2.7))
        changed += 1
if changed != 4:
    raise RuntimeError(f"Expected four doorway collision sections, changed {changed}")
if not levels.save_current_level():
    raise RuntimeError("Could not save widened interior doors")
unreal.log(f"CALMY_FREIGHTER_DOORS_PASS sections=4 clear_width=800cm visuals_no_collision={visuals_disabled}")
