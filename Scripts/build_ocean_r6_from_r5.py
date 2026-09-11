"""Fast R6 iteration against frozen R5, using authoritative generator primitives.

Only function definitions are loaded from the full generator, never its scene
reset or top-level modelling operations. Full generator remains supported.
"""
import ast,bpy,json,math,runpy,hashlib
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
source=Path(bpy.data.filepath)
assert hashlib.sha256(source.read_bytes()).hexdigest()=='d4970d36b1fc0dfcb6ad0af95b8684b4a79849b471f740d8a1539402ca845bda','Expected frozen R5 master'
tree=ast.parse((ROOT/'Scripts/create_ocean_freighter.py').read_text(encoding='utf-8'))
g=dict(bpy=bpy,math=math,Vector=Vector,ROOT=ROOT)
exec(compile(ast.Module(body=[n for n in tree.body if isinstance(n,ast.FunctionDef)],type_ignores=[]),'<generator primitives>','exec'),g)
matnode=next(n.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='MAT' for t in n.targets))
g['MAT']={ast.literal_eval(k):bpy.data.materials[ast.literal_eval(v.args[0])] for k,v in zip(matnode.keys,matnode.values)}
g['COL']={c.name:c for c in bpy.data.collections}
runpy.run_path(str(ROOT/'Scripts/ocean_freighter_circulation.py'))['apply_circulation'](g)
out=ROOT/'SourceAssets/Generated/OceanFreighter/R6-Candidate';out.mkdir(exist_ok=True)
bpy.ops.wm.save_as_mainfile(filepath=str(out/'CalmyOceanFreighter.blend'))
manifest=json.loads((source.parent/'build_manifest.json').read_text(encoding='utf-8'))
for key in ('qa_report','physical_qa_report','review_shots','reviewed_candidate_sha256','promoted_master_sha256','preview','cutaway_preview','access_preview','bow_preview','stern_preview'):
    manifest.pop(key,None)
manifest.update(master_blend=str(out/'CalmyOceanFreighter.blend'),status=bpy.context.scene['design_status'],
                build_mode='R6 module applied to hash-pinned R5; generator primitives reused',
                previews_generated=False)
(out/'build_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('R6_INCREMENTAL_SAVED',flush=True)
