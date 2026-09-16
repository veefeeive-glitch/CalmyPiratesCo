"""Place R7 details S3 on OceanFreighterR7 (backup first, no ship re-import).

Imports SourceAssets/Generated/ShipR7Details/SM_R7_82_DETAILS.glb with complex-as-simple collision (the ship probe's
collision contract for every SM_R7_ mesh) and Nanite on (100% fallback, Nanite usage on its project material instances),
spawns it at the SM_R7_10_HULL transform and attaches it to
ShipMotion_R7 so it heaves, heels and drifts with the hull. Re-running replaces the previous S3 actor.
"""
import unreal, hashlib, json, shutil, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'SourceAssets/Generated/ShipR7Details'
GLB = SRC / 'SM_R7_82_DETAILS.glb'
MANIFEST = json.loads((SRC / 'details_manifest.json').read_text(encoding='utf8'))
MAP = '/Game/Maps/OceanFreighterR7'
MAP_FILE = ROOT / 'Content/Maps/OceanFreighterR7.umap'
DEST = '/Game/Ships/OceanFreighterR7'
NAME = LABEL = 'SM_R7_82_DETAILS'
assert GLB.exists(), GLB

backup = ROOT / ('SourceAssets/Backups/Before-Details-S3-' + time.strftime('%Y%m%d'))
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
assert mesh, 'details mesh not imported'
if mesh.get_path_name().split('.')[0] != target:
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        unreal.EditorAssetLibrary.delete_asset(target)
    assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0], target)
    mesh = unreal.load_asset(target)
meshes.remove_collisions(mesh)
body = mesh.get_editor_property('body_setup')
body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
body.set_editor_property('double_sided_geometry', True)
# Nanite on, like the R7 structure chunks (refine_ship_r7_game.py): as one ship-long non-Nanite mesh moving with the hull,
# the 40k-triangle detail set overflowed the VSM non-Nanite marking queue. Collision stays complex-as-simple.
nanite = meshes.get_nanite_settings(mesh)
nanite.enabled = True
nanite.fallback_target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
nanite.fallback_percent_triangles = 1.0
nanite.fallback_relative_error = 0.0
meshes.set_nanite_settings(mesh, nanite, True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
# Nanite usage only on the project's own imported instances (fix_ship_r7_material_usage.py); never edit engine materials.
lib = unreal.MaterialEditingLibrary
for slot in mesh.get_editor_property('static_materials'):
    m = slot.material_interface
    path = m.get_path_name() if m else 'None'
    if isinstance(m, unreal.MaterialInstanceConstant) and path.startswith('/Game/'):
        lib.set_material_usage_override(m, unreal.MaterialUsage.MATUSAGE_NANITE, True, True)
        lib.update_material_instance(m)
        assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        unreal.log('DETAILS_MATERIAL nanite_usage=1 %s' % path)
    else:
        unreal.log_warning('DETAILS_MATERIAL not_a_project_instance %s' % path)

assert levels.load_level(MAP)
ship_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyShipMotion')
all_actors = list(actors.get_all_level_actors())
ships = [a for a in all_actors if a.get_class() == ship_cls]
assert len(ships) == 1, 'expected ShipMotion_R7'
ship = ships[0]
hull = [a for a in all_actors if a.get_actor_label() == 'SM_R7_10_HULL']
assert hull, 'SM_R7_10_HULL actor missing'
LIGHT_PREFIX = 'S3_Light_'
for old in [a for a in all_actors if a.get_actor_label() == LABEL or a.get_actor_label().startswith(LIGHT_PREFIX)]:
    old.destroy_actor()
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
# Ceiling lights for the enclosed S3 rooms, matched to the Nightwatch service fill lights (same as S1).
fill = [a for a in all_actors if isinstance(a, unreal.PointLight) and 'Nightwatch_Service_Fill' in a.get_actor_label()]
assert fill, 'Nightwatch_Service_Fill reference light missing'
src_light = fill[0].point_light_component
hull_xf = ref.get_actor_transform()
for spec in MANIFEST.get('lights', []):
    x, y, z = spec['blender_m']
    light = actors.spawn_actor_from_class(unreal.PointLight, hull_xf.transform_location(unreal.Vector(x * 100.0, -y * 100.0, z * 100.0)), unreal.Rotator(0, 0, 0))
    light.set_actor_label(LIGHT_PREFIX + spec['name'])
    c = light.point_light_component
    c.set_mobility(unreal.ComponentMobility.MOVABLE)
    for prop in ('intensity_units', 'intensity', 'light_color', 'temperature', 'use_temperature', 'cast_shadows'):
        c.set_editor_property(prop, src_light.get_editor_property(prop))
    # The service fill is sized for open decks; inside a 2.35 m store at full strength it washed the contents out.
    c.set_editor_property('intensity', src_light.get_editor_property('intensity') * .35)
    c.set_editor_property('attenuation_radius', 380.0)
    light.set_folder_path('R7/Details_S3')
    assert light.attach_to_actor(ship, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
# Existing lights the new rooms enclosed: the bow R7_Hold_Worklight pair stood exactly where the stores now are (one inside
# the deck store, one between the paint store ceiling and the deck). Move any light inside a room volume, or up to 0.6 m
# above its ceiling, 0.6 m aft of that room so the compartment stays lit. Lights already outside are left alone.
moved = []
for item in MANIFEST['built']:
    if item['group'] not in ('co2_room', 'paint_store', 'deck_store'):
        continue
    x0, y0, z0, x1, y1, z1 = item['volume']
    for a in actors.get_all_level_actors():
        if not isinstance(a, unreal.PointLight) or a.get_actor_label().startswith(LIGHT_PREFIX):
            continue
        p = hull_xf.inverse_transform_location(a.get_actor_location())
        bx, by, bz = p.x / 100.0, -p.y / 100.0, p.z / 100.0
        if x0 - .1 <= bx <= x1 + .1 and y0 - .1 <= by <= y1 + .1 and z0 - .1 <= bz <= z1 + .6:
            a.set_actor_location(hull_xf.transform_location(unreal.Vector((x0 - .6) * 100.0, p.y, p.z)), False, False)
            moved.append('%s@%s:x%.2f->%.2f' % (a.get_actor_label(), item['group'], bx, x0 - .6))
assert levels.save_current_level()
sha = hashlib.sha256(MAP_FILE.read_bytes()).hexdigest()
unreal.log('SHIP_R7_DETAILS_PASS mesh=%s built=%d skipped=%d triangles=%d lights=%d moved_lights=%s attached=%s map_sha256=%s backup=%s' % (
    target, len(MANIFEST['built']), len(MANIFEST['skipped']), MANIFEST['triangles'], len(MANIFEST.get('lights', [])), moved, ship.get_actor_label(), sha, backup))

end = time.monotonic() + 6
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
