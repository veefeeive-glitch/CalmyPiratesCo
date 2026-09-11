"""C02 candidate: supported cargo turns; retain all exposed-edge protection.
Separate output, never replaces master. Requires full movement and visual review.
"""
import bpy, hashlib, json
from pathlib import Path
from mathutils import Matrix, Vector
ROOT=Path(__file__).resolve().parents[1]
source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='79958c4c180a1f1e880fa0ce5c6ca45be8c80d7c340c64a9d476a8e96b6e1d7b'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-C02'
assert not out.exists() or not any(out.iterdir()),'Preserve existing candidate'
out.mkdir(parents=True,exist_ok=True)
changes=[]
def shift_prefix(prefix,dx):
    selected=[o for o in bpy.data.objects if o.name.startswith(prefix)]
    assert selected,prefix
    for o in selected:o.location.x+=dx
    changes.append(dict(prefix=prefix,dx=dx,count=len(selected)))
def extend_floor(obj,dx,width):
    obj.location.x+=dx;obj.dimensions.x=width
    bpy.context.view_layer.update()

# Move a complete dining set, retaining chair/table relationships.
for prefix in ('R3_R3_Room0_Table1','R3_R3_Room0_Leg(1,','R3_Room0_Chair(1,','R3_Room0_Plate(1,'):
    shift_prefix(prefix,.4)

# Enlarge only the supported intermediate turn, not the inclined flights.
for i in range(3):
    extend_floor(bpy.data.objects['R3_StairTurn'+str(i)],.2,1.3)
    shift_prefix('R3_StairTurnGuard'+str(i),.4)

# Extend the existing stairwell cutout over the new turn so it has headroom.
for name in ('R3_Boat_Ceiling','R3_Crew_Floor','R3_Crew_Ceiling','R3_Officers_Floor','R3_Officers_Ceiling','R3_Bridge_Floor'):
    obj=bpy.data.objects[name]
    bpy.ops.mesh.primitive_cube_add(size=1,location=(-42.23,4.665,obj.location.z))
    cutter=bpy.context.object;cutter.name='C02_HeadroomCutter';cutter.dimensions=(.5,3.30,.6)
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    mod=obj.modifiers.new('C02_extended_turn_headroom','BOOLEAN');mod.operation='DIFFERENCE';mod.solver='EXACT';mod.object=cutter
    bpy.context.view_layer.objects.active=obj;obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=mod.name)
    bpy.data.objects.remove(cutter,do_unlink=True)

for hold,x0,x1 in ((1,-19.5,-12.5),(2,34.5,41.5)):
    for level in range(3):
        for xx in (x0+.38,x1-.38):
            sign=-1 if xx<x0+1 else 1
            key=str((hold,level,xx))
            landing=bpy.data.objects['R3_HoldLanding'+key]
            extend_floor(landing,sign*.3,1.43)
            shift_prefix('R3_HoldLandingEnd'+key,sign*.6)
            # Two girders below each widened landing; no obstruction on walking surface.
            for y in (landing.location.y-1.1,landing.location.y+1.1):
                bpy.ops.mesh.primitive_cube_add(size=1,location=(landing.location.x,y,landing.location.z-.14))
                beam=bpy.context.object;beam.name='C02_LandingBeam'+key+str(y);beam.dimensions=(1.43,.12,.12)
                if landing.data.materials:beam.data.materials.append(landing.data.materials[0])
                for c in list(beam.users_collection):c.objects.unlink(beam)
                landing.users_collection[0].objects.link(beam)
        # Extend the inner safety rail to meet the relocated end guards.
        centre=(x0+x1)/2
        transform=Matrix.Translation((centre,0,0))@Matrix.Diagonal((1.0+1.2/(x1-x0),1,1,1))@Matrix.Translation((-centre,0,0))
        for obj in bpy.data.objects:
            if obj.name.startswith('R3_HoldInnerGuard'+str((hold,level))):obj.matrix_world=transform@obj.matrix_world
bpy.context.scene['cargo_candidate_stage']='C02: supported wider internal/hold turns, extended headroom and guards; dining set relocated'
bpy.context.scene['refit_revision']='R7-Cargo-C02-20260906'
path=out/'CalmyOceanFreighter-Cargo.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),changes=changes,scope='Candidate only: walking and cargo regressions plus visual review required'),indent=2),encoding='utf-8')
print('C02_CANDIDATE_READY',path,flush=True)
