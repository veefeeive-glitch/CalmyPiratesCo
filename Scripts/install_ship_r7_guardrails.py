"""Place R7 guardrails S2 on OceanFreighterR7 (backup first, no ship re-import).

Imports SourceAssets/Generated/ShipR7Guardrails/SM_R7_81_GUARDRAILS.glb with complex-as-simple collision (rails must
stop players) and Nanite off, spawns it at the SM_R7_10_HULL transform and attaches it to ShipMotion_R7 so it heaves,
heels and drifts with the hull. Re-running replaces the previous S2 actor.
"""
import unreal, hashlib, json, shutil, time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'SourceAssets/Generated/ShipR7Guardrails'
GLB = SRC / 'SM_R7_81_GUARDRAILS.glb'
MANIFEST = json.loads((SRC / 'guardrails_manifest.json').read_text(encoding='utf8'))
MAP = '/Game/Maps/OceanFreighterR7'
MAP_FILE = ROOT / 'Content/Maps/OceanFreighterR7.umap'
DEST = '/Game/Ships/OceanFreighterR7'
NAME = LABEL = 'SM_R7_81_GUARDRAILS'
assert GLB.exists(), GLB

backup = ROOT / ('SourceAssets/Backups/Before-Guardrails-S2-' + time.strftime('%Y%m%d'))
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
assert mesh, 'guardrail mesh not imported'
if mesh.get_path_name().split('.')[0] != target:
    if unreal.EditorAssetLibrary.does_asset_exist(target):
        unreal.EditorAssetLibrary.delete_asset(target)
    assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0], target)
    mesh = unreal.load_asset(target)
meshes.remove_collisions(mesh)
body = mesh.get_editor_property('body_setup')
body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
body.set_editor_property('double_sided_geometry', True)
# Nanite on with a full-triangle fallback, like the R7 chunks and the S3 details: the rails are now kilometres of one
# ship-long mesh moving with the hull, and as a non-Nanite mesh they overflowed the VSM non-Nanite marking queue
# (visible as on-screen warnings in the playthrough frames). Collision stays complex-as-simple.
nanite = meshes.get_nanite_settings(mesh)
nanite.enabled = True
nanite.fallback_target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
nanite.fallback_percent_triangles = 1.0
nanite.fallback_relative_error = 0.0
meshes.set_nanite_settings(mesh, nanite, True)
assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
# Nanite usage only on the project's own imported instances; never edit engine materials.
lib = unreal.MaterialEditingLibrary
for slot in mesh.get_editor_property('static_materials'):
    m = slot.material_interface
    path = m.get_path_name() if m else 'None'
    if isinstance(m, unreal.MaterialInstanceConstant) and path.startswith('/Game/'):
        lib.set_material_usage_override(m, unreal.MaterialUsage.MATUSAGE_NANITE, True, True)
        lib.update_material_instance(m)
        assert unreal.EditorAssetLibrary.save_loaded_asset(m)
        unreal.log('GUARDRAIL_MATERIAL nanite_usage=1 %s' % path)
    else:
        unreal.log_warning('GUARDRAIL_MATERIAL not_a_project_instance %s' % path)

assert levels.load_level(MAP)
ship_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyShipMotion')
all_actors = list(actors.get_all_level_actors())
ships = [a for a in all_actors if a.get_class() == ship_cls]
assert len(ships) == 1, 'expected ShipMotion_R7'
ship = ships[0]
hull = [a for a in all_actors if a.get_actor_label() == 'SM_R7_10_HULL']
assert hull, 'SM_R7_10_HULL actor missing'
for old in [a for a in all_actors if a.get_actor_label() == LABEL]:
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
assert levels.save_current_level()
sha = hashlib.sha256(MAP_FILE.read_bytes()).hexdigest()
unreal.log('SHIP_R7_GUARDRAILS_PASS mesh=%s segments=%d rail_m=%s attached=%s map_sha256=%s backup=%s' % (
    target, len(MANIFEST['segments']), MANIFEST['rail_length_m'], ship.get_actor_label(), sha, backup))

end = time.monotonic() + 6
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
