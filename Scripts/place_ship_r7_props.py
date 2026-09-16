"""Place the Poly Haven CC0 ship props on OceanFreighterR7 (after import_polyhaven_ship_props.py).

UnrealEditor.exe CalmyPiratesCo.uproject -ExecutePythonScript=Scripts/place_ship_r7_props.py -unattended -nop4 -NoSound
Ship-local UE cm (rest pose), attached to ShipMotion_R7. The imported kits share one local space per asset (a lamp, a box
with its door, an alarm with its lever), so every listed piece gets the same actor transform. Conventions from the import
bounds: wall fittings have their back on local y=0 and face +Y; 'facing' is the world yaw the front should point to.
Floor props stand on z and keep clear of surveyed walkways (verify with run_ship_playability_ue.ps1).
Re-running removes the previous PH_ props first. Backs up the map once per day. Logs SHIP_R7_PROPS_PASS.
"""
import unreal, time, shutil, hashlib, math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP = '/Game/Maps/OceanFreighterR7'
MAP_FILE = ROOT / 'Content/Maps/OceanFreighterR7.umap'
LIB = '/Game/ThirdParty/PolyHaven'

SCONCE = ('industrial_caged_sconce', ['industrial_caged_sconce_c'])
ALARM = ('fire_alarm', ['fire_alarm', 'fire_alarm_lever'])
POWERBOX = ('power_box_01', ['power_box_01_box', 'power_box_01_door'])
EXTINGUISHER = ('korean_fire_extinguisher_01', None)
LIFEBUOY = ('lifebuoy', None)
DRUM = ('Barrel_01', None)
WETSIGN = ('WetFloorSign_01', None)

# (kit, label, ship-local anchor, facing yaw deg, mount): 'wall' anchor = point on the wall plane; 'floor' = floor point;
# 'rail' = centre of the ring on the rail's inboard face.
PLACEMENTS = [
    # Hold 1 bilge valve station: aft bulkhead of hold 1, plate face X -19.92 m, valve at (-1972, 150, 318).
    (SCONCE, 'PH_Hold1_ValveSconce', (-1992, 150, 440), 0, 'wall'),
    (ALARM, 'PH_Hold1_FloodAlarm', (-1992, 290, 350), 0, 'wall'),
    (SCONCE, 'PH_Hold1_BulkheadSconce_SB', (-1992, -450, 520), 0, 'wall'),
    (SCONCE, 'PH_Hold1_BulkheadSconce_PS', (-1992, 620, 330), 0, 'wall'),
    (WETSIGN, 'PH_Hold1_WetFloorSign', (-1760, -60, 208), 30, 'floor'),
    # Weather-deck fire stations (master FireStation_0..5): the extinguisher stands outboard of each station box, clear
    # of the side lane the traversal routes use (|Y| 8.15 m, capsule edge 8.26 m) and inboard of the rail at |Y| 9.3 m.
    (EXTINGUISHER, 'PH_FireStation0_Extinguisher', (-2800, 878, 1058), 0, 'floor'),
    (EXTINGUISHER, 'PH_FireStation1_Extinguisher', (-2800, -878, 1058), 180, 'floor'),
    (EXTINGUISHER, 'PH_FireStation2_Extinguisher', (1200, 878, 1058), 0, 'floor'),
    (EXTINGUISHER, 'PH_FireStation3_Extinguisher', (1200, -878, 1058), 180, 'floor'),
    (EXTINGUISHER, 'PH_FireStation4_Extinguisher', (4500, 878, 1058), 0, 'floor'),
    (EXTINGUISHER, 'PH_FireStation5_Extinguisher', (4500, -878, 1058), 180, 'floor'),
    # Lifebuoys on the inboard face of the weather-deck side rails, ring in the rail plane, facing inboard.
    (LIFEBUOY, 'PH_Lifebuoy_PS_Aft', (-1000, 918, 1150), -90, 'rail'),
    (LIFEBUOY, 'PH_Lifebuoy_SB_Aft', (-1000, -918, 1150), 90, 'rail'),
    (LIFEBUOY, 'PH_Lifebuoy_PS_Mid', (2600, 918, 1150), -90, 'rail'),
    (LIFEBUOY, 'PH_Lifebuoy_SB_Mid', (2600, -918, 1150), 90, 'rail'),
    # Engine room lower flat: switchboards on the forward bulkhead (X -35 m) facing aft; drums on the tank top.
    (POWERBOX, 'PH_ER_PowerBox_PS', (-3508, 520, 580), 180, 'wall'),
    (POWERBOX, 'PH_ER_PowerBox_SB', (-3508, -520, 580), 180, 'wall'),
    (DRUM, 'PH_ER_Drum_1', (-3440, 760, 208), 0, 'floor'),
    (DRUM, 'PH_ER_Drum_2', (-3375, 790, 208), 25, 'floor'),
]

