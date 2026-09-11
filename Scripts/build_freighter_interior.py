"""Import the Blender interior kit and assemble a playable cutaway interior.

Only generated INT_ actors are added. Existing gameplay actors and collision are
preserved; the old closed bridge art shell is hidden, never deleted.
"""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "SourceAssets" / "Generated" / "FreighterInterior"
DEST = "/Game/Art/FreighterInterior/Meshes"
MAP = "/Game/Maps/FreighterDeck"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def import_mesh(path):
    stem = path.stem.lower()
    if unreal.EditorAssetLibrary.does_directory_exist(DEST):
        for asset_path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True, include_folder=False):
            if stem in asset_path.lower():
                asset = unreal.load_asset(asset_path)
                if isinstance(asset, unreal.StaticMesh):
                    return asset
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", False)
    asset_tools.import_asset_tasks([task])
    for imported in task.get_editor_property("imported_object_paths"):
        asset = unreal.load_asset(imported)
        if isinstance(asset, unreal.StaticMesh):
            return asset
    raise RuntimeError(f"No StaticMesh imported for {path}")


meshes = {path.stem: import_mesh(path) for path in sorted(SOURCE.glob("SM_*.glb"))}
if len(meshes) != 21:
    raise RuntimeError(f"Expected 21 interior meshes, found {len(meshes)}")

levels.load_level(MAP)
existing = actors.get_all_level_actors()
if any(actor.get_actor_label().startswith("INT_") for actor in existing):
    raise RuntimeError("Refusing to duplicate an existing generated interior")

for actor in existing:
    if actor.get_actor_label() == "Safety_Rail_End":
        actor.set_is_temporarily_hidden_in_editor(True)
        actor.set_actor_hidden_in_game(True)
        if isinstance(actor, unreal.StaticMeshActor):
            actor.static_mesh_component.set_collision_profile_name("NoCollision")
            actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if actor.get_actor_label() == "ART_Bridge" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_visibility(False)
        actor.static_mesh_component.set_hidden_in_game(True)
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)


def place(label, mesh_name, location, rotation=(0, 0, 0), scale=(1, 1, 1)):
    actor = actors.spawn_actor_from_object(meshes[mesh_name], unreal.Vector(*location), unreal.Rotator(*rotation))
    if not actor:
        raise RuntimeError(f"Could not place {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return actor


cube = unreal.load_asset("/Engine/BasicShapes/Cube")


def collision_box(label, location, dimensions, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(dimensions[0] / 100, dimensions[1] / 100, dimensions[2] / 100))
    actor.static_mesh_component.set_visibility(False)
    actor.static_mesh_component.set_hidden_in_game(True)
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    return actor


# 9.3 m x 14 m cutaway superstructure, open above for the current elevated camera.
place("INT_Floor", "SM_InteriorFloor8x8", (-1265, 0, 8), scale=(1.16, 1.75, 1))
place("INT_Wall_Port", "SM_InteriorWall4m", (-1265, 700, 0), scale=(2.32, 1, 1))
place("INT_Wall_Starboard", "SM_InteriorWall4m", (-1265, -700, 0), scale=(2.32, 1, 1))
place("INT_Wall_Aft", "SM_InteriorWall4m", (-1730, 0, 0), rotation=(0, 90, 0), scale=(3.5, 1, 1))
place("INT_EntranceFrame", "SM_BulkheadDoorFrame", (-800, 0, 0), rotation=(0, 90, 0), scale=(1.0, 1, 1))
place("INT_EntrancePortWall", "SM_InteriorWall4m", (-800, 445, 0), rotation=(0, 90, 0), scale=(1.25, 1, 1))
place("INT_EntranceStarboardWall", "SM_InteriorWall4m", (-800, -445, 0), rotation=(0, 90, 0), scale=(1.25, 1, 1))

# Bridge is aft; two technical/crew rooms flank a central access corridor.
place("INT_BridgeDoorFrame", "SM_BulkheadDoorFrame", (-1340, 0, 0), rotation=(0, 90, 0), scale=(1.0, 1, 1))
place("INT_BridgeWallPort", "SM_InteriorWall4m", (-1340, 445, 0), rotation=(0, 90, 0), scale=(1.25, 1, 1))
place("INT_BridgeWallStarboard", "SM_InteriorWall4m", (-1340, -445, 0), rotation=(0, 90, 0), scale=(1.25, 1, 1))
place("INT_EngineDoorFrame", "SM_BulkheadDoorFrame", (-1070, 190, 0), scale=(1.35, 1, 1))
place("INT_CrewDoorFrame", "SM_BulkheadDoorFrame", (-1070, -190, 0), rotation=(0, 180, 0), scale=(1.35, 1, 1))

# Bridge equipment.
place("INT_BridgeConsole", "SM_BridgeConsole", (-1590, 0, 0), rotation=(0, 90, 0))
place("INT_Helm", "SM_HelmStation", (-1450, 0, 0), rotation=(0, 90, 0))
place("INT_NavigationTable_Port", "SM_NavigationTable", (-1540, 390, 0), rotation=(0, 90, 0))
place("INT_NavigationTable_Starboard", "SM_NavigationTable", (-1540, -390, 0), rotation=(0, 90, 0))
place("INT_Locker_Bridge", "SM_LockerBank", (-1680, -420, 0), rotation=(0, 90, 0))

# Port machinery room.
place("INT_Generator_A", "SM_EngineGenerator", (-1160, 470, 0), rotation=(0, 90, 0), scale=(0.9, 0.9, 0.9))
place("INT_Generator_B", "SM_EngineGenerator", (-860, 470, 0), rotation=(0, 90, 0), scale=(0.78, 0.78, 0.78))
place("INT_Manifold", "SM_EngineManifold", (-1260, 655, 0))
place("INT_PipeRun", "SM_PipeRun4m", (-1010, 655, 0), scale=(1.05, 1, 1))
place("INT_Workbench", "SM_Workbench", (-1210, 470, 0), rotation=(0, -90, 0), scale=(0.78, 0.78, 0.78))
place("INT_Junction_Engine", "SM_JunctionBox", (-820, 650, 35), rotation=(0, 0, 0))

# Starboard crew service room.
place("INT_Bunks", "SM_BunkPair", (-1150, -500, 0), rotation=(0, 90, 0), scale=(0.92, 0.92, 0.92))
place("INT_Galley", "SM_GalleyUnit", (-900, -545, 0), rotation=(0, 180, 0), scale=(0.88, 0.88, 0.88))
place("INT_Locker_Crew", "SM_LockerBank", (-1260, -650, 0), scale=(0.78, 0.78, 0.78))
place("INT_CargoRack", "SM_CargoRack", (-850, -360, 0), rotation=(0, -90, 0), scale=(0.82, 0.82, 0.82))
place("INT_Junction_Crew", "SM_JunctionBox", (-820, -650, 35), rotation=(0, 180, 0))

# Ship systems and access detail.
place("INT_Ladder_Aft", "SM_Ladder", (-1690, 520, 5), rotation=(0, 90, 0))
place("INT_Stairs_ToUpperDeck", "SM_StairFlight", (-1470, 575, 0), rotation=(0, 180, 0), scale=(0.75, 0.75, 0.75))
place("INT_Duct_Port", "SM_VentDuct", (-1120, 620, 238), scale=(1.25, 1, 1))
place("INT_Duct_Starboard", "SM_VentDuct", (-1120, -620, 238), scale=(1.25, 1, 1))
place("INT_WatertightDoor_Open", "SM_WatertightDoor", (-805, 105, 0), rotation=(0, 15, 0))

for index, (x, y) in enumerate(((-1560, 0), (-1110, 480), (-900, 480), (-1110, -480), (-900, -480), (-920, 0))):
    place(f"INT_CeilingLight_{index}", "SM_CeilingLight", (x, y, 255))
    light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, 225))
    light.set_actor_label(f"INT_WarmLight_{index}")
    light.light_component.set_editor_property("intensity", 700.0)
    light.light_component.set_editor_property("attenuation_radius", 430.0)
    light.light_component.set_editor_property("light_color", unreal.Color(255, 202, 145, 255))
    light.light_component.set_editor_property("cast_shadows", True)

