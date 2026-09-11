"""Create the initial map through Unreal APIs. Existing assets are never overwritten."""
import unreal

MAP = globals().get("MAP", "/Game/Maps/DeckTest")
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log("DeckTest already exists; leaving user edits intact.")
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    assets = unreal.AssetToolsHelpers.get_asset_tools()
    levels.new_level(MAP)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")

    def material(name, color):
        path = "/Game/Materials/" + name
        existing = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
        if existing:
            return existing
        result = assets.create_asset(name, "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
        node = unreal.MaterialEditingLibrary.create_material_expression(result, unreal.MaterialExpressionVectorParameter)
        node.set_editor_property("parameter_name", "Tint")
        node.set_editor_property("default_value", unreal.LinearColor(*color, 1))
        unreal.MaterialEditingLibrary.connect_material_property(node, "", unreal.MaterialProperty.MP_BASE_COLOR)
        unreal.MaterialEditingLibrary.recompile_material(result)
        unreal.EditorAssetLibrary.save_loaded_asset(result)
        return result

    deck_mat = material("M_Deck", (.07, .13, .17))
    edge_mat = material("M_SafetyOrange", (.95, .32, .025))
    steel_mat = material("M_Steel", (.34, .46, .46))
    sea_mat = material("M_Sea", (.012, .09, .12))
    cargo_mat = material("M_Cargo", (.64, .33, .10))

    def box(name, location, scale, mat, collision=True):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(name)
        component = actor.static_mesh_component
        component.set_static_mesh(cube)
        component.set_material(0, mat)
        if not collision:
            component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        return actor

    box("Freighter_Deck", (0, 0, -30), (18, 10, .6), deck_mat)
    box("Hull", (0, 0, -105), (17.4, 9.6, .9), steel_mat)
    box("Sea_Greybox", (0, 0, -180), (100, 100, .1), sea_mat, False)
    for y in (-480, 480):
        box("Safety_Rail", (0, y, 75), (18, .2, 1.5), edge_mat)
    for x in (-880, 880):
        box("Safety_Rail_End", (x, 0, 75), (.2, 9.6, 1.5), edge_mat)
    box("Cabin_Blockout", (650, 310, 140), (2.4, 2.7, 2.8), steel_mat)
    for x in (-600, -200, 200, 600):
        box("Deck_Lane_Marking", (x, -200, 1), (2, .06, .02), edge_mat, False)

    for index, (x, y, yaw) in enumerate([(-450, 0, 0), (350, 0, 180), (-500, 250, 0), (300, 250, 180)]):
        start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(x, y, 100), unreal.Rotator(0, yaw, 0))
        start.set_actor_label("PlayerStart_" + str(index))
        start.set_editor_property("tags", [unreal.Name("P" + str(index))])

    cargo_class = unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyCargo")
    for index, position in enumerate([(180, 0, 45), (-250, 250, 45), (760, -320, 45)], 1):
        cargo = actors.spawn_actor_from_class(cargo_class, unreal.Vector(*position))
        cargo.set_actor_label("Cargo_" + str(index))
        cargo.set_editor_property("cargo_id", index)
        cargo.get_editor_property("mesh").set_material(0, cargo_mat)

    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(-45, -30, 0))
    sun.light_component.set_editor_property("intensity", 5.0)
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, -180))
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 100))
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.light_component.set_editor_property("intensity", 1.0)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyGameMode"))
    levels.save_current_level()
    unreal.log("CALMY_MAP_CREATED " + MAP)
