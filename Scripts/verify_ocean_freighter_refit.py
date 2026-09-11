"""Inspect actual evaluated Blender geometry, not mesh counts.

Ray sampling is a useful regression screen, NOT a swept Unreal capsule test.
Does not modify/save the .blend. CALMY_QA_RENDER=1 also renders closed-scene views.
"""
from pathlib import Path
import bpy
import json
import math
import os
import hashlib
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Saved'/'Verification'/os.environ.get('CALMY_QA_LABEL','OceanFreighter-R3')
OUT.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene
deps=bpy.context.evaluated_depsgraph_get()
vertices=[]; faces=[]; owners=[]
for obj in scene.objects:
    # Curves with bevel are real solid pipes/rails. Ignoring them would let
    # a newly authored pipe cross the player's head while the test stayed green.
    if obj.type not in ('MESH','CURVE') or obj.hide_render or any(c.hide_render for c in obj.users_collection): continue
    if any(m and ('Glass' in m.name or 'Screen' in m.name) for m in obj.data.materials): continue
    evaluated=obj.evaluated_get(deps)
    me=evaluated.to_mesh()
    offset=len(vertices)
    vertices.extend(evaluated.matrix_world@v.co for v in me.vertices)
    for p in me.polygons:
        faces.append(tuple(offset+i for i in p.vertices));owners.append(obj.name)
    evaluated.to_mesh_clear()
bvh=BVHTree.FromPolygons(vertices,faces)

def cast(p,d,maxdist):
    loc,normal,index,dist=bvh.ray_cast(Vector(p),Vector(d),maxdist)
    return None if index is None else dict(object=owners[index],distance=round(dist,4),point=list(loc))

routes=json.loads(scene.get('route_contracts','[]'))
rooms=json.loads(scene.get('room_layout','[]'))
for room in rooms:
    x,y,z=room['door']; side=room['side']
    routes.append(dict(name=room['name']+'_entrance',points=[(x,0,z),(x,side*3.6,z)]))
results=[]
for route in routes:
    failures=[];samples=0
    for start,end in zip(route['points'],route['points'][1:]):
        a,z=Vector(start),Vector(end)
        steps=max(1,math.ceil((z-a).length/.22))
        for i in range(steps+1):
            p=a.lerp(z,i/steps);samples+=1
            # Tiny bevel cracks at adjoining treads are not capsule-sized holes.
            floors=[cast(p+Vector((dx,dy,.26)),(0,0,-1),.59) for dx,dy in ((0,0),(.015,0),(-.015,0),(0,.015),(0,-.015))]
            floors=[f for f in floors if f]
            floor=min(floors,key=lambda f:abs(f['point'][2]-p.z)) if floors else None
            if not floor:
                failures.append(dict(kind='missing_floor',at=list(p)));continue
            foot=Vector(floor['point'])
            for dx,dy in ((0,0),(.24,0),(-.24,0),(0,.24),(0,-.24)):
                hit=cast(foot+Vector((dx,dy,.4)),(0,0,1),1.47)
                if hit:
                    failures.append(dict(kind='head_clearance',at=list(p),hit=hit));break
            else:
                for h in (.45,1.0,1.6):
                    blocked=False
                    for angle in range(0,360,45):
                        q=math.radians(angle)
                        hit=cast(foot+Vector((0,0,h)),(math.cos(q),math.sin(q),0),.30)
                        if hit:
                            failures.append(dict(kind='body_clearance',at=list(p),hit=hit));blocked=True;break
                    if blocked: break
    # Compact distinct obstacle records, preserve sample failure count.
    unique={}
    for f in failures:
        key=f['kind']+f.get('hit',{}).get('object','')
        if key not in unique: unique[key]=f
    results.append(dict(name=route['name'],samples=samples,failed_samples=len(failures),obstacles=list(unique.values())))
report=dict(revision=scene.get('refit_revision'),source=bpy.data.filepath,
    source_sha256=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest(),
    method='evaluated mesh AND beveled curve triangles; .22m sample pitch; .30m radial rays, 1.87m head envelope; NOT Unreal physics',
    route_count=len(results),clear_routes=sum(r['failed_samples']==0 for r in results),routes=results)
