"""Close-up inspection of C01 cooler move. Does not save or hide ship geometry."""
import bpy, json, hashlib, math, os
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'Saved/Verification'/os.environ.get('CALMY_C01_OUTPUT','R7-Cargo-C01-Shots');out.mkdir(parents=True,exist_ok=False)
sha=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest()
s=bpy.context.scene;s.render.engine='BLENDER_EEVEE';s.eevee.taa_render_samples=24
s.render.resolution_x=960;s.render.resolution_y=540;s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG'
bpy.ops.object.camera_add();cam=bpy.context.object;s.camera=cam;cam.data.sensor_fit='HORIZONTAL';cam.data.lens=36/(2*math.tan(math.radians(51)));cam.data.clip_start=.035
bpy.ops.object.light_add(type='AREA');fill=bpy.context.object;fill.data.energy=110;fill.data.size=2
shots=[]
for side in (-1,1):
    for tag,eye,target in (
        ('aisle',(-41.1,side*4.72,5.82),(-43,side*5.9,5.5)),
        ('connections',(-40.6,side*5.1,6.45),(-41.9,side*7.2,6.5))):
        cam.location=eye;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
        fill.location=eye;fill.rotation_euler=cam.rotation_euler
        name=f'C01_{side}_{tag}';s.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
        shots.append(dict(name=name,eye=list(eye),target=list(target),source_sha256=sha,image_sha256=hashlib.sha256(Path(s.render.filepath).read_bytes()).hexdigest(),qa_fill=True))
(out/'shots.json').write_text(json.dumps(shots,indent=2),encoding='utf-8')
print('C01_REVIEW_READY',len(shots),flush=True)
