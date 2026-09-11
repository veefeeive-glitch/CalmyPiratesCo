"""Read fresh R6 evidence, retain every failure, detect regression vs frozen R6.

This does NOT turn exploratory failures into passes and does not publish a model.
The original strict physics runner intentionally remains red for any failed walk.
"""
import argparse
import hashlib
import json
import math
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
QA = ROOT / 'Saved/Verification/OceanFreighter-R6-Reviewed'


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def indexed(report):
    walks = {w['name']: w for w in report['walks']}
    if len(walks) != len(report['walks']):
        raise ValueError('Duplicate walk names')
    if len(walks) != report['total'] or sum(w['pass'] for w in walks.values()) != report['passed']:
        raise ValueError('Inconsistent report totals')
    controls = [w for n, w in walks.items() if n.startswith('CONTROL_')]
    if len(controls) != 4 or not all(w['pass'] for w in controls):
        raise ValueError('Physical controls failed or missing')
    return walks


def compare(old, new, input_routes):
    before, after = indexed(old), indexed(new)
    expected = {r['name'] + d for r in input_routes for d in ('_forward', '_reverse')}
    if len(expected) != len(input_routes) * 2:
        raise ValueError('Duplicate input routes')
    observed = {n for n in after if not n.startswith('CONTROL_')}
    if observed != expected or set(before) != set(after):
        raise ValueError('Missing, renamed or unexpected routes; manual review required')
    if old['source_sha256'] != new['source_sha256']:
        raise ValueError('Baseline is for another model; do not reuse R6 classification')
    return {
        'regressions': sorted(n for n in before if before[n]['pass'] and not after[n]['pass']),
        'resolved_attempts': sorted(n for n in before if not before[n]['pass'] and after[n]['pass']),
    }


def classify(name):
    stem = name.rsplit('_', 1)[0]
    if stem.startswith('AUDIT_room_'):
        return 'furnishing-perimeter', stem.replace('AUDIT_room_', 'AUDIT_usable_', 1)
    if stem.startswith('AUDIT_weather_to_forepeak_'):
        return 'old-thruster-centreline', stem.replace('AUDIT_weather_to_forepeak_', 'r6_weather_to_forepeak_', 1)
    if stem.startswith('AUDIT_stern_weather_'):
        return 'old-stern-equipment-line', stem.replace('AUDIT_stern_weather_', 'r6_stern_service_', 1)
    if stem in ('AUDIT_exit_lane_(-1, 7.9)', 'AUDIT_exit_lane_(1, 7.9)'):
        return 'old-exit-lane-7.9m', 'AUDIT_accommodation_to_weather_' + ('-1' if '(-1,' in stem else '1')
    return 'unreviewed', None


def verified_views(directory, source_hash):
    shots=read(directory/'shots.json')
    if not shots:
        raise ValueError('Empty visual review')
    names=set()
    for shot in shots:
        name=shot['name']
        if Path(name).name != name or '/' in name or '\\' in name or name in names:
            raise ValueError('Invalid or duplicate image name')
        names.add(name)
        if shot['source_sha256'] != source_hash or digest(directory/(name+'.png')) != shot['image_sha256']:
            raise ValueError('Stale or changed review image')
    return shots


