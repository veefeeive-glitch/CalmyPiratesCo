"""Enable bundled Rigify, preserve preferences, and smoke-test an unsaved rig.

Run with Blender --background --python this_file. Never opens/saves project assets.
"""
import datetime
import json
from pathlib import Path
import shutil

import addon_utils
import bpy

if not bpy.app.background:
    raise RuntimeError("Run in a separate background Blender, not an artist session")

pref = Path(bpy.utils.user_resource('CONFIG')) / 'userpref.blend'
backup = None
if pref.exists():
    backup = pref.with_name('userpref.before-rigify-' + datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f') + '.blend')
    shutil.copy2(pref, backup)

addon_utils.enable('rigify', default_set=True, persistent=True)
assert addon_utils.check('rigify')[1], 'Rigify did not load'
# Respect the project's existing no-telemetry policy; do not enable cloud services.
mcp = bpy.context.preferences.addons.get('blender_mcp')
if mcp and hasattr(mcp.preferences, 'telemetry_consent'):
    mcp.preferences.telemetry_consent = False
bpy.ops.wm.save_userpref()

# This is a disposable background scene, not a character or ship master.
assert bpy.ops.object.armature_human_metarig_add() == {'FINISHED'}
metarig = bpy.context.object
assert bpy.ops.pose.rigify_generate() == {'FINISHED'}
rig = bpy.context.object
assert rig.type == 'ARMATURE' and rig != metarig and len(rig.data.bones) > 100
print('RIGIFY_SETUP_PASS=' + json.dumps({
    'blender': bpy.app.version_string,
    'enabled': addon_utils.check('rigify'),
    'generated_bones': len(rig.data.bones),
    'preferences_backup': str(backup),
    'project_assets_saved': False,
}))
