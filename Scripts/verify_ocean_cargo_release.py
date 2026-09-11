"""R7 scoped clearance acceptance. Preserves exploratory FAILs; never publishes.

Requires exact route coverage, observed cargo pose, hash-bound fresh visuals and
geometry. Not a whole-ship art, interaction, moving-base or multiplayer gate.
"""
import argparse,hashlib,json,shutil,copy
from pathlib import Path
from recheck_ocean_traversal import indexed,classify,verified_views
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'Evidence/ShipTraversal/R6-Recheck-20260905-223837'
CONTROLS={'CONTROL_'+n+d for n in ('step15cm','wall') for d in ('_forward','_reverse')}
POSE='observed_mesh_component_transform_after_production_tick'
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(test,message):
    if not test:raise ValueError(message)
def validate_route_changes(before,after):
    # C03 deliberately narrows three rooms by 40cm to widen the stair lobby.
    # Review exactly their boundary-following points, not arbitrary retargeting.
    approved={'AUDIT_room_Boat LAUNDRY':(4,5),'AUDIT_room_Crew MEDICAL':(4,5),
        'AUDIT_room_Officers RADIO':(4,5),'AUDIT_usable_Crew MEDICAL':(2,3)}
    expected=copy.deepcopy(before);changes=[]
    for name,indices in approved.items():
        require(name in expected,'Missing C03 reviewed route')
        for i in indices:
            require(abs(expected[name][i][0]+4975)<.001,'Baseline wall-relative coordinate differs')
            expected[name][i][0]-=40
        changes.append(dict(name=name,before=before[name],after=expected[name],reason='C03 room boundary moved 40cm aft; same door and destination function'))
    require(expected==after,'Unreviewed route coordinate change')
    return changes
def validate_carry(report,route_names,sha):
    walks=indexed(report)
    require(report['source_sha256']==sha,'Stale cargo report')
    require(report.get('carry_audit') and report.get('cargo_overlap_pose')==POSE,'Not observed cargo pose')
    require({n for n in walks if n.startswith('CONTROL_')}==CONTROLS,'Wrong controls')
    selected={n for n in route_names if n.startswith('r6_') or n=='AUDIT_mess_to_bridge' or n.startswith('AUDIT_weather_to_hold_')}
    require(len(selected)==13,'Cargo route contract changed')
    expected={n+d+k for n in selected for d in ('_forward','_reverse') for k in ('_standard','_bulky')}
    require(set(walks)==expected|CONTROLS,'Cargo route coverage differs')
    require(all(w['pass'] for w in walks.values()),'Cargo failure remains')
    require(all(walks[n].get('actual_cargo_grabbed') for n in expected),'Cargo not actually grabbed')
    require(all(walks[n].get('cargo_kind')==('Bulky' if n.endswith('_bulky') else 'Standard') for n in expected),'Wrong cargo kind')
    require(all('helper_position' in walks[n] for n in expected if n.endswith('_bulky')),'Missing physical helper')
    return dict(passed=report['passed'],total=report['total'],cargo_walks=len(expected),controls=4)
