"""Place R7 service spaces S1 on the existing OceanFreighterR7 map (backup first, no re-import of the ship).

Imports SourceAssets/Generated/ShipR7ServiceSpaces/SM_R7_80_SERVICE_SPACES.glb with the same collision and
Nanite settings as the R7 chunks, spawns one static mesh actor at the ship origin and attaches it to
ShipMotion_R7 so it heaves, heels and drifts with the hull. Re-running replaces the previous S1 actor.
"""
import unreal, hashlib, json, shutil, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'SourceAssets/Generated/ShipR7ServiceSpaces'
GLB = SRC / 'SM_R7_80_SERVICE_SPACES.glb'
MANIFEST = json.loads((SRC / 'service_spaces_manifest.json').read_text(encoding='utf8'))
MAP = '/Game/Maps/OceanFreighterR7'
MAP_FILE = ROOT / 'Content/Maps/OceanFreighterR7.umap'
DEST = '/Game/Ships/OceanFreighterR7'
NAME = 'SM_R7_80_SERVICE_SPACES'
LABEL = 'SM_R7_80_SERVICE_SPACES'
LIGHT_PREFIX = 'S1_Light_'
assert GLB.exists(), GLB

backup = ROOT / ('SourceAssets/Backups/Before-ServiceSpaces-S1-' + time.strftime('%Y%m%d'))
backup.mkdir(parents=True, exist_ok=True)
if not (backup / MAP_FILE.name).exists():
    shutil.copy2(MAP_FILE, backup / MAP_FILE.name)

tools = unreal.AssetToolsHelpers.get_asset_tools()
meshes = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

target = DEST + '/' + NAME
task = unreal.AssetImportTask()
task.filename = str(GLB); task.destination_path = DEST; task.automated = True; task.save = True; task.replace_existing = True
tools.import_asset_tasks([task])
mesh = None
for p in task.imported_object_paths:
    a = unreal.load_asset(p)
    if isinstance(a, unreal.StaticMesh): mesh = a
assert mesh, 'service spaces mesh not imported'
if mesh.get_path_name().split('.')[0] != target:
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        unreal.EditorAssetLibrary.delete_asset(target)
    assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0], target)
    mesh = unreal.load_asset(target)
meshes.remove_collisions(mesh)
body = mesh.get_editor_property('body_setup')
body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
body.set_editor_property('double_sided_geometry', True)
nanite = meshes.get_nanite_settings(mesh); nanite.enabled = False; meshes.set_nanite_settings(mesh, nanite, True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)

assert levels.load_level(MAP)
ship_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyShipMotion')
all_actors = list(actors.get_all_level_actors())
ships = [a for a in all_actors if a.get_class() == ship_cls]
assert len(ships) == 1, 'expected ShipMotion_R7'
ship = ships[0]
hull = [a for a in all_actors if a.get_actor_label() == 'SM_R7_10_HULL']
assert hull, 'SM_R7_10_HULL actor missing'
for old in [a for a in all_actors if a.get_actor_label() == LABEL or a.get_actor_label().startswith(LIGHT_PREFIX)]:
    old.destroy_actor()

# Same transform as the existing R7 chunk actors (they were imported at the map origin and attached to the ship).
ref = hull[0]
actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, ref.get_actor_location(), ref.get_actor_rotation())
actor.set_actor_label(LABEL)
actor.set_actor_scale3d(ref.get_actor_scale3d())
comp = actor.static_mesh_component
comp.set_mobility(unreal.ComponentMobility.MOVABLE)
comp.set_static_mesh(mesh)
comp.set_collision_profile_name('BlockAll')
actor.set_folder_path('R7/Imported_Static_Structure')
assert actor.attach_to_actor(ship, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
# Ceiling lights for the enclosed S1 rooms, matched to the existing Nightwatch service fill lights.
fill = [a for a in all_actors if isinstance(a, unreal.PointLight) and 'Nightwatch_Service_Fill' in a.get_actor_label()]
assert fill, 'Nightwatch_Service_Fill reference light missing'
src = fill[0].point_light_component
hull_xf = ref.get_actor_transform()
for spec in MANIFEST.get('lights', []):
    x, y, z = spec['blender_m']
    world = hull_xf.transform_location(unreal.Vector(x * 100.0, -y * 100.0, z * 100.0))
    light = actors.spawn_actor_from_class(unreal.PointLight, world, unreal.Rotator(0, 0, 0))
    light.set_actor_label(LIGHT_PREFIX + spec['name'])
    c = light.point_light_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    for prop in ('intensity_units', 'intensity', 'light_color', 'temperature', 'use_temperature', 'cast_shadows'):
        c.set_editor_property(prop, src.get_editor_property(prop))
    c.set_editor_property('attenuation_radius', 380.0)
    light.set_folder_path('R7/ServiceSpaces_S1')
    assert light.attach_to_actor(ship, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
assert levels.save_current_level()
sha = hashlib.sha256(MAP_FILE.read_bytes()).hexdigest()
unreal.log('SHIP_R7_SERVICE_SPACES_PASS mesh=%s built=%d skipped=%d lights=%d attached=%s map_sha256=%s backup=%s' % (
    target, len(MANIFEST['built']), len(MANIFEST['skipped']), len(MANIFEST.get('lights', [])), ship.get_actor_label(), sha, backup))

end = time.monotonic() + 6
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
