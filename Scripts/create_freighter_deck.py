"""Create an art-dressed freighter deck without overwriting gameplay maps.

Imports the generated Blender kit plus a small, provenance-recorded CC0 support
set, duplicates LeakTest, retains its authoritative gameplay actors/colliders,
and adds a non-colliding visual shell aligned to the tested route.
"""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
TARGET = "/Game/Maps/FreighterDeck"
SOURCE = "/Game/Maps/LeakTest"
KIT_DEST = "/Game/Art/FreighterKit/Meshes"
KENNEY_DEST = "/Game/Art/ThirdParty/KenneyFactoryKit"
POLY_DEST = "/Game/Art/ThirdParty/PolyHaven"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_tools = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def import_file(filename, destination):
    source_stem = Path(filename).stem.lower().replace("-", "_")

    def discover_existing():
        candidates = []
        if unreal.EditorAssetLibrary.does_directory_exist(destination):
            for path in unreal.EditorAssetLibrary.list_assets(destination, recursive=True, include_folder=False):
                normalized = path.lower().replace("-", "_")
                if source_stem in normalized or destination.lower().replace("-", "_").endswith(source_stem):
                    obj = unreal.load_asset(path)
                    if obj:
                        candidates.append(obj)
        return candidates

    existing = discover_existing()
    if existing:
        return existing
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", False)
    asset_tools.import_asset_tasks([task])
    loaded = []
    for path in task.get_editor_property("imported_object_paths"):
        obj = unreal.load_asset(path)
        if obj:
            loaded.append(obj)
    if not loaded:
        loaded = discover_existing()
    if not loaded:
        raise RuntimeError(f"Import produced no assets: {filename}")
    return loaded


def first_static_mesh(objects, label):
    for obj in objects:
        if isinstance(obj, unreal.StaticMesh):
            return obj
    raise RuntimeError(f"No StaticMesh imported for {label}")


def first_texture(objects, label):
    for obj in objects:
        if isinstance(obj, unreal.Texture):
            return obj
    raise RuntimeError(f"No Texture imported for {label}")


