"""C05: contrast-only correction after inspecting the two C04 sign closeups."""
import bpy, hashlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='1148c8ffa0eca999428080b94a696de3db34cad140e5cb667bac170ef5741e85'
out=ROOT/'SourceAssets/Generated/OceanFreighter/R7-Cargo-C05';out.mkdir(parents=True,exist_ok=False)
mat=bpy.data.materials.new('C05_SignNavy');mat.use_nodes=True;mat.diffuse_color=(.015,.06,.085,1)
bsdf=mat.node_tree.nodes.get('Principled BSDF');bsdf.inputs['Base Color'].default_value=mat.diffuse_color;bsdf.inputs['Roughness'].default_value=.65
for hold in (1,2):
    obj=bpy.data.objects[f'C04_HoldSign{hold}_Board'];obj.data.materials.clear();obj.data.materials.append(mat)
bpy.context.scene['refit_revision']='R7-Cargo-C05-20260906'
bpy.context.scene['cargo_candidate_stage']='C05: C01-C04 fixes, legible navy sign backboards'
path=out/'CalmyOceanFreighter-Cargo.blend';bpy.ops.wm.save_as_mainfile(filepath=str(path))
(out/'change-manifest.json').write_text(json.dumps(dict(base_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),candidate_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),scope='Two sign board materials only; no geometry/route changes'),indent=2),encoding='utf-8')