def evaluate(model, report_path, ship_path):
    from verify_ocean_r6_acceptance import check
    # Also verifies the original 35 image hashes and required-route contract.
    acceptance = check(model, report_path, QA)
    old = read(ROOT / 'Saved/OceanPhysics/R6-traversal3.json')
    new, ship = read(report_path), read(ship_path)
    if digest(model) != ship['sha256'] or ship['sha256'] != new['source_sha256']:
        raise ValueError('Stale collision input')
    frozen = read(QA / 'route-input-snapshot.json')
    canonical = lambda rs: {r['name']: r['points'] for r in rs}
    if canonical(ship['routes']) != canonical(frozen['routes']):
        raise ValueError('Route coordinates changed; cannot claim same-intent recheck')
    delta = compare(old, new, ship['routes'])
    walks = indexed(new)
    previous_contacts = {c['name']: c for c in read(QA / 'remaining-contacts.json')}
    chunks = {c['name']: c for c in ship['chunks']}
    contacts = []
    for w in new['walks']:
        if w['pass']:
            continue
        face = w.get('face', -1)
        owners = chunks.get(w.get('hit_component'), {}).get('owners', [])
        owner = next((o['name'] for o in owners if o['first'] <= face < o['first'] + o['count']), None)
        category, alternative = classify(w['name'])
        prev = previous_contacts.get(w['name'])
        same_contact = bool(prev and owner and prev['object'] == owner and
                            prev['reason'] == w['reason'] and
                            math.dist(prev['position_cm'], w['position']) <= 25)
        alt_ok = bool(alternative and all(walks.get(alternative + d, {}).get('pass', False)
                                         for d in ('_forward', '_reverse')))
        contacts.append(dict(name=w['name'],category=category,object=owner,position_cm=w['position'],
                             reason=w['reason'],same_contact_within_25cm=same_contact,
                             alternative=alternative,alternative_both_directions=alt_ok))
    intents = {
        'T01': ('AUDIT_exterior_tower_to_roof_',),
        'T02': ('AUDIT_accommodation_to_weather_',),
        'T03': ('r6_stern_service_', 'AUDIT_stern_inboard_'),
        'T04': ('r6_aft_gallery_', 'AUDIT_engine_gallery_'),
        'T05': ('r6_forepeak_service_', 'r6_weather_to_forepeak_', 'AUDIT_forepeak_bypass_'),
    }
    intent_results = {}
    for key, prefixes in intents.items():
        selected = [w for n, w in walks.items() if n.startswith(prefixes)]
        intent_results[key] = dict(total=len(selected),passed=sum(w['pass'] for w in selected),
                                   names=[w['name'] for w in selected])
    review = [c['name'] for c in contacts if not c['same_contact_within_25cm'] or not c['alternative_both_directions']]
    ok = not delta['regressions'] and not review and all(v['total'] and v['total']==v['passed'] for v in intent_results.values())
    return dict(status='R6_REQUIRED_PASS_WITH_RETAINED_CONTACTS' if ok else 'REVIEW_REQUIRED',
                scope='Isolated current CharacterMovement, actor Tick disabled; not full bodycam/cargo/multiplayer acceptance',
                source_sha256=ship['sha256'],report_sha256=digest(report_path),collision_sha256=digest(ship_path),
                required_passed=acceptance['required_passed'],required_total=acceptance['required_walks'],
                broad_passed=new['passed'],broad_total=new['total'],controls_passed=new['controls_passed'],
                intents=intent_results,contacts=contacts,changed_or_unreviewed_contacts=review,**delta)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--model', type=Path, default=ROOT/'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend')
    parser.add_argument('--ship', type=Path, default=ROOT/'Saved/OceanPhysics/ship.json')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--views', type=Path, help='Optional inspected render directory to preserve with evidence')
    args=parser.parse_args()
    result=evaluate(args.model.resolve(),args.report.resolve(),args.ship.resolve())
    views=verified_views(args.views,result['source_sha256']) if args.views else []
    args.output.mkdir(parents=True,exist_ok=False)
    (args.output/'recheck.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
    # Freeze inputs rather than referring only to mutable Saved/OceanPhysics/ship.json.
    frozen=read(args.ship)
    (args.output/'route-input-snapshot.json').write_text(json.dumps({k:v for k,v in frozen.items() if k!='chunks'},indent=2),encoding='utf-8')
    lines=['# R6 fresh traversal recheck','',result['status'],'',
           '| Finding | Passed / directed attempts |','|---|---:|']
    for key,value in result['intents'].items():
        lines.append(f"| {key} | {value['passed']}/{value['total']} |")
    lines += ['','All failed attempts remain FAIL. Coordinates below are Blender feet in metres (capsule half height 88cm).','',
              '| Failed attempt | Contact mesh | Feet XYZ m | Tested alternative (both directions) |','|---|---|---|---|']
    for c in result['contacts']:
        x,y,z=c['position_cm'];p=f'{x/100:.2f}, {-y/100:.2f}, {(z-88)/100:.2f}'
        lines.append(f"| {c['name']} | {c['object']} | {p} | {c['alternative']} ({c['alternative_both_directions']}) |")
    (args.output/'contact-register.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    shutil.copy2(args.report,args.output/'physical-report.json')
    if views:
        (args.output/'views').mkdir()
        shutil.copy2(args.views/'shots.json',args.output/'views/shots.json')
        for shot in views:
            shutil.copy2(args.views/(shot['name']+'.png'),args.output/'views'/(shot['name']+'.png'))
    sources=[Path(__file__),ROOT/'Scripts/test_traversal_recheck.py',
             ROOT/'Scripts/render_ocean_traversal_audit.py',ROOT/'Scripts/traversal_camera_profile.py',
             ROOT/'Scripts/export_ocean_physics.py',ROOT/'Scripts/ocean_traversal_audit.py',
             ROOT/'Scripts/verify_ocean_r6_acceptance.py',
             ROOT/'Source/CalmyPiratesCo/Private/Testing/CalmyOceanPhysicsProbe.cpp',
             ROOT/'Source/CalmyPiratesCo/Private/Gameplay/CalmyCharacter.cpp',
             ROOT/'Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll']
    manifest=dict(model_sha256=result['source_sha256'],
                  sources={str(p.relative_to(ROOT)):digest(p) for p in sources},
                  files={str(p.relative_to(args.output)):digest(p) for p in args.output.rglob('*') if p.is_file()},
                  inspected_views_count=len(views),
                  scope='Evidence archive only; visual acceptance is recorded separately by the reviewer')
    (args.output/'evidence-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k not in ('contacts','intents')},indent=2))
    return 0 if result['status']=='R6_REQUIRED_PASS_WITH_RETAINED_CONTACTS' else 1


if __name__=='__main__':
    raise SystemExit(main())
