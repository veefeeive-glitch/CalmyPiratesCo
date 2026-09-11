"""Stage C01: move complete lower-deck cooler assemblies off the cargo aisle.
Hash-pinned R6 input; separate candidate output; no master/GUI-session overwrite.
"""
import bpy, hashlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
original=Path(bpy.data.filepath)
assert hashlib.sha256(original.read_bytes()).hexdigest()=='9ceafe214b04051c293734a81c31ab81e7daf109cfa73f82a8525a38ab759fdd'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-Candidate'
out.mkdir(parents=True,exist_ok=False)
moved=[]
for index,side in ((1,-1),(2,1)):
    group=[o for o in bpy.context.scene.objects if o.name.startswith(f'R4_HX{index}_')]
    assert len(group)>50,'Missing cooler assembly'
    for obj in group:
        assert obj.parent is None,'Avoid applying a translation twice through parenting'
        obj.location.y+=side*.30
        moved.append(obj.name)
bpy.context.scene['cargo_candidate_stage']='C01: complete HX assemblies moved 0.30m outboard; rail protection unchanged'
path=out/'CalmyOceanFreighter-Cargo.blend'
bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(original.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),scope='Only two complete lower-engine cooler assemblies translated; routes and rails unchanged',moved=moved),indent=2),encoding='utf-8')
print('CARGO_CANDIDATE_READY',str(path),len(moved),flush=True)
