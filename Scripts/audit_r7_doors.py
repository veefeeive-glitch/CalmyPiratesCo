import bpy,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'))
rows=[]
for o in bpy.context.scene.objects:
    if 'OpenLeaf' in o.name or 'DoorLeaf' in o.name or 'DoorPanel' in o.name:
        pts=[o.matrix_world@Vector(v) for v in o.bound_box]
        rows.append(dict(name=o.name,p=list(o.location),dim=list(o.dimensions),rot=list(o.rotation_euler),col=[c.name for c in o.users_collection],lo=[min(p[k] for p in pts) for k in range(3)],hi=[max(p[k] for p in pts) for k in range(3)]))
out=root/'Saved/Verification/Doors-R1';out.mkdir(parents=True,exist_ok=True)
(out/'inventory.json').write_text(json.dumps(rows,indent=2))
print('DOOR_INVENTORY '+str(len(rows)))
for r in rows:print(r['name'],r['p'],r['dim'],r['col'])
