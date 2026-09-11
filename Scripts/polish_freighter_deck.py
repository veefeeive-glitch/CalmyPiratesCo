"""Apply a fixed exposure baseline and persist renderer usage flags."""
import unreal

MAP = "/Game/Maps/FreighterDeck"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels.load_level(MAP)

deck_material = unreal.load_asset("/Game/Art/FreighterKit/Meshes/SM_MainDeck/Materials/MAT_Deck_BlueGrey")
if not deck_material:
    raise RuntimeError("Could not load the generated deck material")

for actor in actors.get_all_level_actors():
    label = actor.get_actor_label()
    if isinstance(actor, unreal.DirectionalLight):
        actor.light_component.set_editor_property("intensity", 5.0)
    elif isinstance(actor, unreal.SkyLight):
        actor.light_component.set_editor_property("intensity", 1.0)
    elif isinstance(actor, unreal.PointLight) and label.startswith("ART_WorkLight_"):
        actor.light_component.set_editor_property("intensity", 850.0)
    if label == "ART_MainDeck" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_material(0, deck_material)

post = next((actor for actor in actors.get_all_level_actors() if actor.get_actor_label() == "ART_ExposureBaseline"), None)
if not post:
    post = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    post.set_actor_label("ART_ExposureBaseline")
post.set_editor_property("unbound", True)
settings = post.get_editor_property("settings")
settings.set_editor_property("override_auto_exposure_min_brightness", True)
settings.set_editor_property("override_auto_exposure_max_brightness", True)
settings.set_editor_property("override_auto_exposure_bias", True)
settings.set_editor_property("auto_exposure_min_brightness", 3.5)
settings.set_editor_property("auto_exposure_max_brightness", 3.5)
settings.set_editor_property("auto_exposure_bias", 0.0)
post.set_editor_property("settings", settings)

usage = unreal.MaterialUsage.MATUSAGE_NANITE
for path in unreal.EditorAssetLibrary.list_assets("/Game/Art", recursive=True, include_folder=False):
    asset = unreal.load_asset(path)
    if isinstance(asset, unreal.Material):
        unreal.MaterialEditingLibrary.set_base_material_usage(asset, usage, True)
    elif isinstance(asset, unreal.MaterialInstanceConstant):
        unreal.MaterialEditingLibrary.set_material_usage_override(asset, usage, True, True)
unreal.EditorAssetLibrary.save_directory("/Game/Art", only_if_is_dirty=False, recursive=True)
if not levels.save_current_level():
    raise RuntimeError("Could not save FreighterDeck lighting polish")
unreal.log("CALMY_FREIGHTER_POLISH_PASS fixed_ev100=3.5 work_lights=850 nanite_materials=ready")
