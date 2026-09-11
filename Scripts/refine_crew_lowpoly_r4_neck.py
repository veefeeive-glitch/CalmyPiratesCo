"""Scoped correction of this iteration's two heads after close-up review."""
from pathlib import Path
import bpy
from mathutils import Vector
out=Path(__file__).resolve().parents[1]/'SourceAssets/Generated/CrewLowPolyR4'
bpy.ops.wm.open_mainfile(filepath=str(out/'Crew_LowPoly_R4.blend'))
for variant in ('Deckhand','Misha'):
    o=bpy.data.objects['SM_'+variant+'_Head']
    bottom=[v for v in o.data.vertices if abs(v.co.z)<.00001]
    assert len(bottom)==8,len(bottom)
    for v in bottom:v.co.z=-.05
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);bpy.context.view_layer.objects.active=o
    loc=o.location.copy();o.location=(0,0,0)
    bpy.ops.export_scene.gltf(filepath=str(out/(o.name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    o.location=loc
scene=bpy.context.scene
bpy.ops.wm.save_as_mainfile(filepath=str(out/'Crew_LowPoly_R4.blend'))
scene.render.filepath=str(out/'R4_Lineup.png');bpy.ops.render.render(write_still=True)
cam=scene.camera;cam.location=(3,-1.5,.65);cam.rotation_euler=(Vector((0,-.1,.60))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.9
scene.render.filepath=str(out/'R4_Faces.png');bpy.ops.render.render(write_still=True)
