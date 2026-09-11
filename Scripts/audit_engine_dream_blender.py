"""Read-only raycast into unchanged master to identify real visible surfaces."""
import bpy,json
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'))
deps=bpy.context.evaluated_depsgraph_get()
rows=[]
for point in [(-45,-3,3.7),(-43,-3,3.7),(-47,-3,3.7),(-45,-3,6.0)]:
    for direction in [(0,0,-1),(0,0,1)]:
        hit,loc,n,face,obj,m=bpy.context.scene.ray_cast(deps,Vector(point),Vector(direction))
        if hit:
            ev=obj.evaluated_get(deps);mat=ev.data.materials[ev.data.polygons[face].material_index] if face<len(ev.data.polygons) else None
            rows.append({'origin':point,'direction':direction,'hit':list(loc),'object':obj.name,'material':mat.name if mat else None})
for obj in bpy.context.scene.objects:
    if any(mat and mat.name=='MAT_R4_MachineryLight' for mat in getattr(obj.data,'materials',[])):
        rows.append({'fixture':obj.name,'xyz':list(obj.location),'dimensions':list(obj.dimensions)})
(root/'Saved/Verification/EngineDream-R1-20260908/blender-surface-audit.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
print('ENGINE_DREAM_BLENDER_AUDIT_PASS '+json.dumps(rows[:8]))
