"""Import the 2026-09-15 Poly Haven CC0 ship props (glTF 1K) into /Game/ThirdParty/PolyHaven/<asset>.

UnrealEditor.exe CalmyPiratesCo.uproject -ExecutePythonScript=Scripts/import_polyhaven_ship_props.py -unattended -nop4 -NoSound
Assets only: never loads or saves a map. Collision: simple box for props people bump into (barrel, power box, extinguisher,
sign), none for wall/ceiling fittings (sconce, alarm, lifebuoy, pipes) so they can never block a walkway. Logs
POLYHAVEN_IMPORT_PASS with mesh bounds (cm) for placement.
"""
import unreal, time, json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / 'SourceAssets/ThirdParty/PolyHaven'
DEST = '/Game/ThirdParty/PolyHaven'
ASSETS = {
    'lifebuoy': False, 'korean_fire_extinguisher_01': True, 'fire_alarm': False, 'industrial_caged_sconce': False,
    'modular_industrial_pipes_01': False, 'power_box_01': True, 'Barrel_01': True, 'WetFloorSign_01': True,
}
tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
report = {}
for aid, blocking in ASSETS.items():
    gltf = next(SRC.joinpath(aid).glob('*_1k.gltf'))
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(gltf))
    task.set_editor_property('destination_path', DEST + '/' + aid)
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    tools.import_asset_tasks([task])
    meshes = [p for p in lib.list_assets(DEST + '/' + aid, recursive=True) if isinstance(lib.load_asset(p), unreal.StaticMesh)]
    info = []
    for p in meshes:
        mesh = lib.load_asset(p)
        # Small props stay off Nanite: the glTF import turns it on, and then their translucent slots (extinguisher glass
        # and label) are invalid on a Nanite mesh and every slot without the Nanite usage flag falls back to the default
        # material in game. Saving each material also stops the "will recompile every editor launch" warning.
        editor = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
        nanite = editor.get_nanite_settings(mesh)
        nanite.enabled = False
        editor.set_nanite_settings(mesh, nanite, True)
        for slot in mesh.get_editor_property('static_materials'):
            m = slot.material_interface
            if m and m.get_path_name().startswith('/Game/'):
                lib.save_loaded_asset(m)
        body = mesh.get_editor_property('body_setup')
        if body:
            body.set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
        if blocking:
            unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
            unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
        else:
            unreal.EditorStaticMeshLibrary.remove_collisions(mesh)
        b = mesh.get_bounding_box()
        info.append({'mesh': p.split('.')[0], 'min': [round(b.min.x), round(b.min.y), round(b.min.z)], 'max': [round(b.max.x), round(b.max.y), round(b.max.z)],
                     'triangles': unreal.EditorStaticMeshLibrary.get_number_verts(mesh, 0), 'blocking': blocking})
        lib.save_asset(p)
    report[aid] = info
    unreal.log('POLYHAVEN_IMPORT %s meshes=%d %s' % (aid, len(info), json.dumps(info)[:400]))
out = ROOT / 'Saved/Verification/PolyHaven-Import-2026-09-15.json'
out.write_text(json.dumps(report, indent=1), encoding='utf-8')
ok = all(report.get(a) for a in ASSETS)
unreal.log('POLYHAVEN_IMPORT_%s assets=%d report=%s' % ('PASS' if ok else 'FAIL', sum(1 for a in ASSETS if report.get(a)), out))
end = time.monotonic() + 4
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
