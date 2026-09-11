"""Render first-person inspection at measured Chaos trajectory positions.
Read-only with respect to the .blend; QA fill light, no hidden ship geometry.
"""
import bpy,json,hashlib,os,re,sys
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from traversal_camera_profile import get_profile
label=os.environ.get('CALMY_AUDIT_REPORT','R5-traversal-audit1')
profile_name=os.environ.get('CALMY_AUDIT_CAMERA','bodycam')
profile=get_profile(profile_name)
report=json.loads((ROOT/'Saved'/'OceanPhysics'/(label+'.json')).read_text(encoding='utf-8-sig'))
ship=json.loads((ROOT/'Saved'/'OceanPhysics'/'ship.json').read_text(encoding='utf-8'))
sha=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest()
assert sha==report['source_sha256']==ship['sha256']
output_label=os.environ.get('CALMY_AUDIT_OUTPUT',label+'-'+profile_name)
assert re.fullmatch(r'[A-Za-z0-9_-]+',output_label),'Use a simple output name'
out=ROOT/'Saved'/'Verification'/output_label
assert not out.exists(),'Review output already exists; preserve previous evidence'
out.mkdir(parents=True)
routes={r['name']:r for r in ship['routes']}
chunks={c['name']:c for c in ship['chunks']}
findings=[];shots=[]
for w in report['walks']:
    if not w['name'].startswith(('AUDIT_','r6_')):continue
    if not w['pass']:
        component=w.get('hit_component');face=w.get('face',-1)
        owner=next((o['name'] for o in chunks.get(component,{}).get('owners',[]) if o['first']<=face<o['first']+o['count']),None)
        findings.append(dict(name=w['name'],segment=w['segment'],position_cm=w['position'],target_cm=w['target'],reason=w['reason'],component=component,face=face,object=owner))
    if not w['name'].endswith('_forward'):continue
    samples=w['trajectory'];usable=[s for s in samples if s['t']>.6]
    if not usable:continue
    route=routes[w['name'].removesuffix('_forward')]
    # End-of-failure is useful for contact identification. Room circuits that
    # pass use the farthest measured position, not the returned doorway.
    if not w['pass']:chosen=usable[-1]
    elif 'room_' in w['name']:
        chosen=max(usable,key=lambda s:(Vector(s['position'])-Vector(usable[0]['position'])).length)
    else:chosen=usable[len(usable)//2]
    shots.append((w,chosen,'contact' if not w['pass'] else 'mid',route))
    if 'room_' not in w['name'] and 'equipment' not in w['name']:
        for fraction,tag in ((.15,'early'),(.85,'late')):shots.append((w,usable[int((len(usable)-1)*fraction)],tag,route))
# Include longitudinal interior passages from their full measured traversals.
for w in report['walks']:
    if w['name'].startswith('full_length_walk') and w['name'].endswith('_forward'):
        samples=w['trajectory'];shots.append((w,samples[len(samples)//2],'mid',routes[w['name'].removesuffix('_forward')]))
(out/'contacts.json').write_text(json.dumps(findings,indent=2),encoding='utf-8')
filter_pattern=os.environ.get('CALMY_AUDIT_FILTER','')
if filter_pattern:shots=[s for s in shots if re.search(filter_pattern,s[0]['name']) and s[2] not in ('early','late')]
assert shots,'No matching review views; do not claim an empty review passed'
scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE'
scene.render.resolution_x=960;scene.render.resolution_y=540;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.eevee.taa_render_samples=24
scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.5
bpy.ops.object.light_add(type='AREA');lamp=bpy.context.object;lamp.data.energy=45;lamp.data.size=2.5
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam
# Nominal current bodycam at rest, not the old CameraComponent transform.
# Static Blender inspection does NOT reproduce CalcCamera's collision sweep,
# spring, actor yaw, or knockdown. Do not label these gameplay screenshots.
import math
cam.data.lens=36/(2*math.tan(math.radians(profile['fov']/2)));cam.data.sensor_width=36
cam.data.sensor_fit='HORIZONTAL'
cam.data.clip_start=.035;cam.data.clip_end=500
manifest=[]
for i,(w,s,tag,route) in enumerate(shots,1):
    p=Vector(s['position']);ss=w['trajectory'];idx=ss.index(s)
    future=ss[min(idx+3,len(ss)-1)]['position'];heading=Vector(future)-p
    if heading.length<5:heading=Vector(w['target'])-p
    heading.z=0
    if heading.length<.001:heading=Vector((1,0,0))
    heading.normalize();eye=p+heading*profile['forward_cm']+Vector((0,0,profile['up_cm']))
    eye=Vector((eye.x/100,-eye.y/100,eye.z/100));d=Vector((heading.x,-heading.y,-.15)).normalized()
    if 'AUDIT_usable_' in w['name']:
        roomname=w['name'].removeprefix('AUDIT_usable_').removesuffix('_forward')
        room=next(r for r in json.loads(scene['room_layout']) if r['name']==roomname)
        # Look at the room's furnishing from an actually reached point, not
        # into the wall at the end of an approach. View yaw is free in game.
        d=(Vector(((room['x0']+room['x1'])/2,room['side']*(room['width']-1.6),room['floor']+.95))-eye).normalized()
    cam.location=eye;cam.rotation_euler=d.to_track_quat('-Z','Y').to_euler()
    lamp.location=eye;lamp.rotation_euler=cam.rotation_euler
    for o in scene.objects:
        if o.type=='LIGHT' and o!=lamp and o.name not in ('Sun_Key','Area_Fill'):o.hide_render=(o.location-eye).length>24
    name=f'{i:02d}_'+re.sub(r'[^a-zA-Z0-9_-]+','_',w['name'])+'_'+tag
    scene.render.filepath=str(out/(name+'.png'))
    bpy.ops.render.render(write_still=True)
    manifest.append(dict(name=name,walk=w['name'],pass_walk=w['pass'],simulation_t=s['t'],eye_m=list(eye),fov_horizontal=profile['fov'],camera_profile=profile_name,nominal_camera=profile,camera_scope='Static inspection at measured capsule; no camera collision/spring/actor-yaw simulation',qa_fill=True,source_sha256=sha,image_sha256=hashlib.sha256(Path(scene.render.filepath).read_bytes()).hexdigest()))
    (out/'shots.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print('AUDIT_SHOT',i,len(shots),name,flush=True)
print('AUDIT_REVIEW_READY',len(shots),len(findings),flush=True)
