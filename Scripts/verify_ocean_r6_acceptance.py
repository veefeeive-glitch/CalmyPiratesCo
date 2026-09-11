"""Hash-bound R6 publication gate; historical exploratory failures stay visible."""
import json,hashlib,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def check(model,report,qa):
    digest=hashlib.sha256(model.read_bytes()).hexdigest()
    physics=json.loads(report.read_text(encoding='utf-8-sig'))
    geometry=json.loads((qa/'geometry-routes.json').read_text(encoding='utf-8'))
    assert physics['source_sha256']==geometry['source_sha256']==digest,'Stale evidence'
    assert physics['controls_total']==physics['controls_passed']==4,'Physical controls failed'
    assert geometry['clear_routes']==geometry['route_count'],'Geometric route regression'
    assert geometry['route_count']>=98,'Missing R6 routes'
    assert not geometry['deck_fitting_room_intrusions'],'Room intrusions'
    assert all(x['pass'] for x in geometry['machinery_assembly_checks']),'Machinery assembly'
    assert len(geometry['machinery_assembly_checks'])==4,'Missing assembly tests'
    required={r['name']+d for r in geometry['routes'] for d in ('_forward','_reverse')}
    # Exit tower, forward working deck, long internal journeys and every usable
    # room are required; attempts behind fixed furniture are retained as findings.
    for w in physics['walks']:
        if w['name'].startswith(('AUDIT_exterior_tower_to_roof_','AUDIT_accommodation_to_weather_',
            'AUDIT_usable_','AUDIT_mess_to_bridge','AUDIT_weather_to_stern_engine_',
            'AUDIT_weather_to_hold_','AUDIT_upper_exit_','AUDIT_bridge_watch')):required.add(w['name'])
    passed={w['name'] for w in physics['walks'] if w['pass']}
    assert not required-passed, 'Failed required walks: '+str(sorted(required-passed))
    assert len(required)>=240,'Insufficient full audit coverage'
    shots=json.loads((qa/'shots.json').read_text(encoding='utf-8'))
    assert {str(i).zfill(2) for i in range(1,36)}<={s['name'].split('_')[0] for s in shots},'Missing views'
    for s in shots:
        assert s['source_sha256']==digest
        assert hashlib.sha256((qa/(s['name']+'.png')).read_bytes()).hexdigest()==s['image_sha256']
    return dict(source_sha256=digest,required_walks=len(required),required_passed=len(required),
                exploratory_failed=[w['name'] for w in physics['walks'] if not w['pass']],
                physical_report=str(report),geometry_report=str(qa/'geometry-routes.json'),
                scope='Isolated Unreal CharacterMovement/Chaos; not multiplayer or art completion')
if __name__=='__main__':
    print(json.dumps(check(*(Path(x).resolve() for x in sys.argv[1:4])),indent=2))
