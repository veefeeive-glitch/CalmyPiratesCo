"""Import and assemble the traversable below-deck freighter extension."""
from pathlib import Path
import unreal

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "SourceAssets" / "Generated" / "FullFreighter"
DEST = "/Game/Art/FullFreighter/Meshes"
MAP = "/Game/Maps/FreighterDeck"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def import_mesh(path):
    stem = path.stem.lower()
    if unreal.EditorAssetLibrary.does_directory_exist(DEST):
        for asset_path in unreal.EditorAssetLibrary.list_assets(DEST, recursive=True, include_folder=False):
            if stem in asset_path.lower():
                loaded = unreal.load_asset(asset_path)
                if isinstance(loaded, unreal.StaticMesh):
                    return loaded
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(path))
    task.set_editor_property("destination_path", DEST)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", False)
    asset_tools.import_asset_tasks([task])
    for imported in task.get_editor_property("imported_object_paths"):
        loaded = unreal.load_asset(imported)
        if isinstance(loaded, unreal.StaticMesh):
            return loaded
    raise RuntimeError(f"No StaticMesh imported for {path}")


def find_mesh(directory, stem):
    for path in unreal.EditorAssetLibrary.list_assets(directory, recursive=True, include_folder=False):
        if stem.lower() in path.lower():
            loaded = unreal.load_asset(path)
            if isinstance(loaded, unreal.StaticMesh):
                return loaded
    raise RuntimeError(f"Missing existing mesh {stem}")


meshes = {path.stem: import_mesh(path) for path in sorted(SOURCE.glob("SM_*.glb"))}
if len(meshes) != 12:
    raise RuntimeError(f"Expected 12 full-ship meshes, found {len(meshes)}")
interior_dir = "/Game/Art/FreighterInterior/Meshes"
for name in ("SM_InteriorWall4m", "SM_BulkheadDoorFrame", "SM_StairFlight", "SM_CargoRack",
             "SM_BunkPair", "SM_LockerBank", "SM_Workbench", "SM_PipeRun4m", "SM_CeilingLight"):
    meshes[name] = find_mesh(interior_dir, name)

levels.load_level(MAP)
existing = actors.get_all_level_actors()
if any(actor.get_actor_label().startswith("FS_") for actor in existing):
    raise RuntimeError("Refusing to duplicate the full freighter assembly")

# Replace the single uncut deck visual/physics with a deck that has a real
# stairwell and gangway. Original actors remain available but inactive.
for actor in existing:
    label = actor.get_actor_label()
    if label.startswith("ART_") and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label in ("ART_MainDeck", "Freighter_Deck", "Hull", "INT_Floor", "INT_ZONE_Crew") and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_visibility(False)
        actor.static_mesh_component.set_hidden_in_game(True)
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if label == "Sea_Greybox" and isinstance(actor, unreal.StaticMeshActor):
        actor.static_mesh_component.set_collision_profile_name("NoCollision")
        actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_editor_property("tags", [unreal.Name("CalmyLowerDeckOccluder")])


def place(label, mesh_name, location, rotation=(0, 0, 0), scale=(1, 1, 1), occluder=False):
    actor = actors.spawn_actor_from_object(meshes[mesh_name], unreal.Vector(*location), unreal.Rotator(*rotation))
    if not actor:
        raise RuntimeError(f"Could not place {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if occluder:
        actor.set_editor_property("tags", [unreal.Name("CalmyLowerDeckOccluder")])
    return actor


cube = unreal.load_asset("/Engine/BasicShapes/Cube")


def collision_box(label, location, dimensions, rotation=(0, 0, 0)):
    actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location), unreal.Rotator(*rotation))
    actor.set_actor_label(label)
    actor.static_mesh_component.set_static_mesh(cube)
    actor.set_actor_scale3d(unreal.Vector(dimensions[0] / 100, dimensions[1] / 100, dimensions[2] / 100))
    actor.static_mesh_component.set_visibility(False)
    actor.static_mesh_component.set_hidden_in_game(True)
    actor.static_mesh_component.set_collision_profile_name("BlockAll")
    actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)
    return actor