# Deterministic invisible simple collision: door openings remain at least 1.4 m wide.
collision_box("INT_COLLISION_PortWall", (-1265, 700, 135), (930, 14, 270))
collision_box("INT_COLLISION_StarboardWall", (-1265, -700, 135), (930, 14, 270))
collision_box("INT_COLLISION_AftWall", (-1730, 0, 135), (14, 1400, 270))
for y in (-550, 550):
    collision_box(f"INT_COLLISION_Entrance_{y}", (-800, y, 135), (14, 300, 270))
    collision_box(f"INT_COLLISION_Bridge_{y}", (-1340, y, 135), (14, 300, 270))
for y in (-190, 190):
    collision_box(f"INT_COLLISION_PartitionA_{y}", (-1255, y, 135), (170, 14, 270))
    collision_box(f"INT_COLLISION_PartitionB_{y}", (-885, y, 135), (170, 14, 270))

for label, loc, dims, rot in (
    ("BridgeConsole", (-1590, 0, 65), (90, 290, 130), (0, 0, 0)),
    ("NavPort", (-1540, 390, 55), (110, 180, 110), (0, 0, 0)),
    ("NavStarboard", (-1540, -390, 55), (110, 180, 110), (0, 0, 0)),
    ("GeneratorA", (-1160, 470, 75), (120, 250, 150), (0, 0, 0)),
    ("GeneratorB", (-860, 470, 65), (105, 210, 130), (0, 0, 0)),
    ("Bunks", (-1150, -500, 100), (100, 200, 200), (0, 0, 0)),
    ("Galley", (-900, -545, 70), (215, 80, 140), (0, 0, 0)),
    ("CargoRack", (-850, -360, 100), (80, 205, 200), (0, 0, 0)),
):
    collision_box(f"INT_COLLISION_{label}", loc, dims, rot)

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyGameMode"))
unreal.EditorAssetLibrary.save_directory("/Game/Art/FreighterInterior", only_if_is_dirty=False, recursive=True)
if not levels.save_current_level():
    raise RuntimeError("Could not save FreighterDeck interior")

interior_count = sum(1 for actor in actors.get_all_level_actors() if actor.get_actor_label().startswith("INT_"))
collision_count = sum(1 for actor in actors.get_all_level_actors() if actor.get_actor_label().startswith("INT_COLLISION_"))
unreal.log(f"CALMY_FREIGHTER_INTERIOR_PASS meshes={len(meshes)} actors={interior_count} collision={collision_count}")
