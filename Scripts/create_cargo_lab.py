"""Create a separate cooperative cargo-behavior map using Unreal asset APIs."""
import unreal

TARGET = "/Game/Maps/CargoLab"
if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
    unreal.log("CargoLab already exists; preserving edits.")
else:
    if not unreal.EditorAssetLibrary.duplicate_asset("/Game/Maps/ShiftTest", TARGET):
        raise RuntimeError("Could not duplicate ShiftTest into CargoLab")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(TARGET)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    kinds = {1: unreal.CalmyCargoKind.BULKY, 2: unreal.CalmyCargoKind.UNSTABLE, 3: unreal.CalmyCargoKind.FRAGILE}
    locations = {1: (-400, 220, 50), 2: (-400, -50, 45), 3: (-400, -280, 45)}
    for actor in actors.get_all_level_actors():
        if actor.get_class().get_name() == "CalmyCargo":
            cargo_id = actor.get_editor_property("cargo_id")
            actor.set_editor_property("kind", kinds[cargo_id])
            actor.set_actor_label("Cargo_" + kinds[cargo_id].name)
            actor.set_actor_location(unreal.Vector(*locations[cargo_id]), False, True)
            actor.set_actor_scale3d(unreal.Vector(1.15, .9, .75) if cargo_id == 1 else unreal.Vector(.65, .65, .65))
    levels.save_current_level()
    unreal.log("CALMY_CARGO_LAB_CREATED " + TARGET)
