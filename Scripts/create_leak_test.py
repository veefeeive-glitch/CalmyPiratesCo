"""Opt-in leak/repair drill; preserve all existing maps and edits."""
import unreal

TARGET = "/Game/Maps/LeakTest"
if unreal.EditorAssetLibrary.does_asset_exist(TARGET):
    unreal.log("LeakTest exists; preserving edits.")
else:
    if not unreal.EditorAssetLibrary.duplicate_asset("/Game/Maps/ShiftTest", TARGET):
        raise RuntimeError("Could not duplicate ShiftTest")
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    levels.load_level(TARGET)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    valve = actors.spawn_actor_from_class(unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyLeak"), unreal.Vector(-430, 340, 50))
    valve.set_actor_label("LEAK_REPAIR_VALVE_HOLD_F")
    if not levels.save_current_level():
        raise RuntimeError("Could not save LeakTest")
    unreal.log("CALMY_LEAK_TEST_CREATED " + TARGET)
