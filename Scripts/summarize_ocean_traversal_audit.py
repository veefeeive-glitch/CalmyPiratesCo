"""Freeze the inspected route evidence and derive an auditable route index."""
import json,hashlib,math
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
label='R5-traversal-audit2'
out=ROOT/'Saved'/'Verification'/label
ship=json.loads((ROOT/'Saved/OceanPhysics/ship.json').read_text(encoding='utf-8'))
report=json.loads((ROOT/'Saved/OceanPhysics'/(label+'.json')).read_text(encoding='utf-8-sig'))
master=ROOT/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
assert hashlib.sha256(master.read_bytes()).hexdigest()==ship['sha256']==report['source_sha256']
walks={w['name']:w for w in report['walks']}
required={r['name']+suffix for r in ship['routes'] for suffix in ('_forward','_reverse')}
assert required=={n for n in walks if not n.startswith('CONTROL_')}
assert len(walks)==report['total'] and sum(w['pass'] for w in walks.values())==report['passed']
assert report['controls_total']==report['controls_passed']==4
assert all(w['pass'] for n,w in walks.items() if not n.startswith('AUDIT_'))
summary=dict(source_sha256=ship['sha256'],routes=len(ship['routes']),base_routes=88,
             exploratory_routes=len(ship['routes'])-88,total=report['total'],passed=report['passed'],
             failed=report['total']-report['passed'],controls_passed=4,
             status='AUDIT COMPLETE; model NOT accepted, failed intent routes require triage',
             script_hashes={n:hashlib.sha256((ROOT/'Scripts'/n).read_bytes()).hexdigest() for n in ('ocean_traversal_audit.py','export_ocean_physics.py','render_ocean_traversal_audit.py')})
(out/'evidence-summary.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
(out/'route-input-snapshot.json').write_text(json.dumps({k:v for k,v in ship.items() if k!='chunks'},indent=2),encoding='utf-8')
lines=['# Physical route index — R5 traversal audit','',
       'Unreal centimetres in input snapshot; table length is intended polyline length, not measured travel.',
       'FAIL means this specific itinerary did not finish, not that the entire room is unreachable.','',
       '| Route | Length m | Forward | Reverse |','|---|---:|---|---|']
for r in ship['routes']:
    length=sum(math.dist(a,b) for a,b in zip(r['points'],r['points'][1:]))/100
    def status(suffix):
        w=walks[r['name']+suffix]
        return ('PASS' if w['pass'] else 'FAIL '+w['reason'])+f" ({w['seconds']:.1f}s, segment {w['segment']})"
    lines.append(f"| {r['name']} | {length:.1f} | {status('_forward')} | {status('_reverse')} |")
(out/'route-index.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
for directory,count in (('R5-traversal-audit1',78),('R5-traversal-audit2',32)):
    path=ROOT/'Saved/Verification'/directory
    shots=json.loads((path/'shots.json').read_text(encoding='utf-8'))
    assert len(shots)==count
    assert all(s['source_sha256']==ship['sha256'] and hashlib.sha256((path/(s['name']+'.png')).read_bytes()).hexdigest()==s['image_sha256'] for s in shots)
print(json.dumps(summary))