def create_color_material(name, color, metallic, roughness, emissive=0.0):
    package = "/Game/Art/FreighterKit/Materials"
    path = f"{package}/{name}"
    existing = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if existing:
        return existing
    mat = asset_tools.create_asset(name, package, unreal.Material, unreal.MaterialFactoryNew())
    color_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionVectorParameter)
    color_node.set_editor_property("parameter_name", "Color")
    color_node.set_editor_property("default_value", unreal.LinearColor(*color, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(color_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    metal_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    metal_node.set_editor_property("parameter_name", "Metallic")
    metal_node.set_editor_property("default_value", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(metal_node, "", unreal.MaterialProperty.MP_METALLIC)
    rough_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionScalarParameter)
    rough_node.set_editor_property("parameter_name", "Roughness")
    rough_node.set_editor_property("default_value", roughness)
    unreal.MaterialEditingLibrary.connect_material_property(rough_node, "", unreal.MaterialProperty.MP_ROUGHNESS)
    if emissive > 0:
        emission = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply)
        emission.set_editor_property("const_b", emissive)
        unreal.MaterialEditingLibrary.connect_material_expressions(color_node, "", emission, "A")
        unreal.MaterialEditingLibrary.connect_material_property(emission, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def create_deck_material(diffuse, normal, arm):
    package = "/Game/Art/FreighterKit/Materials"
    path = f"{package}/M_DeckWeathered"
    existing = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if existing:
        return existing
    normal.set_editor_property("srgb", False)
    normal.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    arm.set_editor_property("srgb", False)
    unreal.EditorAssetLibrary.save_loaded_asset(normal)
    unreal.EditorAssetLibrary.save_loaded_asset(arm)
    mat = asset_tools.create_asset("M_DeckWeathered", package, unreal.Material, unreal.MaterialFactoryNew())
    uv = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate)
    uv.set_editor_property("u_tiling", 12.0)
    uv.set_editor_property("v_tiling", 7.0)
    diff_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureSample)
    diff_node.set_editor_property("texture", diffuse)
    normal_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureSample)
    normal_node.set_editor_property("texture", normal)
    normal_node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    arm_node = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionTextureSample)
    arm_node.set_editor_property("texture", arm)
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", diff_node, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", normal_node, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", arm_node, "Coordinates")
    unreal.MaterialEditingLibrary.connect_material_property(diff_node, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(normal_node, "RGB", unreal.MaterialProperty.MP_NORMAL)
    unreal.MaterialEditingLibrary.connect_material_property(arm_node, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    unreal.MaterialEditingLibrary.connect_material_property(arm_node, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(arm_node, "B", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


generated_dir = ROOT / "SourceAssets" / "Generated" / "FreighterKit"
generated_meshes = {}
for source in sorted(generated_dir.glob("SM_*.glb")):
    generated_meshes[source.stem] = first_static_mesh(import_file(source, KIT_DEST), source.stem)
if len(generated_meshes) != 12:
    raise RuntimeError(f"Expected 12 generated meshes, imported {len(generated_meshes)}")

kenney_dir = ROOT / "SourceAssets" / "ThirdParty" / "KenneyFactoryKit_3.0" / "extracted" / "Models" / "GLB format"
kenney_sources = {
    "crane": kenney_dir / "crane.glb",
    "pipe_valve": kenney_dir / "pipe-large-valve.glb",
    "machine": kenney_dir / "machine-fortified.glb",
    "screen": kenney_dir / "screen-wide.glb",
    "warning": kenney_dir / "warning-orange.glb",
}
kenney_meshes = {name: first_static_mesh(import_file(path, f"{KENNEY_DEST}/{name}"), name) for name, path in kenney_sources.items()}

barrel_dir = ROOT / "SourceAssets" / "ThirdParty" / "PolyHaven" / "barrel_03"
barrel_mesh = first_static_mesh(import_file(barrel_dir / "barrel_03_1k.gltf", f"{POLY_DEST}/Barrel03"), "barrel_03")
plate_dir = ROOT / "SourceAssets" / "ThirdParty" / "PolyHaven" / "metal_plate"
plate_diff = first_texture(import_file(plate_dir / "metal_plate_diff_1k.png", f"{POLY_DEST}/MetalPlate"), "metal_plate diffuse")
plate_normal = first_texture(import_file(plate_dir / "metal_plate_nor_dx_1k.png", f"{POLY_DEST}/MetalPlate"), "metal_plate normal")
plate_arm = first_texture(import_file(plate_dir / "metal_plate_arm_1k.png", f"{POLY_DEST}/MetalPlate"), "metal_plate arm")

deck_material = create_deck_material(plate_diff, plate_normal, plate_arm)
safety_material = create_color_material("M_SafetyReadability", (1.0, 0.20, 0.015), 0.25, 0.32, 0.7)
route_material = create_color_material("M_RouteReadability", (0.02, 0.52, 0.66), 0.25, 0.30, 0.35)
sea_material = create_color_material("M_SeaArt", (0.006, 0.055, 0.085), 0.35, 0.18)

if not unreal.EditorAssetLibrary.does_asset_exist(TARGET):
    if not unreal.EditorAssetLibrary.duplicate_asset(SOURCE, TARGET):
        raise RuntimeError(f"Could not duplicate {SOURCE} to {TARGET}")
    unreal.EditorAssetLibrary.save_asset(TARGET, only_if_is_dirty=False)
    unreal.SystemLibrary.collect_garbage()
levels.load_level(TARGET)

if any(actor.get_actor_label().startswith("ART_") for actor in actor_tools.get_all_level_actors()):
    raise RuntimeError(f"Refusing to duplicate an existing art pass in {TARGET}")


def hide_blockout(actor):
    label = actor.get_actor_label()
    prefixes = (
        "Freighter_Deck",
        "Hull",
        "Safety_Rail",
        "Route_Equipment_Block",
        "Open_Gangway_Rail",
        "Gangway_Hazard_Edge",
        "Deck_Lane_Marking",
        "Cabin_Blockout",
    )
    if label.startswith(prefixes) and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_visibility(False)
        actor.static_mesh_component.set_hidden_in_game(True)
    if label == "Sea_Greybox" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_material(0, sea_material)


for existing_actor in actor_tools.get_all_level_actors():
    hide_blockout(existing_actor)


def place(name, mesh, location, rotation=(0, 0, 0), scale=(1, 1, 1), material=None):
    actor = actor_tools.spawn_actor_from_object(mesh, unreal.Vector(*location), unreal.Rotator(*rotation))
    if not actor:
        raise RuntimeError(f"Could not spawn {name}")
    actor.set_actor_label(name)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    component = actor.static_mesh_component
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if material:
        component.set_material(0, material)
    return actor


place("ART_MainDeck", generated_meshes["SM_MainDeck"], (0, 0, 1), material=deck_material)
place("ART_FreighterHull", generated_meshes["SM_FreighterHull"], (0, 0, 0))
place("ART_Bridge", generated_meshes["SM_Bridge"], (-1450, 0, 0))
place("ART_ExhaustStack", generated_meshes["SM_ExhaustStack"], (-1450, 0, 343))
place("ART_DeckContainer_Chokepoint", generated_meshes["SM_CargoContainer"], (0, 80, 0), rotation=(0, 90, 0))
place("ART_DeckCrane", generated_meshes["SM_DeckCrane"], (900, 690, 0), rotation=(0, 180, 0), scale=(0.78, 0.78, 0.78))
place("ART_DeckWinch_Port", generated_meshes["SM_DeckWinch"], (650, 610, 0), rotation=(0, 90, 0), scale=(0.8, 0.8, 0.8))
place("ART_DeckWinch_Starboard", generated_meshes["SM_DeckWinch"], (650, -610, 0), rotation=(0, 90, 0), scale=(0.8, 0.8, 0.8))
for index, pos in enumerate(((-1050, -660, 0), (-1050, 660, 0), (1180, -650, 0), (1180, 650, 0))):
    place(f"ART_Bollard_{index}", generated_meshes["SM_Bollard"], pos, scale=(0.8, 0.8, 0.8))
for index, pos in enumerate(((-1030, -470, 0), (-1030, 470, 0), (-1180, -470, 0), (-1180, 470, 0))):
    place(f"ART_Vent_{index}", generated_meshes["SM_DeckVent"], pos, scale=(0.72, 0.72, 0.72))
place("ART_Hatch", generated_meshes["SM_DeckHatch"], (-560, 610, 0), rotation=(0, 90, 0), scale=(0.8, 0.8, 0.8))
place("ART_Lifebuoy_Port", generated_meshes["SM_Lifebuoy"], (-1420, 451, 170), rotation=(0, 0, 0), scale=(0.78, 0.78, 0.78))
place("ART_Lifebuoy_Starboard", generated_meshes["SM_Lifebuoy"], (-1420, -451, 170), rotation=(0, 180, 0), scale=(0.78, 0.78, 0.78))

for side in (-1, 1):
    for index, x in enumerate((-1200, -800, -400, 0, 400, 800, 1200)):
        if side < 0 and x in (-400, 0):
            continue
        place(f"ART_Rail_{'Starboard' if side < 0 else 'Port'}_{index}", generated_meshes["SM_Railing4m"], (x, side * 950, 0))
place("ART_Rail_Stern", generated_meshes["SM_Railing4m"], (-1740, -600, 0), rotation=(0, 90, 0))
place("ART_Rail_Stern2", generated_meshes["SM_Railing4m"], (-1740, -200, 0), rotation=(0, 90, 0))
place("ART_Rail_Stern3", generated_meshes["SM_Railing4m"], (-1740, 200, 0), rotation=(0, 90, 0))
place("ART_Rail_Stern4", generated_meshes["SM_Railing4m"], (-1740, 600, 0), rotation=(0, 90, 0))

place("ART_KenneyMachine", kenney_meshes["machine"], (860, -690, 0), scale=(1.4, 1.4, 1.4))
place("ART_KenneyPipeValve", kenney_meshes["pipe_valve"], (-420, 515, 45), rotation=(0, 90, 0), scale=(1.25, 1.25, 1.25))
place("ART_KenneyScreen", kenney_meshes["screen"], (625, -420, 120), rotation=(0, -90, 0), scale=(1.35, 1.35, 1.35))
place("ART_KenneyWarning", kenney_meshes["warning"], (380, -680, 0), scale=(1.3, 1.3, 1.3))
place("ART_KenneyCraneDetail", kenney_meshes["crane"], (1380, 480, 0), rotation=(0, 180, 0), scale=(1.1, 1.1, 1.1))
for index, pos in enumerate(((980, -720, 0), (1120, -720, 0), (1260, -720, 0))):
    place(f"ART_PolyHavenBarrel_{index}", barrel_mesh, pos)

cube = unreal.load_asset("/Engine/BasicShapes/Cube")
for index, x in enumerate(range(-1150, 1251, 300)):
    place(f"ART_RouteMarker_{index}", cube, (x, -325, 3), scale=(1.0, 0.05, 0.025), material=route_material)
for index, x in enumerate(range(-1450, 1451, 200)):
    place(f"ART_EdgeMarker_Port_{index}", cube, (x, 860, 3), scale=(0.72, 0.045, 0.025), material=safety_material)
    if x < -500 or x > 300:
        place(f"ART_EdgeMarker_Starboard_{index}", cube, (x, -860, 3), scale=(0.72, 0.045, 0.025), material=safety_material)

for index, (x, y) in enumerate(((-900, -690), (-900, 690), (0, -690), (0, 690), (900, -690), (900, 690))):
    light = actor_tools.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, 380))
    light.set_actor_label(f"ART_WorkLight_{index}")
    light.light_component.set_editor_property("intensity", 1800.0)
    light.light_component.set_editor_property("attenuation_radius", 720.0)
    light.light_component.set_editor_property("light_color", unreal.Color(255, 198, 135, 255))
    light.light_component.set_editor_property("cast_shadows", True)

for actor in actor_tools.get_all_level_actors():
    if isinstance(actor, unreal.DirectionalLight):
        actor.set_actor_rotation(unreal.Rotator(-32, -28, 0), False)
        actor.light_component.set_editor_property("intensity", 3.2)
    elif isinstance(actor, unreal.SkyLight):
        actor.light_component.set_editor_property("intensity", 0.8)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyGameMode"))
if not levels.save_current_level():
    raise RuntimeError("Could not save FreighterDeck")
unreal.log(
    "CALMY_FREIGHTER_DECK_PASS "
    f"generated_meshes={len(generated_meshes)} kenney_meshes={len(kenney_meshes)} "
    f"art_actors={sum(1 for actor in actor_tools.get_all_level_actors() if actor.get_actor_label().startswith('ART_'))}"
)
