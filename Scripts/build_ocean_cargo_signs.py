"""C04: upright, supported hold entrance signs outside the walking opening."""
import bpy, hashlib, json, math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='0262764c031f83591a9ba23238231fe0eccad4d3fccc298b981ebaa34dd0a77b'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-C04';out.mkdir(parents=True,exist_ok=False)
changes=[]
for hold,entry,side in ((1,-12.9,-1),(2,41.1,1)):
    text=bpy.data.objects['R3_HoldStairSign'+str(hold)]
    text.data=text.data.copy();text.data.align_x='CENTER';text.data.align_y='CENTER';text.data.size=.105
    text.rotation_euler=(math.pi/2,0,0 if side<0 else math.pi)
    text.location=(entry-1.4,side*7.626,12.0)
    collection=text.users_collection[0]
    material=bpy.data.objects['R3_StairTurn0'].data.materials[0]
    for suffix,p,d in [('Board',(entry-1.4,side*7.60,12.0),(1.45,.045,.28)),
                       ('PostA',(entry-1.9,side*7.60,11.30),(.045,.045,1.44)),
                       ('PostB',(entry-.9,side*7.60,11.30),(.045,.045,1.44))]:
        bpy.ops.mesh.primitive_cube_add(size=1,location=p)
        obj=bpy.context.object;obj.name=f'C04_HoldSign{hold}_{suffix}';obj.dimensions=d
        bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
        for c in list(obj.users_collection):c.objects.unlink(obj)
        collection.objects.link(obj);obj.data.materials.append(material)
        bevel=obj.modifiers.new('Manufactured edges','BEVEL');bevel.width=.005;bevel.segments=2
    changes.append(dict(hold=hold,entry=entry,side=side,scope='Upright outward-facing text, board and two supports beside opening'))
bpy.context.scene['refit_revision']='R7-Cargo-C04-20260906'
bpy.context.scene['cargo_candidate_stage']='C04: C01-C03 clearance fixes plus supported upright hold signs'
path=out/'CalmyOceanFreighter-Cargo.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),changes=changes),indent=2),encoding='utf-8')
