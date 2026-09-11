"""Add the locally controlled roof/cutaway actor to FreighterDeck."""
import unreal

MAP = "/Game/Maps/FreighterDeck"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level(MAP)
if not any(actor.get_actor_label() == "INT_CutawayRoof" for actor in actors.get_all_level_actors()):
    cls = unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyInteriorCutaway")
    if not cls:
        raise RuntimeError("CalmyInteriorCutaway class is unavailable")
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(-1265, 0, 0))
    actor.set_actor_label("INT_CutawayRoof")
if not levels.save_current_level():
    raise RuntimeError("Could not save FreighterDeck cutaway roof")
unreal.log("CALMY_FREIGHTER_CUTAWAY_PASS roof_panels=6 local_visibility=1")
