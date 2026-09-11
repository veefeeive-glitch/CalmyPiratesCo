"""C03: widen aft stair lobby, with adjacent room boundary metadata updated."""
import bpy,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='79a58b10a16bb28e17c14d1440e7ae1d25584ff3067d1a52cd730d0d18f91eda'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-C03';out.mkdir(parents=True,exist_ok=False)
moved=[]
for deck in ('Boat','Crew','Officers'):
    for name in (f'R3_{deck}_Partition1_0',f'R3_{deck}_StairEnd-49.15'):
        obj=bpy.data.objects[name];obj.location.x-=.4;moved.append(name)
rooms=json.loads(bpy.context.scene['room_layout'])
for room in rooms:
    if room['side']==1 and abs(room['x1']+49.15)<.001:room['x1']-=.4
bpy.context.scene['room_layout']=json.dumps(rooms)
bpy.context.scene['refit_revision']='R7-Cargo-C03-20260906'
bpy.context.scene['cargo_candidate_stage']='C03: aft stair lobby +0.4m; corresponding laundry/medical/radio boundary shrunk, doors unchanged'
path=out/'CalmyOceanFreighter-Cargo.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),moved=moved,scope='Candidate, not promoted; requires geometry, cargo, full walking and visual review'),indent=2),encoding='utf-8')
