"""Resolve measured Unreal held-box contacts against Blender evaluated meshes.
Read-only model inspection; writes a separate JSON report, never saves the blend.
"""
import bpy, json, os, math, hashlib
from pathlib import Path
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

ROOT=Path(__file__).resolve().parents[1]
label=os.environ['CALMY_CARRY_REPORT']
report=json.loads((ROOT/'Saved/OceanPhysics'/(label+'.json')).read_text(encoding='utf-8-sig'))
sha=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest()
assert sha==report['source_sha256']
out=ROOT/'Saved/Verification'/(label+'-Contacts')
out.mkdir(parents=True,exist_ok=False)
deps=bpy.context.evaluated_depsgraph_get()
objects=[]
for o in bpy.context.scene.objects:
    if o.type not in ('MESH','CURVE') or o.hide_render or any(c.hide_render for c in o.users_collection):continue
    e=o.evaluated_get(deps)
    bb=[e.matrix_world@Vector(v) for v in e.bound_box]
    objects.append((o,Vector(tuple(min(p[i] for p in bb) for i in range(3))),Vector(tuple(max(p[i] for p in bb) for i in range(3)))))
faces=[(0,1,3,2),(4,6,7,5),(0,4,5,1),(2,3,7,6),(0,2,6,4),(1,5,7,3)]
findings=[]
for w in report['walks']:
    if w['pass'] or w['reason']!='held_cargo_intersects_geometry':continue
    x,y,z=w['cargo_position'];centre=Vector((x/100,-y/100,z/100))
    extent=Vector((.565,.44,.365)) if w['cargo_kind']=='Bulky' else Vector((.34,.34,.34))
    rot=Matrix.Rotation(-math.radians(w['cargo_yaw']),4,'Z')
    verts=[centre+rot.to_3x3()@Vector((a*extent.x,b*extent.y,c*extent.z)) for a in (-1,1) for b in (-1,1) for c in (-1,1)]
    lo=Vector(tuple(min(p[i] for p in verts) for i in range(3)));hi=Vector(tuple(max(p[i] for p in verts) for i in range(3)))
    box=BVHTree.FromPolygons(verts,faces)
    contacts=[]
    for o,ol,oh in objects:
        if any(oh[i]<lo[i] or ol[i]>hi[i] for i in range(3)):continue
        ev=o.evaluated_get(deps);mesh=ev.to_mesh();mesh.calc_loop_triangles()
        tree=BVHTree.FromPolygons([ev.matrix_world@v.co for v in mesh.vertices],[tuple(t.vertices) for t in mesh.loop_triangles],all_triangles=True)
        overlap=box.overlap(tree)
        if overlap:contacts.append(dict(object=o.name,surface_intersections=len(overlap),bounds_min=list(ol),bounds_max=list(oh)))
        ev.to_mesh_clear()
    findings.append(dict(route=w['name'],cargo_centre_m=list(centre),cargo_yaw_blender=-w['cargo_yaw'],contacts=contacts))
(out/'contacts.json').write_text(json.dumps(dict(source_sha256=sha,scope='Surface BVH intersection at measured held-box transform; not a new movement test',findings=findings),indent=2),encoding='utf-8')
print(json.dumps(findings,indent=2),flush=True)
