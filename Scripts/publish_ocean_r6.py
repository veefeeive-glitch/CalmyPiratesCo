"""Publish the exact reviewed bytes; no re-save invalidating the physics hash."""
import json,hashlib,shutil
from pathlib import Path
from verify_ocean_r6_acceptance import check
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'SourceAssets/Generated/OceanFreighter'
candidate=out/'R6-Candidate/CalmyOceanFreighter.blend'
qa=ROOT/'Saved/Verification/OceanFreighter-R6-Reviewed'
report=ROOT/'Saved/OceanPhysics/R6-traversal3.json'
evidence=check(candidate,report,qa)
assert evidence['source_sha256']=='9ceafe214b04051c293734a81c31ab81e7daf109cfa73f82a8525a38ab759fdd','Visual approval is for different bytes'
assert evidence['source_sha256'] in (ROOT/'OCEAN_FREIGHTER_R6_REVIEW.md').read_text(encoding='utf-8')
old_digest='d4970d36b1fc0dfcb6ad0af95b8684b4a79849b471f740d8a1539402ca845bda'
assert hashlib.sha256((out/'Backup-20260905-R5/CalmyOceanFreighter.blend').read_bytes()).hexdigest()==old_digest
master=out/'CalmyOceanFreighter.blend'
assert hashlib.sha256(master.read_bytes()).hexdigest() in (old_digest,evidence['source_sha256']),'Master changed since R5; preserve user edits'
ship=json.loads((ROOT/'Saved/OceanPhysics/ship.json').read_text(encoding='utf-8'))
assert ship['sha256']==evidence['source_sha256'],'Shared lab input has changed'
owners={c['name']:c['owners'] for c in ship['chunks']}
physics=json.loads(report.read_text(encoding='utf-8-sig'))
contacts=[]
for w in physics['walks']:
    if w['pass']:continue
    obj=next((o['name'] for o in owners.get(w.get('hit_component'),[]) if o['first']<=w.get('face',-1)<o['first']+o['count']),None)
    contacts.append(dict(name=w['name'],reason=w['reason'],object=obj,position_cm=w['position'],target_cm=w['target']))
(qa/'remaining-contacts.json').write_text(json.dumps(contacts,indent=2),encoding='utf-8')
(qa/'route-input-snapshot.json').write_text(json.dumps(dict(source_sha256=ship['sha256'],units=ship['units'],routes=ship['routes']),indent=2),encoding='utf-8')
(qa/'acceptance.json').write_text(json.dumps(evidence,indent=2),encoding='utf-8')
shutil.copy2(candidate,master)
assert hashlib.sha256(master.read_bytes()).hexdigest()==evidence['source_sha256']
manifest=json.loads((candidate.parent/'build_manifest.json').read_text(encoding='utf-8'))
for k in ('mesh_objects','vertices','polygons','collections','qa_scope'):manifest.pop(k,None)
manifest.update(master_blend=str(master),revision='R6',status='R6 reviewed circulation increment; human visual rating pending',
    promoted_master_sha256=evidence['source_sha256'],qa_scope=evidence['scope'],
    physical_qa_report=str(report),qa_report=str(qa/'geometry-routes.json'),review_shots=str(qa/'shots.json'),
    preview=str(qa/'01_Overview.png'),previews_generated=True,acceptance=str(qa/'acceptance.json'))
(out/'build_manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('R6_PUBLISHED',evidence['source_sha256'],'required walks',evidence['required_passed'])