def validate_walks(report,baseline,routes,geometry):
    old,new=indexed(baseline),indexed(report)
    expected={r['name']+d for r in routes for d in ('_forward','_reverse')}|CONTROLS
    require(set(old)==set(new)==expected,'Walking route coverage differs')
    regressions=[n for n in old if old[n]['pass'] and not new[n]['pass']]
    require(not regressions,'Previously passed walks regressed: '+str(regressions))
    required={r['name']+d for r in geometry['routes'] for d in ('_forward','_reverse')}
    prefixes=('AUDIT_exterior_tower_to_roof_','AUDIT_accommodation_to_weather_','AUDIT_usable_',
        'AUDIT_mess_to_bridge','AUDIT_weather_to_stern_engine_','AUDIT_weather_to_hold_','AUDIT_upper_exit_','AUDIT_bridge_watch')
    required|={n for n in expected if n.startswith(prefixes)}
    require(len(required)==260,'Required intent contract changed')
    require(all(new.get(n,{}).get('pass') for n in required),'Required intent failed')
    return dict(required_passed=len(required),required_total=len(required),broad_passed=report['passed'],broad_total=report['total'],regressions=regressions)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ('model','carry','walk','geometry','ship','review','output'):p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--views',type=Path,nargs='+',required=True);a=p.parse_args()
    sha=digest(a.model);carry,walk,g,ship=map(read,(a.carry,a.walk,a.geometry,a.ship))
    require(sha==ship['sha256']==g['source_sha256']==walk['source_sha256'],'Stale model/collision/geometry/walk evidence')
    require(sha in a.review.read_text(encoding='utf-8'),'Visual review not bound to current model')
    canonical=lambda rs:{r['name']:r['points'] for r in rs}
    route_changes=validate_route_changes(canonical(read(BASE/'route-input-snapshot.json')['routes']),canonical(ship['routes']))
    require(len(g['routes'])==g['route_count']==g['clear_routes']==98 and all(r['failed_samples']==0 for r in g['routes']),'Geometry failed/missing')
    require(not g['deck_fitting_room_intrusions'],'Fittings inside rooms')
    require(len(g['machinery_assembly_checks'])==4 and all(x['pass'] for x in g['machinery_assembly_checks']),'Missing/failed pump checks')
    summary=validate_walks(walk,read(BASE/'physical-report.json'),ship['routes'],g)
    summary['carry']=validate_carry(carry,canonical(ship['routes']),sha)
    shots=[s for directory in a.views for s in verified_views(directory,sha)]
    required_shots={'mess','internal_turn','internal_headroom','hold1_entry','hold1_turn','hold2_entry','boat_lobby','officer_lobby','laundry_boundary','medical_boundary','radio_boundary','hold1_sign','hold2_sign'}
    required_shots|={f'C01_{side}_{tag}' for side in (-1,1) for tag in ('aisle','connections')}
    require({s['name'] for s in shots}==required_shots and len(shots)==17,'Missing or duplicate scoped closeups')
    new=indexed(walk);chunks={c['name']:c for c in ship['chunks']};contacts=[]
    for w in walk['walks']:
        if w['pass']:continue
        category,alternative=classify(w['name']);face=w.get('face',-1)
        owner=next((o['name'] for o in chunks.get(w.get('hit_component'),{}).get('owners',[]) if o['first']<=face<o['first']+o['count']),None)
        require(owner and alternative and all(new.get(alternative+d,{}).get('pass') for d in ('_forward','_reverse')),'Unreviewed contact or failed alternative')
        contacts.append(dict(name=w['name'],object=owner,reason=w['reason'],position_cm=w['position'],category=category,alternative=alternative))
    summary.update(status='SCOPED_CLEARANCE_PASS_WITH_RETAINED_EXPLORATORY_FAILURES',source_sha256=sha,reviewed_route_changes=route_changes,
        collision_sha256=digest(a.ship),contacts=contacts,visual_count=len(shots),
        scope='Static ship lab, actual CharacterMovement and observed held cargo box; 13 cargo itineraries with local helper; NOT all interactions, human play, moving ship or ship multiplayer')
    a.output.mkdir(parents=True,exist_ok=False)
    for path,name in ((a.carry,'carry-report.json'),(a.walk,'physical-report.json'),(a.geometry,'geometry-routes.json'),(a.review,'visual-review.md')):shutil.copy2(path,a.output/name)
    (a.output/'route-input-snapshot.json').write_text(json.dumps({k:v for k,v in ship.items() if k!='chunks'},indent=2),encoding='utf-8')
    (a.output/'acceptance.json').write_text(json.dumps(summary,indent=2),encoding='utf-8')
    viewout=a.output/'views';viewout.mkdir()
    for directory in a.views:
        for s in read(directory/'shots.json'):shutil.copy2(directory/(s['name']+'.png'),viewout/(s['name']+'.png'))
    (viewout/'shots.json').write_text(json.dumps(shots,indent=2),encoding='utf-8')
    manifest={str(f.relative_to(a.output)):digest(f) for f in a.output.rglob('*') if f.is_file()}
    (a.output/'evidence-manifest.json').write_text(json.dumps(dict(model_sha256=sha,files=manifest),indent=2),encoding='utf-8')
    print(json.dumps({k:v for k,v in summary.items() if k!='contacts'},indent=2))
if __name__=='__main__':main()