backup = ROOT / ('SourceAssets/Backups/Before-PolyHavenProps-' + time.strftime('%Y%m%d'))
backup.mkdir(parents=True, exist_ok=True)
if not (backup / MAP_FILE.name).exists():
    shutil.copy2(MAP_FILE, backup / MAP_FILE.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lib = unreal.EditorAssetLibrary
assert levels.load_level(MAP)
all_actors = actors.get_all_level_actors() or []
ship = next(a for a in all_actors if a.get_actor_label() == 'ShipMotion_R7')
for a in all_actors:
    if a.get_actor_label().startswith('PH_'):
        actors.destroy_actor(a)


def kit_meshes(kit):
    asset, names = kit
    meshes = {}
    for p in lib.list_assets(LIB + '/' + asset, recursive=True):
        m = lib.load_asset(p)
        if isinstance(m, unreal.StaticMesh):
            meshes[m.get_name()] = m
    if names is None:
        assert len(meshes) == 1, '%s has %d meshes' % (asset, len(meshes))
        return list(meshes.values())
    missing = [n for n in names if n not in meshes]
    assert not missing, '%s missing %s' % (asset, missing)
    return [meshes[n] for n in names]


def union_bounds(meshes):
    lo = [1e9] * 3; hi = [-1e9] * 3
    for m in meshes:
        b = m.get_bounding_box()
        for i, (a, c) in enumerate(((b.min.x, b.max.x), (b.min.y, b.max.y), (b.min.z, b.max.z))):
            lo[i] = min(lo[i], a); hi[i] = max(hi[i], c)
    return lo, hi


rest = ship.get_actor_transform()
placed = []
for kit, label, (x, y, z), facing, mount in PLACEMENTS:
    meshes = kit_meshes(kit)
    # The first listed piece is the body: its bounds place the kit (a door or lever must not shift the anchor).
    lo, hi = union_bounds(meshes[:1])
    if mount == 'floor':
        yaw = facing
        local_off = unreal.Vector(-(lo[0] + hi[0]) / 2, -(lo[1] + hi[1]) / 2, -lo[2])
    else:
        # Front is local +Y: rotate so it points along 'facing'; back plane (local min y) sits on the wall/rail face.
        yaw = facing - 90
        local_off = unreal.Vector(-(lo[0] + hi[0]) / 2, -lo[1] + (1 if mount == 'wall' else 0), -(lo[2] + hi[2]) / 2)
    rot = unreal.Rotator(0, yaw, 0)
    # unreal.Rotator has no rotate_vector in UE 5.8 Python: yaw-only rotation is two lines of trigonometry.
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    offset = unreal.Vector(local_off.x * c - local_off.y * s, local_off.x * s + local_off.y * c, local_off.z)
    world = rest.transform_location(unreal.Vector(x, y, z) + offset)
    for i, mesh in enumerate(meshes):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, world, rot)
        actor.set_actor_label(label if i == 0 else '%s_%s' % (label, mesh.get_name().split('_')[-1]))
        comp = actor.static_mesh_component
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_static_mesh(mesh)
        # Fire extinguishers stand in the side lane the traversal routes use at |Y| 7.9/8.15/8.6 m: on a 1.2 m strip
        # between the station cabinet and the rail there is no spot a blocking prop can take, so they are decor only.
        blocking = mount == 'floor' and i == 0 and kit is not EXTINGUISHER
        comp.set_collision_profile_name('BlockAll' if blocking else 'NoCollision')
        comp.set_editor_property('can_character_step_up_on', unreal.CanBeCharacterBase.ECB_NO)
        actor.attach_to_actor(ship, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    if kit is SCONCE:
        # The caged sconce is only a mesh: without a lamp of its own the hold corner it marks stays as dark as before.
        lamp = actors.spawn_actor_from_class(unreal.PointLight, world + unreal.Vector(c * 34, s * 34, 0), rot)
        lamp.set_actor_label(label + '_Lamp')
        comp = lamp.point_light_component
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        comp.set_editor_property('intensity', 260.0)
        comp.set_editor_property('attenuation_radius', 620.0)
        comp.set_editor_property('light_color', unreal.Color(255, 236, 205))
        comp.set_editor_property('cast_shadows', False)
        lamp.attach_to_actor(ship, '', unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    placed.append('%s@(%d,%d,%d)x%d' % (label, x, y, z, len(meshes)))

assert levels.save_current_level()
unreal.log('SHIP_R7_PROPS_PASS placed=%d map_sha256=%s backup=%s props=%s' % (len(placed), hashlib.sha256(MAP_FILE.read_bytes()).hexdigest(), backup, ';'.join(placed)))
end = time.monotonic() + 5
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
