"""Promote the currently loaded candidate only after exact-file R3 QA evidence.

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
assert candidate== (OUT/'R3-Candidate'/'CalmyOceanFreighter.blend').resolve(),candidate
qa=ROOT/'Saved'/'Verification'/os.environ.get('CALMY_QA_LABEL','OceanFreighter-R3-Final')
report=json.loads((qa/'geometry-routes.json').read_text(encoding='utf-8'))
assert report['source_sha256']==hashlib.sha256(candidate.read_bytes()).hexdigest(),'QA is for a different file'
assert report['route_count']>=78 and report['clear_routes']==report['route_count'],'Route regression is not clear'
assert report.get('deck_fitting_room_intrusions')==[],'Deck fittings still intrude into rooms'
shots=json.loads((qa/'shots.json').read_text(encoding='utf-8'))
assert len(shots)>=16 and all((qa/(s['name']+'.png')).is_file() for s in shots),'Missing visual review outputs'
assert all(s.get('source_sha256')==report['source_sha256'] for s in shots),'Stale visual manifest'
assert all(hashlib.sha256((qa/(s['name']+'.png')).read_bytes()).hexdigest()==s['image_sha256'] for s in shots),'Image hash mismatch'
backup=OUT/'Backup-20260905-Refit'/'CalmyOceanFreighter.blend'
assert backup.is_file(),'Pre-refit backup missing'
scene=bpy.context.scene
scene['qa_report']=str(qa/'geometry-routes.json')
scene['design_status']='R3 Blender circulation refit; not production complete; Unreal acceptance outstanding'
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
(OUT/'build_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('R3_PROMOTED '+json.dumps(manifest),flush=True)
