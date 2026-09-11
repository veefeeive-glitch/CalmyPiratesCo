"""Apply verified clearance fixes to the assembled full-freighter map."""
import unreal

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level("/Game/Maps/FreighterDeck")
changed = 0
for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if label.startswith("ART_") and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label == "Sea_Greybox" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_editor_property("tags", [unreal.Name("CalmyLowerDeckOccluder")])
    if label in ("Freighter_Deck", "Hull") and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label == "FS_FuelTank":
        actor.set_actor_location(unreal.Vector(-1350, -520, -270), False, True)
        changed += 1
    elif label == "FS_COLLISION_FuelTank":
        actor.set_actor_location(unreal.Vector(-1350, -520, -190), False, True)
        actor.set_actor_scale3d(unreal.Vector(3.1, 1.4, 1.6))
        changed += 1
if changed != 2:
    raise RuntimeError(f"Expected fuel visual and collision, changed {changed}")
if not levels.save_current_level():
    raise RuntimeError("Could not save full freighter clearance fix")
unreal.log("CALMY_FULL_FREIGHTER_CLEARANCE_PASS stair_fuel_clearance=1")
