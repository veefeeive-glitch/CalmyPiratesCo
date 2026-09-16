"""Create minimal PortHub map: pier volume, lights, PlayerStarts, MissionBoard."""
import time
import unreal

MAP = '/Game/Maps/PortHub'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log('PORTHUB_MAP_EXISTS ' + MAP)
else:
    assert levels.new_level(MAP)

    # Pier deck
    deck = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    deck.set_actor_label('PortHub_PierDeck')
    deck.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    deck.set_actor_scale3d(unreal.Vector(40, 18, 0.4))
    deck.set_actor_location(unreal.Vector(0, 0, -20), False, True)
    deck.static_mesh_component.set_collision_profile_name('BlockAll')

    # Warehouse block behind board
    shed = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-600, 0, 200))
    shed.set_actor_label('PortHub_Warehouse')
    shed.static_mesh_component.set_static_mesh(unreal.load_asset('/Engine/BasicShapes/Cube'))
    shed.set_actor_scale3d(unreal.Vector(8, 12, 4))
    shed.static_mesh_component.set_collision_profile_name('BlockAll')

    # Mission board pedestal actor
    board_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyMissionBoard')
    board = actors.spawn_actor_from_class(board_cls, unreal.Vector(200, 0, 40))
    board.set_actor_label('PortHub_MissionBoard')

    for i, (x, y) in enumerate([(400, 200), (400, -200), (700, 200), (700, -200)]):
        p = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(x, y, 100))
        p.set_editor_property('tags', [unreal.Name('P' + str(i))])
        p.set_actor_label('PortHub_Start_' + str(i))

    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(pitch=-40, yaw=35))
    sun.light_component.set_editor_property('intensity', 5)
    sun.light_component.set_editor_property('atmosphere_sun_light', True)
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, -500))
    sky = actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500))
    sky.light_component.set_editor_property('real_time_capture', True)

    for name, pos in [('PortHub_BoardLight', (200, 0, 280)), ('PortHub_PierLight', (500, 0, 250))]:
        light = actors.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*pos))
        light.set_actor_label(name)
        comp = light.point_light_component
        comp.set_editor_property('intensity_units', unreal.LightUnits.LUMENS)
        comp.set_editor_property('intensity', 2500)
        comp.set_editor_property('attenuation_radius', 900)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property(
        'default_game_mode', unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyGameMode'))
    assert levels.save_current_level()
    unreal.log('PORTHUB_MAP_PASS map=' + MAP)

_after = None
_handle = None

def finish(_dt):
    global _after, _handle
    if _after is None:
        _after = time.monotonic() + 8
    if time.monotonic() >= _after:
        unreal.unregister_slate_post_tick_callback(_handle)
        unreal.SystemLibrary.quit_editor()

_handle = unreal.register_slate_post_tick_callback(finish)
