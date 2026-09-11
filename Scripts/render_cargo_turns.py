"""Six focused C02 inspection views; no cutaway, no save of master/candidate."""
import bpy,json,hashlib,math,os
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'Saved/Verification'/os.environ.get('CALMY_CARGO_SHOTS','R7-Cargo-C02-Shots');out.mkdir(parents=True,exist_ok=False)
sha=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest()
s=bpy.context.scene;s.render.engine='BLENDER_EEVEE';s.eevee.taa_render_samples=24
s.render.resolution_x=960;s.render.resolution_y=540;s.render.resolution_percentage=100;s.render.image_settings.file_format='PNG'
bpy.ops.object.camera_add();cam=bpy.context.object;s.camera=cam;cam.data.sensor_fit='HORIZONTAL';cam.data.lens=36/(2*math.tan(math.radians(51)));cam.data.clip_start=.035
bpy.ops.object.light_add(type='AREA');light=bpy.context.object;light.data.energy=90;light.data.size=2
views=[
    ('mess',(-49.09,-2.8,11.91),(-47.6,-4.8,11.5)),
    ('internal_turn',(-42.7,5.57,18.785),(-43.5,3.77,18.1)),
    ('internal_headroom',(-43.8,3.77,18.4),(-42.25,4.7,19.2)),
    ('hold1_entry',(-12.9,-7.9,11.82),(-12.9,-5.0,11.2)),
    ('hold1_turn',(-12.65,-5.0,5.82),(-14.5,-4.08,5.0)),
    ('hold2_entry',(41.1,7.9,11.82),(41.1,5.0,11.2)),
    ('boat_lobby',(-48.7,2.7,11.91),(-49.4,4.8,11.5)),
    ('officer_lobby',(-48.65,4.3,17.41),(-49.7,3.0,16.9)),
    ('laundry_boundary',(-51.0,3.6,11.91),(-49.7,6.0,11.6)),
    ('medical_boundary',(-51.0,3.6,14.66),(-49.7,6.0,14.3)),
    ('radio_boundary',(-51.0,3.6,17.41),(-49.7,6.0,17.0)),
    ('hold1_sign',(-14.4,-9.3,11.82),(-14.0,-7.5,12.0)),
    ('hold2_sign',(39.6,9.3,11.82),(39.9,7.5,12.0))]
shots=[]
for name,eye,target in views:
    cam.location=eye;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    light.location=eye;light.rotation_euler=cam.rotation_euler;s.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
    shots.append(dict(name=name,eye=eye,target=target,source_sha256=sha,image_sha256=hashlib.sha256(Path(s.render.filepath).read_bytes()).hexdigest(),qa_fill=True,scope='Static Blender inspection, not a gameplay screenshot'))
(out/'shots.json').write_text(json.dumps(shots,indent=2),encoding='utf-8')
