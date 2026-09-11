"""Final spatial/readability pass for the generated FreighterDeck interior."""
import unreal

MAP = "/Game/Maps/FreighterDeck"
PACKAGE = "/Game/Art/FreighterInterior/Materials"
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


bridge_mat = color_material("M_BridgeZone", (0.025, 0.12, 0.16), 0.55, 0.42)
engine_mat = color_material("M_EngineZone", (0.20, 0.055, 0.018), 0.62, 0.48)
crew_mat = color_material("M_CrewZone", (0.055, 0.10, 0.13), 0.38, 0.58)
safety_mat = color_material("M_InteriorSafetyLine", (0.95, 0.15, 0.01), 0.25, 0.32, 0.45)

all_actors = actors.get_all_level_actors()
for actor in all_actors:
    label = actor.get_actor_label()
    if label == "ART_ExhaustStack":
        actor.set_actor_location(unreal.Vector(-1480, 820, 0), False, True)
    elif isinstance(actor, unreal.PointLight) and label.startswith("INT_WarmLight_"):
        actor.light_component.set_editor_property("intensity", 420.0)

cube = unreal.load_asset("/Engine/BasicShapes/Cube")


def pad(label, location, dimensions, material):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(dimensions[0] / 100, dimensions[1] / 100, dimensions[2] / 100))
    actor.static_mesh_component.set_material(0, material)
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


if not any(actor.get_actor_label() == "INT_ZONE_Bridge" for actor in all_actors):
    pad("INT_ZONE_Bridge", (-1535, 0, 12), (375, 1240, 2), bridge_mat)
    pad("INT_ZONE_Engine", (-1065, 445, 12), (500, 450, 2), engine_mat)
    pad("INT_ZONE_Crew", (-1065, -445, 12), (500, 450, 2), crew_mat)
    pad("INT_ZONE_Corridor", (-1065, 0, 13), (500, 175, 2), crew_mat)
    for index, y in enumerate((-150, 150)):
        pad(f"INT_ZONE_EntranceStripe_{index}", (-790, y, 14), (28, 110, 3), safety_mat)
    for index, x in enumerate((-1280, -1180, -980, -880)):
        pad(f"INT_ZONE_CorridorStripe_{index}", (x, 0, 14), (45, 8, 3), safety_mat)

unreal.EditorAssetLibrary.save_directory("/Game/Art/FreighterInterior", only_if_is_dirty=False, recursive=True)
if not levels.save_current_level():
    raise RuntimeError("Could not save interior polish")
unreal.log("CALMY_FREIGHTER_INTERIOR_POLISH_PASS stack=exterior lights=420 zones=4")
