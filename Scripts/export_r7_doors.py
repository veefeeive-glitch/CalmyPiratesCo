"""Separate all 61 passage leaves from static superstructure; preserve master and other faces."""
import bpy,bmesh,json,hashlib
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1];out=root/'SourceAssets/Generated/ShipDoorsR1'
assert not out.exists();out.mkdir(parents=True)
master=root/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
assert hashlib.sha256(master.read_bytes()).hexdigest()=='8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
bpy.ops.wm.open_mainfile(filepath=str(master));scene=bpy.context.scene;deps=bpy.context.evaluated_depsgraph_get()
leaves=[o for o in scene.objects if o.name.endswith('_OpenLeaf')]
assert len(leaves)==61
remove=set();doors=[]
for i,o in enumerate(sorted(leaves,key=lambda o:o.name)):
    prefix=o.name.removesuffix('_OpenLeaf');j=[bpy.data.objects[prefix+'_Jamb'+str(s)] for s in (-1,1)]
    center=(j[0].location+j[1].location)*.5;floor=o.location.z-1.05
    axis='Y' if o.dimensions.x>o.dimensions.y else 'X';w=max(o.dimensions.x,o.dimensions.y)
    pivot=[center.x,-center.y,floor]
    if axis=='Y':pivot[0]+=w/2;pivot[1]-=.13
    else:pivot[0]+=.13;pivot[1]-=w/2
    doors.append(dict(id=i+1,name=prefix,pivot_cm=[v*100 for v in pivot],yaw=0 if axis=='Y' else -90,width_cm=w*100,height_cm=210))
    remove.add(o.name)
    if prefix+'_Handle' in bpy.data.objects:remove.add(prefix+'_Handle')
copies=[];removed_tri=0;kept_tri=0
for o in list(scene.objects):
    if o.type not in ('MESH','CURVE','FONT') or o.hide_render or any(c.hide_render for c in o.users_collection):continue
    if not o.users_collection or o.users_collection[0].name!='30_SUPERSTRUCTURE':continue
    data=bpy.data.meshes.new_from_object(o.evaluated_get(deps),depsgraph=deps)
    if o.name in remove:
        data.calc_loop_triangles();removed_tri+=len(data.loop_triangles);bpy.data.meshes.remove(data);continue
    glass={i for i,m in enumerate(data.materials) if m and m.name=='MAT_Bridge_Glass'}
    bm=bmesh.new();bm.from_mesh(data);bmesh.ops.delete(bm,geom=[f for f in bm.faces if f.material_index in glass],context='FACES');bm.to_mesh(data);bm.free()
    if not data.polygons:bpy.data.meshes.remove(data);continue
    used=sorted({p.material_index for p in data.polygons});mats=[data.materials[i] for i in used];remap={v:i for i,v in enumerate(used)};indices=[remap[p.material_index] for p in data.polygons]
    data.materials.clear()
    for m in mats:data.materials.append(m)
    for p,i in zip(data.polygons,indices):p.material_index=i
    c=bpy.data.objects.new('export_'+o.name,data);scene.collection.objects.link(c);c.matrix_world=o.matrix_world.copy();copies.append(c)
bpy.ops.object.select_all(action='DESELECT')
for o in copies:o.select_set(True)
bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0];joined.name='SM_R7_30_SUPERSTRUCTURE_DoorsR1'
scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR');bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
joined.data.calc_loop_triangles();kept_tri=len(joined.data.loop_triangles)
bpy.ops.export_scene.gltf(filepath=str(out/(joined.name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
(out/'doors.json').write_text(json.dumps(dict(doors=doors,removed_objects=sorted(remove),removed_triangles=removed_tri,static_triangles=kept_tri,master_sha256=hashlib.sha256(master.read_bytes()).hexdigest()),indent=2))
print('DOORS_EXPORT_PASS doors=61 removed='+str(len(remove))+' triangles='+str(kept_tri))