# Whole weather deck, including true stairwell and open-starboard gangway holes.
place("FS_MainDeckWithOpenings", "SM_MainDeckFull", (0, 0, 1), occluder=True)
for index, (loc, dims) in enumerate((
    ((-1495, 0, -5), (510, 1900, 10)),
    ((-1080, -690, -5), (320, 520, 10)),
    ((-1080, 340, -5), (320, 1220, 10)),
    ((-710, 0, -5), (420, 1900, 10)),
    ((-100, 225, -5), (800, 1450, 10)),
    ((825, 0, -5), (1050, 1900, 10)),
    ((1575, 0, -5), (450, 800, 10)),
)):
    collision_box(f"FS_COLLISION_MainDeck_{index}", loc, dims)

# Lower deck occupies the useful hull volume: engine aft, crew/services amidship,
# and cargo/workshop forward, all connected by a central passage.
place("FS_LowerDeckFloor", "SM_LowerDeckFloor", (-250, 0, -282))
collision_box("FS_COLLISION_LowerDeckFloor", (-250, 0, -276), (2800, 1300, 12))

for side in (-1, 1):
    for index, x in enumerate((-1450, -1050, -650, -250, 150, 550, 950)):
        place(f"FS_LowerWall_{side}_{index}", "SM_InteriorWall4m", (x, side * 650, -270))
        collision_box(f"FS_COLLISION_LowerWall_{side}_{index}", (x, side * 650, -135), (400, 14, 270))
for label, x in (("Aft", -1650), ("Bow", 1150)):
    place(f"FS_LowerWall_{label}", "SM_InteriorWall4m", (x, 0, -270), (0, 90, 0), (3.25, 1, 1))
    collision_box(f"FS_COLLISION_LowerWall_{label}", (x, 0, -135), (14, 1300, 270))

for bulkhead, x in (("Engine", -750), ("Cargo", 150)):
    place(f"FS_{bulkhead}BulkheadPort", "SM_InteriorWall4m", (x, 400, -270), (0, 90, 0), (1.25, 1, 1))
    place(f"FS_{bulkhead}BulkheadStarboard", "SM_InteriorWall4m", (x, -400, -270), (0, 90, 0), (1.25, 1, 1))
    place(f"FS_{bulkhead}DoorFrame", "SM_BulkheadDoorFrame", (x, 0, -270), (0, 90, 0))
    collision_box(f"FS_COLLISION_{bulkhead}BulkheadPort", (x, 400, -135), (14, 500, 270))
    collision_box(f"FS_COLLISION_{bulkhead}BulkheadStarboard", (x, -400, -135), (14, 500, 270))

# Physical stair from crew deck to the lower deck.
place("FS_StairToLowerDeck", "SM_StairFlight", (-1080, -350, -270))
for index in range(10):
    collision_box(f"FS_COLLISION_Stair_{index}", (-1215 + index * 30, -350, -258 + index * 24), (32, 116, 24))
place("FS_StairSafetyFrame", "SM_BulkheadDoorFrame", (-920, -350, 0), (0, 90, 0), (0.9, 1, 1))

# Aft machinery space.
place("FS_MainEngine", "SM_MainEngine", (-1280, 285, -270), (0, 0, 0), (0.82, 0.82, 0.82))
place("FS_FuelTank", "SM_FuelTank", (-1350, -520, -270), (0, 0, 0), (0.78, 0.78, 0.78))
place("FS_PumpSkid", "SM_PumpSkid", (-900, 390, -270), (0, 90, 0), (0.82, 0.82, 0.82))
place("FS_PropellerShaft", "SM_PropellerShaft", (-1430, 0, -270), (0, 0, 0), (0.72, 0.72, 0.72))
place("FS_PipeRunEngine", "SM_PipeRun4m", (-1120, 610, -250), (0, 0, 0), (1.2, 1, 1))