# Separate placement invariant catches equipment outside the sampled walking
# line but INSIDE living rooms. A route-only PASS must not conceal this defect.
intrusions=[]
for obj in scene.objects:
    if not any(c.name=='60_DECK_FITTINGS' for c in obj.users_collection) or obj.hide_render: continue
    pts=[obj.matrix_world@Vector(v) for v in obj.bound_box]
    lo=[min(p[k] for p in pts) for k in range(3)];hi=[max(p[k] for p in pts) for k in range(3)]
    for room in rooms:
        if room['floor']!=10.67: continue
        rlo,rhi=(1.3,room['width']-.1) if room['side']>0 else (-room['width']+.1,-1.3)
        if min(hi[0],room['x1']-.1)>max(lo[0],room['x0']+.1) and min(hi[1],rhi)>max(lo[1],rlo) and min(hi[2],13.1)>max(lo[2],10.72):
            intrusions.append(dict(object=obj.name,room=room['name']))
report['deck_fitting_room_intrusions']=intrusions
assembly=[]
if scene.get('refit_revision','').startswith(('R4-','R5-','R6-','R7-')):
    def bounds(name):
        obj=bpy.data.objects[name].evaluated_get(deps)
        pts=[obj.matrix_world@Vector(v) for v in obj.bound_box]
        return ([min(p[k] for p in pts) for k in range(3)],
                [max(p[k] for p in pts) for k in range(3)])
    for i in range(1,5):
        names=['R4_P'+str(i)+'_'+s for s in ('Motor','MotorFeet','Base','Volute','Shaft')]
        present=all(n in bpy.data.objects for n in names)
        checks=dict(name='pump_'+str(i)+'_assembly',objects_present=present)
        if present:
            motor,feet,base,volute,shaft=[bpy.data.objects[n] for n in names]
            mb,fb,bb=[bounds(n) for n in names[:3]]
            checks['feet_contact_base']=fb[0][2]<=bb[1][2]+.001 and fb[1][2]>=bb[1][2]
            checks['feet_contact_motor']=fb[1][2]>=mb[0][2]-.001
            checks['coaxial_motor_volute_shaft']=all(abs(o.location.y-motor.location.y)<.001 and abs(o.location.z-motor.location.z)<.001 for o in (volute,shaft))
            checks['horizontal_axes']=all(abs((o.matrix_world.to_3x3()@Vector((0,0,1))).normalized().x)>.999 for o in (motor,volute,shaft))
        checks['pass']=all(v for k,v in checks.items() if k!='name')
        assembly.append(checks)
