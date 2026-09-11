"""Exact evaluated collision export for the opt-in Unreal physical walking lab.
Not production render export. Converts metres/right-handed Y to Unreal cm/-Y.
"""
import bpy,json,hashlib,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'Saved'/'OceanPhysics';out.mkdir(parents=True,exist_ok=True)
scene=bpy.context.scene;deps=bpy.context.evaluated_depsgraph_get()
chunks={}
for obj in scene.objects:
    if obj.type not in ('MESH','CURVE') or obj.hide_render or any(c.hide_render for c in obj.users_collection):continue
    key=obj.users_collection[0].name
    chunk=chunks.setdefault(key,dict(name=key,vertices=[],triangles=[],owners=[]))
    ev=obj.evaluated_get(deps);mesh=ev.to_mesh();mesh.calc_loop_triangles()
    base=len(chunk['vertices']);first=len(chunk['triangles'])//3
    for v in mesh.vertices:
        p=ev.matrix_world@v.co
        chunk['vertices'].append([round(p.x*100,3),round(-p.y*100,3),round(p.z*100,3)])
    for t in mesh.loop_triangles:
        # Reflection in Y already reverses Blender's CCW order to Unreal's CW.
        # Flipping again produces inward Chaos normals and false stair failures.
        a,b,c=t.vertices;chunk['triangles'].extend([base+a,base+b,base+c])
    chunk['owners'].append(dict(name=obj.name,first=first,count=len(mesh.loop_triangles)))
    ev.to_mesh_clear()
routes=json.loads(scene['route_contracts'])
for room in json.loads(scene['room_layout']):
    x,y,z=room['door'];routes.append(dict(name=room['name']+'_entrance',points=[(x,0,z),(x,room['side']*3.6,z)]))
if os.environ.get('CALMY_TRAVERSAL_AUDIT')=='1':
    import runpy
    extra=runpy.run_path(str(ROOT/'Scripts'/'ocean_traversal_audit.py'))['build_routes'](routes,json.loads(scene['room_layout']))
    (out/'audit-itineraries.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest(),units='Blender metres',routes=extra),indent=2),encoding='utf-8')
    routes+=extra
for r in routes:r['points']=[[p[0]*100,-p[1]*100,p[2]*100] for p in r['points']]
data=dict(source=bpy.data.filepath,sha256=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest(),
          revision=scene.get('refit_revision'),units='Unreal cm; Blender Y negated; source index order retained for UE clockwise faces',
          collision_scope='all visible mesh and beveled curves INCLUDING glass; no simplification',
          chunks=list(chunks.values()),routes=routes)
path=out/'ship.json';path.write_text(json.dumps(data,separators=(',',':')),encoding='utf-8')
print('OCEAN_PHYSICS_EXPORT',str(path),'routes',len(routes),'triangles',sum(len(c['triangles'])//3 for c in chunks.values()),flush=True)