# Crew and hotel services amidships.
place("FS_CrewBunksA", "SM_BunkPair", (-520, -470, -270), (0, 90, 0), (0.9, 0.9, 0.9))
place("FS_CrewBunksB", "SM_BunkPair", (-270, -470, -270), (0, 90, 0), (0.9, 0.9, 0.9))
place("FS_CrewLockers", "SM_LockerBank", (-520, 560, -270), (0, 0, 0), (0.8, 0.8, 0.8))
place("FS_MessTable", "SM_MessTableSet", (-250, 300, -270), (0, 0, 0), (0.82, 0.82, 0.82))
place("FS_Washroom", "SM_WashroomUnit", (-20, -475, -270), (0, 0, 0), (0.82, 0.82, 0.82))
place("FS_ElectricalCabinets", "SM_ElectricalCabinets", (-50, 540, -270), (0, 0, 0), (0.82, 0.82, 0.82))

# Forward hold and workshop.
place("FS_CargoRackPort", "SM_CargoRack", (520, 540, -270), (0, 0, 0), (0.86, 0.86, 0.86))
place("FS_CargoRackStarboard", "SM_CargoRack", (520, -540, -270), (0, 180, 0), (0.86, 0.86, 0.86))
place("FS_CargoPalletA", "SM_CargoPalletStack", (780, 310, -270), (0, 0, 0), (0.78, 0.78, 0.78))
place("FS_CargoPalletB", "SM_CargoPalletStack", (780, -310, -270), (0, 0, 0), (0.78, 0.78, 0.78))
place("FS_Workbench", "SM_Workbench", (1020, 500, -270), (0, -90, 0), (0.8, 0.8, 0.8))
place("FS_AirHandler", "SM_AirHandler", (1010, -420, -270), (0, 0, 0), (0.75, 0.75, 0.75))
place("FS_BilgePump", "SM_BilgePump", (1000, 120, -270), (0, 0, 0), (0.75, 0.75, 0.75))

for label, loc, dims in (
    ("MainEngine", (-1280, 285, -190), (460, 210, 160)),
    ("FuelTank", (-1350, -520, -190), (310, 140, 160)),
    ("PumpSkid", (-900, 390, -220), (135, 250, 100)),
    ("BunksA", (-520, -470, -170), (110, 190, 200)),
    ("BunksB", (-270, -470, -170), (110, 190, 200)),
    ("Washroom", (-20, -475, -145), (200, 140, 250)),
    ("Electrical", (-50, 540, -165), (200, 70, 210)),
    ("RackPort", (520, 540, -170), (220, 90, 200)),
    ("RackStarboard", (520, -540, -170), (220, 90, 200)),
    ("PalletA", (780, 310, -185), (190, 135, 170)),
    ("PalletB", (780, -310, -185), (190, 135, 170)),
):
    collision_box(f"FS_COLLISION_{label}", loc, dims)

# Readable industrial lighting; one fixture mesh and one light per bay.
for index, (x, y) in enumerate(((-1450, 0), (-1050, 0), (-550, 0), (-100, 0), (350, 0), (800, 0),
                                (-1100, -480), (-1100, 480), (-300, -480), (-300, 480), (750, -480), (750, 480))):
    place(f"FS_CeilingLightMesh_{index}", "SM_CeilingLight", (x, y, -24), (0, 0, 0), (0.9, 0.9, 0.9))
    light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, -70))
    light.set_actor_label(f"FS_WorkLight_{index}")
    light.light_component.set_editor_property("intensity", 520.0)
    light.light_component.set_editor_property("attenuation_radius", 430.0)
    light.light_component.set_editor_property("light_color", unreal.Color(255, 195, 128, 255))
    light.light_component.set_editor_property("cast_shadows", True)

unreal.EditorAssetLibrary.save_directory("/Game/Art/FullFreighter", only_if_is_dirty=False, recursive=True)
if not levels.save_current_level():
    raise RuntimeError("Could not save complete FreighterDeck")

full_count = sum(1 for actor in actors.get_all_level_actors() if actor.get_actor_label().startswith("FS_"))
collision_count = sum(1 for actor in actors.get_all_level_actors() if actor.get_actor_label().startswith("FS_COLLISION_"))
unreal.log(f"CALMY_FULL_FREIGHTER_PASS meshes=12 actors={full_count} collision={collision_count} decks=2 rooms=7")
