"""Generate a separate contract greybox; never overwrite an existing map or the original DeckTest."""
import runpy
from pathlib import Path
import unreal

MAP = "/Game/Maps/ShiftTest"
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log("ShiftTest already exists; leaving user edits intact.")
else:
    runpy.run_path(str(Path(__file__).with_name("create_deck_test.py")), init_globals={"MAP": MAP})
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    cube = unreal.load_asset("/Engine/BasicShapes/Cube")
    steel = unreal.load_asset("/Game/Materials/M_Steel")
    orange = unreal.load_asset("/Game/Materials/M_SafetyOrange")
    starts = [(-650, 0, 100), (-650, 220, 100), (-650, -220, 100), (-400, 350, 100)]
    crates = [(-430, -280, 45), (-430, 0, 45), (-430, 220, 45)]
    for actor in actors.get_all_level_actors():
        name = actor.get_actor_label()
        if name.startswith("Cabin_Blockout") or name.startswith("Deck_Lane_Marking") or (name.startswith("Safety_Rail") and actor.get_actor_location().y < -450):
            actors.destroy_actor(actor)  # Only actors generated in this new, unsaved map.
        elif name.startswith("PlayerStart_"):
            index = int(name.rsplit("_", 1)[1])
            actor.set_actor_location_and_rotation(unreal.Vector(*starts[index]), unreal.Rotator(), False, True)
        elif name.startswith("Cargo_"):
            index = actor.get_editor_property("cargo_id") - 1
            actor.set_actor_location(unreal.Vector(*crates[index]), False, True)

    def box(name, location, scale, material, collision=True):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*location))
        actor.set_actor_label(name)
        actor.static_mesh_component.set_static_mesh(cube)
        actor.static_mesh_component.set_material(0, material)
        actor.set_actor_scale3d(unreal.Vector(*scale))
        if not collision:
            actor.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        return actor

    box("Route_Equipment_Block", (0, 80, 100), (2, 3.8, 2), steel)
    for x in (-550, 550):
        box("Open_Gangway_Rail", (x, -480, 75), (6, .2, 1.5), orange)
    box("Gangway_Hazard_Edge", (0, -480, 2), (4.8, .2, .03), orange, False)

    assets = unreal.AssetToolsHelpers.get_asset_tools()
    green = assets.create_asset("M_DeliveryGreen", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew()) if not unreal.EditorAssetLibrary.does_asset_exist("/Game/Materials/M_DeliveryGreen") else unreal.load_asset("/Game/Materials/M_DeliveryGreen")
    node = unreal.MaterialEditingLibrary.create_material_expression(green, unreal.MaterialExpressionVectorParameter)
    node.set_editor_property("parameter_name", "Tint")
    node.set_editor_property("default_value", unreal.LinearColor(.08, .65, .22, 1))
    unreal.MaterialEditingLibrary.connect_material_property(node, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(green)
    unreal.EditorAssetLibrary.save_loaded_asset(green)
    box("DELIVERY_GREEN_ZONE", (650, -180, 1), (3.4, 5, .02), green, False)
    for y in (-430, 70):
        box("Delivery_Border", (650, y, 2), (3.4, .06, .02), orange, False)
    for x in (480, 820):
        box("Delivery_Border", (x, -180, 2), (.06, 5, .02), orange, False)
    shift = actors.spawn_actor_from_class(unreal.load_class(None, "/Script/CalmyPiratesCo.CalmyShift"), unreal.Vector(650, -180, 100))
    shift.set_actor_label("Contract_Delivery_Zone")
    shift.get_editor_property("delivery_zone").set_box_extent(unreal.Vector(170, 250, 120))
    levels.save_current_level()
    unreal.log("CALMY_SHIFT_MAP_CREATED " + MAP)
