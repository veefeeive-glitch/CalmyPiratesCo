"""Export named wing mask as COLOR_0 instead of glTF's default white color."""
from pathlib import Path
import bpy
root=Path(__file__).resolve().parents[1]/'SourceAssets/Generated/FlyR2'
bpy.ops.wm.open_mainfile(filepath=str(root/'Fly_R2.blend'))
bpy.ops.object.select_all(action='DESELECT')
fly=bpy.data.objects['SM_Fly_R2'];fly.select_set(True);bpy.context.view_layer.objects.active=fly
bpy.ops.export_scene.gltf(filepath=str(root/'SM_Fly_R2_Corrected.glb'),export_format='GLB',use_selection=True,export_yup=True,export_apply=True,
    export_vertex_color='NAME',export_vertex_color_name='Color',export_all_vertex_colors=False)
bpy.context.scene.view_settings.exposure=-3
bpy.ops.wm.save_as_mainfile(filepath=str(root/'Fly_R2.blend'))
bpy.ops.render.render(write_still=True)
print('FLY_MASK_EXPORT_PASS')
