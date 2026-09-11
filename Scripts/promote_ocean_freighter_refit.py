"""Promote a reviewed candidate with exact-file QA; R5 also requires UE walking.

Run headless with the candidate as the input .blend and CALMY_QA_LABEL set.
This verifies the regression screen, not final production acceptance.
"""
from pathlib import Path
import bpy
import os
import json
import hashlib

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'SourceAssets'/'Generated'/'OceanFreighter'
candidate=Path(bpy.data.filepath).resolve()
revision=os.environ.get('CALMY_PROMOTE_REVISION','R5')
assert revision in ('R3','R4','R5'),'Unrecognised candidate revision'
assert candidate== (OUT/(revision+'-Candidate')/'CalmyOceanFreighter.blend').resolve(),candidate
assert bpy.context.scene.get('refit_revision','').startswith(revision+'-'),'Scene revision mismatch'
qa=ROOT/'Saved'/'Verification'/os.environ.get('CALMY_QA_LABEL','OceanFreighter-'+revision+'-Final')
report=json.loads((qa/'geometry-routes.json').read_text(encoding='utf-8'))
assert report['source_sha256']==hashlib.sha256(candidate.read_bytes()).hexdigest(),'QA is for a different file'
assert report['route_count']>={'R3':78,'R4':86,'R5':88}[revision] and report['clear_routes']==report['route_count'],'Route regression is not clear'
assert report.get('deck_fitting_room_intrusions')==[],'Deck fittings still intrude into rooms'
if revision in ('R4','R5'):
    assert len(report.get('machinery_assembly_checks',[]))==4 and all(c['pass'] for c in report['machinery_assembly_checks']),'Pump assembly checks failed'
    assert 'curve' in report['method'],'QA must include beveled pipe curves'
shots=json.loads((qa/'shots.json').read_text(encoding='utf-8'))
assert len(shots)>=16 and all((qa/(s['name']+'.png')).is_file() for s in shots),'Missing visual review outputs'
assert {str(i).zfill(2) for i in range(1,{'R3':17,'R4':23,'R5':26}[revision])} <= {s['name'].split('_')[0] for s in shots},'Incomplete shot coverage'
assert all(s.get('source_sha256')==report['source_sha256'] for s in shots),'Stale visual manifest'
assert all(hashlib.sha256((qa/(s['name']+'.png')).read_bytes()).hexdigest()==s['image_sha256'] for s in shots),'Image hash mismatch'
backup=OUT/'Backup-20260905-Refit'/'CalmyOceanFreighter.blend'
assert backup.is_file(),'Pre-refit backup missing'
if revision in ('R4','R5'):
    assert (OUT/'Backup-20260905-R3'/'CalmyOceanFreighter.blend').is_file(),'R3 backup missing'
if revision=='R5':
    assert (OUT/'Backup-20260905-R4'/'CalmyOceanFreighter.blend').is_file(),'R4 backup missing'
    physics_path=ROOT/'Saved'/'OceanPhysics'/(os.environ.get('CALMY_PHYSICS_REPORT','R5-final')+'.json')
    physics=json.loads(physics_path.read_text(encoding='utf-8-sig'))
    assert physics['source_sha256']==report['source_sha256'],'Physics evidence is for a different model'
    assert physics['total']>=180 and physics['passed']==physics['total'],'Physical walking failures remain'
    assert physics['controls_total']==4 and physics['controls_passed']==4,'Physics controls failed'
    required={r['name']+suffix for r in report['routes'] for suffix in ('_forward','_reverse')}
    assert required<={w['name'] for w in physics['walks'] if w['pass']},'Missing physical route coverage'
    bpy.context.scene['physical_qa_report']=str(physics_path)
scene=bpy.context.scene
scene['qa_report']=str(qa/'geometry-routes.json')
scene['design_status']=revision+' Blender authoring refit; not production complete; Unreal acceptance outstanding'
scene['source_reference']='Original game arrangement; not a certified naval architecture design'
# First-person review defaults; studio material mode remains available to users.
camera=bpy.data.objects.get('CAM_Walkthrough_FirstPerson')
scene.camera=camera
for workspace in bpy.data.workspaces:
    for screen in workspace.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.show_region_ui=True
                area.spaces.active.region_3d.view_perspective='CAMERA'
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CalmyOceanFreighter.blend'))
manifest=json.loads((candidate.parent/'build_manifest.json').read_text(encoding='utf-8'))
manifest.update(master_blend=str(OUT/'CalmyOceanFreighter.blend'),status=scene['design_status'],
                preview=str(qa/'01_Overview.png'),qa_report=str(qa/'geometry-routes.json'),
                qa_scope='Blender sampled route geometry only; NOT Unreal collision or production acceptance',
                reviewed_candidate_sha256=report['source_sha256'],
                promoted_master_sha256=hashlib.sha256((OUT/'CalmyOceanFreighter.blend').read_bytes()).hexdigest())
for key in ('cutaway_preview','stern_preview','access_preview','bow_preview'):
    manifest[key]=None
manifest['previews_generated']=True
manifest['review_shots']=str(qa/'shots.json')
if revision=='R5':
    manifest['physical_qa_report']=str(physics_path)
    manifest['qa_scope']='Blender ray screen + Unreal isolated CharacterMovement/Chaos route lab; NOT multiplayer traversal or production acceptance'
(OUT/'build_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('R3_PROMOTED '+json.dumps(manifest),flush=True)
