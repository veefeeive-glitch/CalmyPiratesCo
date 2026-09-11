"""C06: move sign posts behind backboards, eliminating coplanar face flicker."""
import bpy,hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='4af9a924decb3d322a4506fe514594764571dee8576311187f41b392676ec70f'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-C06';out.mkdir(parents=True,exist_ok=False)
for hold,side in ((1,-1),(2,1)):
    for suffix in ('PostA','PostB'):bpy.data.objects[f'C04_HoldSign{hold}_{suffix}'].location.y=side*7.56
bpy.context.scene['refit_revision']='R7-Cargo-C06-20260906'
bpy.context.scene['cargo_candidate_stage']='C06: reviewed cargo clearance patch; sign posts behind boards'
path=out/'CalmyOceanFreighter-Cargo.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),scope='Four sign posts moved 4cm inboard, behind signboards'),indent=2),encoding='utf-8')