report['machinery_assembly_checks']=assembly
(OUT/'geometry-routes.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('R3_GEOMETRY '+json.dumps({k:v for k,v in report.items() if k!='routes'}),flush=True)
for r in results:
    if r['failed_samples']: print('BLOCKED '+r['name']+' '+json.dumps(r['obstacles']),flush=True)
if intrusions: print('FITTING_INTRUSIONS '+json.dumps(intrusions),flush=True)

if os.environ.get('CALMY_QA_RENDER')=='1':
    scene.render.engine='BLENDER_EEVEE'
    scene.render.resolution_x=1280;scene.render.resolution_y=720;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG'
    scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value=.5
    # Dedicated low-intensity inspection headlamp, no geometry cutaway.
    bpy.ops.object.light_add(type='AREA')
    lamp=bpy.context.object;lamp.name='QA_InspectionFill';lamp.data.energy=45;lamp.data.size=2.5
    bpy.ops.object.camera_add()
    camera=bpy.context.object;camera.data.clip_start=.035;camera.data.clip_end=500
    scene.camera=camera
    shots=[
        ('01_Overview',(-105,-155,64),(-2,0,11),42),
        ('02_Bridge',(-39,0,20.6),(-31.5,0,20.2),24),
        ('03_BridgeConsole',(-35.9,-1.9,20.6),(-33.5,0,19.8),28),
        ('04_CrewCabin',(-47.7,-2.35,15.1),(-47.7,-6,14.25),24),
        ('05_CrewCorridor',(-53.8,0,15.1),(-33.4,0,15.05),25),
        ('06_Mess',(-44.4,-2.45,12.35),(-51,-5,11.6),24),
        ('07_Galley',(-39,2.35,12.35),(-39,7,11.9),24),
        ('08_Laundry',(-52,2.4,12.35),(-52,7,11.5),24),
        ('09_InternalStairs',(-48.65,2.4,12.35),(-44.5,4.7,12.4),22),
        ('10_Engine',(-49,-5.5,6.26),(-43,0,5.2),24),
        ('11_EngineAccess',(-24.6,-5.8,6.26),(-30,-5.8,3.5),23),
        ('12_Hold1',(-8,0,3.76),(-16,-4.7,5.7),23),
        ('13_Hold2',(27,0,3.76),(38,4.7,5.7),23),
        ('14_ExteriorTower',(-29,-22,20),(-35.5,-9.3,16),33),
        ('15_Sanitary',(-38,-2.0,12.35),(-40,-7,11.5),24),
        ('16_Medical',(-52,2.4,15.1),(-52,6.3,14.1),24),
        ('17_Generator',(-44,-4.1,3.76),(-46,-6,3.05),25),
        ('18_Pump',(-38.4,-3.9,3.65),(-40,-5.65,2.9),30),
        ('19_HeatExchanger',(-41.3,-4.75,6.15),(-43,-5.5,5.4),28),
        ('20_EngineCrankcase',(-45,-3.3,3.76),(-44,-1.8,4.1),23),
        ('21_EngineCylinder',(-44.5,-5.25,6.26),(-44,-1.8,6.45),30),
        ('22_GeneratorReverse',(-48.5,-4.1,3.7),(-46,-6,3.12),24),
        ('23_Workbench',(-30.5,7.12,3.75),(-29.5,8.05,3.2),22),
        ('24_FirePoint',(-30,-7.12,3.76),(-29.2,-8.02,3.1),20),
        ('25_CableReel',(-33.45,7.12,3.55),(-32.5,8.0,2.7),24),
        ('26_BoatExitPort',(-32.7,-6.4,12.19),(-32.7,-10.0,11.4),20),
        ('27_BoatExitStarboard',(-32.7,6.4,12.19),(-32.7,10.0,11.4),20),
        ('28_ForwardConnection',(-34.0,-8.5,12.19),(-29,-8.5,11.5),22),
        ('29_AftConnection',(-38.0,8.5,12.19),(-42,8.5,11.3),22),
        ('30_EngineAftCrosswalk',(-51.3,-4.8,6.10),(-51.3,2.0,5.15),20),
        ('31_EngineTweenCrosswalk',(-51.3,4.8,8.85),(-51.3,-2.0,7.85),20),
        ('32_ThrusterServiceApron',(51.4,-2.3,3.60),(50.0,0,3.0),20),
        ('33_MedicalClose',(-50.5,4.8,14.94),(-52.4,6.6,14.15),24),
        ('34_RadioClose',(-50.75,4.85,17.69),(-50.75,6.5,17.2),24),
        ('35_SternPassage',(-55.8,8.5,12.10),(-55.8,3.5,11.2),20),
    ]
    selected=os.environ.get('CALMY_QA_SHOTS','')
    manifest=[]
    for name,p,target,lens in shots:
        if selected and name.split('_')[0] not in selected.split(','):continue
        camera.location=p;camera.data.lens=lens
        camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
        lamp.location=camera.location;lamp.rotation_euler=camera.rotation_euler
        # Local fixture culling avoids Eevee shadow-atlas overflow; relevant
        # lights retain shadows. This visibility is QA-only and never saved.
        for obj in scene.objects:
            if obj.type=='LIGHT' and obj not in (lamp,) and obj.name not in ('Sun_Key','Area_Fill'):
                obj.hide_render=(obj.location-camera.location).length>24
        scene.render.filepath=str(OUT/(name+'.png'))
        bpy.ops.render.render(write_still=True)
        manifest.append(dict(name=name,eye=p,target=target,lens=lens,geometry_hidden=False,
                             source_sha256=report['source_sha256'],
                             image_sha256=hashlib.sha256(Path(scene.render.filepath).read_bytes()).hexdigest()))
        print('R3_RENDERED '+name,flush=True)
    (OUT/'shots.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')

if os.environ.get('CALMY_QA_STRICT')=='1' and (report['clear_routes']!=report['route_count'] or intrusions or any(not c['pass'] for c in assembly)):
    raise RuntimeError('R3 route regression: review geometry-routes.json')
