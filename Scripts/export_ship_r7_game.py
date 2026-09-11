"""Hash-pinned static review export; NEVER saves/changes the Blender master."""
from pathlib import Path
import bpy, json, hashlib, re, sys, bmesh
from mathutils import Matrix
ROOT=Path(__file__).resolve().parents[1]
SPLIT='--split-glass' in sys.argv
OUT=ROOT/('SourceAssets/Generated/ShipR7RenderB' if SPLIT else 'SourceAssets/Generated/ShipR7Game')
MASTER=ROOT/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
HASH='8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
assert hashlib.sha256(MASTER.read_bytes()).hexdigest()==HASH
assert not OUT.exists(),'Preserve existing export'
OUT.mkdir(parents=True)
scene=bpy.context.scene;deps=bpy.context.evaluated_depsgraph_get()
sources=[o for o in scene.objects if o.type in ('MESH','CURVE','FONT') and not o.hide_render and not any(c.hide_render for c in o.users_collection)]
groups={}
for o in sources:groups.setdefault(o.users_collection[0].name,[]).append(o)
if SPLIT:
    groups={group+suffix:items for group,items in groups.items() if group in ('30_SUPERSTRUCTURE','50_CARGO_EQUIPMENT','60_DECK_FITTINGS') for suffix in ('_Opaque','_Glass')}
hull=bpy.data.objects['Hull_Complete'];stations={};vertical={}
for v in hull.data.vertices:
    p=hull.matrix_world@v.co;x=round(p.x,3)
    stations[x]=max(stations.get(x,0),abs(p.y))
    if abs(p.x)<.001:vertical[round(p.z,3)]=max(vertical.get(round(p.z,3),0),abs(p.y)/10.2)
assert len(stations)>10 and len(vertical)>4
manifest={'source_sha256':HASH,'revision':scene.get('refit_revision'),'units':'GLB metres -> UE centimetres; Blender Y reflected by importer',
          'hull_stations_cm':[[x*100,w*100] for x,w in sorted(stations.items())],
          'hull_vertical_cm':[[z*100,f] for z,f in sorted(vertical.items())],
          'routes':json.loads(scene['route_contracts']),'rooms':json.loads(scene['room_layout']),
          'lights':[{'name':o.name,'p':[o.location.x*100,-o.location.y*100,o.location.z*100],'energy':o.data.energy,'color':list(o.data.color)} for o in scene.objects if o.type=='LIGHT' and o.name.startswith('InteriorLight')],
          'chunks':[]}
# Only temporary evaluated copies are joined. Hidden collision/LOD/review helpers excluded.
for group,items in sorted(groups.items()):
    copies=[]
    for o in items:
        ev=o.evaluated_get(deps);data=bpy.data.meshes.new_from_object(ev,depsgraph=deps)
        if SPLIT:
            glass={i for i,m in enumerate(data.materials) if m and m.name=='MAT_Bridge_Glass'}
            bm=bmesh.new();bm.from_mesh(data)
            bmesh.ops.delete(bm,geom=[f for f in bm.faces if (f.material_index in glass)!=group.endswith('_Glass')],context='FACES')
            bm.to_mesh(data);bm.free()
            if not data.polygons:bpy.data.meshes.remove(data);continue
            used=sorted({p.material_index for p in data.polygons});remap={old:new for new,old in enumerate(used)}
            materials=[data.materials[i] for i in used]
            indices=[remap[p.material_index] for p in data.polygons]
            data.materials.clear()
            for m in materials:data.materials.append(m)
            for p,i in zip(data.polygons,indices):p.material_index=i
        copy=bpy.data.objects.new('export_'+o.name,data);scene.collection.objects.link(copy)
        copy.matrix_world=o.matrix_world.copy();copies.append(copy)
    bpy.ops.object.select_all(action='DESELECT')
    for o in copies:o.select_set(True)
    bpy.context.view_layer.objects.active=copies[0];bpy.ops.object.join();joined=copies[0]
    name='SM_R7_'+re.sub(r'[^A-Za-z0-9_]','_',group);joined.name=name
    scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    joined.data.calc_loop_triangles()
    manifest['chunks'].append({'name':name,'collection':group,'objects':len(items),'triangles':len(joined.data.loop_triangles),'materials':len(joined.data.materials)})
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    bpy.data.objects.remove(joined,do_unlink=True)
(OUT/'export_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
assert hashlib.sha256(MASTER.read_bytes()).hexdigest()==HASH
print('SHIP_R7_EXPORT_PASS',len(sources),'objects',len(groups),'chunks',sum(c['triangles'] for c in manifest['chunks']),'triangles',flush=True)
