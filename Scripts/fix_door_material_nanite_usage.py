"""Give the watertight-door material instances a project-local, Nanite-enabled material chain.

UE logged "missing usage flag Nanite! Default Material will be used in game" for MAT_Interior_EquipmentBlue,
MAT_Interior_Machinery and MAT_Interior_Safety. Their chain was
    MAT_Interior_*  ->  /InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque_DS  ->  /InterchangeAssets/gltf/M_Default
and both engine assets live in the UE install (outside the project) and must not be edited.

This script copies the chain into the project and keeps the intermediate instance so its overrides (opaque,
double-sided) survive:
    MAT_Interior_*  ->  MI_Gltf_Default_Opaque_DS_Nanite  ->  M_Gltf_Default_Nanite (bUsedWithNanite)
It backs up the door instance .uasset files and restores bUsedWithNanite=False on the engine M_Default if an
earlier run set it.
"""
import unreal, shutil, time, json, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ENGINE_BASE = '/InterchangeAssets/gltf/M_Default'
ENGINE_MID = '/InterchangeAssets/gltf/MaterialInstances/MI_Default_Opaque_DS'
LOCAL_BASE = '/Game/Art/FreighterInterior/Materials/M_Gltf_Default_Nanite'
LOCAL_MID = '/Game/Art/FreighterInterior/Materials/MI_Gltf_Default_Opaque_DS_Nanite'
INSTANCES = [
    '/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_EquipmentBlue',
    '/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Machinery',
    '/Game/Art/FreighterInterior/Meshes/SM_WatertightDoor/Materials/MAT_Interior_Safety',
]
assets = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
backup = ROOT / ('SourceAssets/Backups/Before-DoorNaniteUsage-' + time.strftime('%Y%m%d'))
backup.mkdir(parents=True, exist_ok=True)

def disk(path):
    return ROOT / ('Content/' + path[len('/Game/'):] + '.uasset')

def sha(path):
    f = disk(path)
    return hashlib.sha256(f.read_bytes()).hexdigest() if f.exists() else None

def overrides(mi):
    o = mi.get_editor_property('base_property_overrides')
    return {k: str(o.get_editor_property(k)) for k in ('override_blend_mode', 'blend_mode', 'override_two_sided', 'two_sided')}

engine_base = unreal.load_asset(ENGINE_BASE)
engine_mid = unreal.load_asset(ENGINE_MID)
assert isinstance(engine_base, unreal.Material) and isinstance(engine_mid, unreal.MaterialInstanceConstant)

if not assets.does_asset_exist(LOCAL_BASE):
    assert assets.duplicate_asset(ENGINE_BASE, LOCAL_BASE), 'duplicate base failed'
local_base = unreal.load_asset(LOCAL_BASE)
local_base.set_editor_property('used_with_nanite', True)
mel.recompile_material(local_base)
assert assets.save_loaded_asset(local_base, False)

if not assets.does_asset_exist(LOCAL_MID):
    assert assets.duplicate_asset(ENGINE_MID, LOCAL_MID), 'duplicate intermediate instance failed'
local_mid = unreal.load_asset(LOCAL_MID)
mel.set_material_instance_parent(local_mid, local_base)
mel.update_material_instance(local_mid)
assert assets.save_loaded_asset(local_mid, False)
assert overrides(local_mid) == overrides(engine_mid), 'intermediate overrides changed'

report = []
for path in INSTANCES:
    mi = unreal.load_asset(path)
    assert isinstance(mi, unreal.MaterialInstanceConstant), 'not a material instance: ' + path
    f = disk(path)
    if f.exists() and not (backup / f.name).exists():
        shutil.copy2(f, backup / f.name)
    before = mi.get_editor_property('parent').get_path_name().split('.')[0]
    if before != LOCAL_MID:
        mel.set_material_instance_parent(mi, local_mid)
        mel.update_material_instance(mi)
        assert assets.save_loaded_asset(mi, False), 'save failed ' + path
    report.append({'instance': path, 'parent_before': before,
                   'parent_now': mi.get_editor_property('parent').get_path_name().split('.')[0], 'sha256': sha(path)})

engine_restored = False
if engine_base.get_editor_property('used_with_nanite'):
    engine_base.set_editor_property('used_with_nanite', False)
    mel.recompile_material(engine_base)
    engine_restored = assets.save_loaded_asset(engine_base, False)

out = ROOT / 'Saved/Verification/DoorNaniteUsage-report.json'
out.write_text(json.dumps({'backup': str(backup), 'local_base': LOCAL_BASE, 'local_base_sha256': sha(LOCAL_BASE),
                           'local_intermediate': LOCAL_MID, 'local_intermediate_sha256': sha(LOCAL_MID),
                           'intermediate_overrides': overrides(local_mid),
                           'engine_base_used_with_nanite': bool(engine_base.get_editor_property('used_with_nanite')),
                           'engine_flag_restored_this_run': engine_restored, 'instances': report}, indent=2), encoding='utf8')
unreal.log('DOOR_NANITE_USAGE_PASS chain=%s->%s instances=%d engine_base_used_with_nanite=%s report=%s' % (
    LOCAL_MID, LOCAL_BASE, len(report), engine_base.get_editor_property('used_with_nanite'), out))

end = time.monotonic() + 6
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
