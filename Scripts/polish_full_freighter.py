"""Apply readable industrial materials and zoning to the full freighter."""
import unreal

MAP = "/Game/Maps/FreighterDeck"
PACKAGE = "/Game/Art/FullFreighter/Materials"
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets = unreal.AssetToolsHelpers.get_asset_tools()
levels.load_level(MAP)


def color_material(name, rgb, metallic, roughness, emissive=0.0):
    path = f"{PACKAGE}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    mat = assets.create_asset(name, PACKAGE, unreal.Material, unreal.MaterialFactoryNew())
    color = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(*rgb, 1))
    unreal.MaterialEditingLibrary.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    metal = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    metal.set_editor_property("parameter_name", "Metallic")
    metal.set_editor_property("default_value", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(metal, "", unreal.MaterialProperty.MP_METALLIC)
    rough = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    rough.set_editor_property("parameter_name", "Roughness")
    rough.set_editor_property("default_value", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive:
        multiply = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply)
        multiply.set_editor_property("const_b", emissive)
        unreal.MaterialEditingLibrary.connect_material_expressions(color, "", multiply, "A")
        unreal.MaterialEditingLibrary.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


deck = color_material("M_LowerDeck", (0.018, 0.042, 0.052), 0.72, 0.44)
wall = color_material("M_LowerBulkhead", (0.11, 0.145, 0.15), 0.58, 0.52)
engine = color_material("M_EngineZone", (0.20, 0.045, 0.012), 0.55, 0.48)
crew = color_material("M_CrewZone", (0.018, 0.10, 0.12), 0.48, 0.54)
cargo = color_material("M_CargoZone", (0.025, 0.075, 0.15), 0.52, 0.47)
route = color_material("M_LowerRoute", (0.92, 0.16, 0.012), 0.28, 0.34, 0.35)

all_actors = actors.get_all_level_actors()
if any(actor.get_actor_label().startswith("FS_ZONE_") for actor in all_actors):
    raise RuntimeError("Refusing to duplicate full-freighter polish actors")

for actor in all_actors:
    label = actor.get_actor_label()
    if not isinstance(actor, unreal.StaticMeshActor):
        if isinstance(actor, unreal.PointLight) and label.startswith("FS_WorkLight_"):
            actor.light_component.set_editor_property("intensity", 280.0)
            actor.light_component.set_editor_property("attenuation_radius", 360.0)
        continue
    if label == "FS_LowerDeckFloor":
        actor.static_mesh_component.set_material(0, deck)
    elif label.startswith("FS_LowerWall_") or "Bulkhead" in label or label == "FS_StairSafetyFrame":
        actor.static_mesh_component.set_material(0, wall)
    elif label == "FS_MainDeckWithOpenings":
        existing_deck = unreal.load_asset("/Game/Art/FreighterKit/Materials/M_DeckWeathered")
        if existing_deck:
            actor.static_mesh_component.set_material(0, existing_deck)

cube = unreal.load_asset("/Engine/BasicShapes/Cube")


def pad(label, location, dimensions, material):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(dimensions[0] / 100, dimensions[1] / 100, dimensions[2] / 100))
    actor.static_mesh_component.set_material(0, material)
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


pad("FS_ZONE_EngineRoom", (-1200, 0, -268), (880, 1180, 2), engine)
pad("FS_ZONE_CrewServices", (-300, 0, -267), (880, 1180, 2), crew)
pad("FS_ZONE_CargoHold", (650, 0, -266), (980, 1180, 2), cargo)
pad("FS_ZONE_CentralRoute", (-250, 0, -264), (2760, 120, 3), deck)
for index, x in enumerate((-760, 140)):
    for side in (-1, 1):
        pad(f"FS_ZONE_BulkheadStripe_{index}_{side}", (x, side * 92, -262), (28, 42, 4), route)
for index, x in enumerate(range(-1450, 1051, 250)):
    pad(f"FS_ZONE_RouteDash_{index}", (x, 0, -260), (80, 7, 4), route)

unreal.EditorAssetLibrary.save_directory("/Game/Art/FullFreighter", only_if_is_dirty=False, recursive=True)
if not levels.save_current_level():
    raise RuntimeError("Could not save full-freighter polish")
unreal.log("CALMY_FULL_FREIGHTER_POLISH_PASS zones=3 route=1 lighting=industrial")
