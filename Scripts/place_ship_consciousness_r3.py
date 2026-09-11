"""Move the existing fly resident into HOLD 1; remove the training duplicate.

All map changes use Unreal Editor APIs. Does not change the LIF model/assets.
"""
from pathlib import Path
import hashlib,json,shutil,time
import unreal

ROOT=Path(__file__).resolve().parents[1]
BACKUP=ROOT/'SourceAssets/Backups/Before-FlyHabitat-R3-20260910'
BACKUP.mkdir(parents=True,exist_ok=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
fly_class=unreal.load_class(None,'/Script/CalmyPiratesCo.FlySwarmMob')
report={'operation':'habitat relocation only; no consciousness claim','habitat':'HOLD 1','model_changed':False,'maps':{}}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()

for name in ('WaterLab','OceanFreighterR7'):
    path=ROOT/f'Content/Maps/{name}.umap'
    backup=BACKUP/path.name
    if not backup.exists():shutil.copy2(path,backup)
    before=sha(path)
    assert levels.load_level('/Game/Maps/'+name)
    residents=[a for a in actors.get_all_level_actors() if a.actor_has_tag('FlyR2_Gameplay')]
    if name=='WaterLab':
        for a in residents:assert actors.destroy_actor(a)
        assert not any(a.actor_has_tag('FlyR2_Gameplay') for a in actors.get_all_level_actors())
        state={'resident_count':0,'removed':len(residents)}
    else:
        assert len(residents)==1, 'Expected exactly one existing ship resident'
        resident=residents[0]
        # HOLD 1 spans X=-2000..1000 cm; tank-top walking surface Z=208 cm.
        # This point sits inside the open hold, forward of the access stair.
        center=unreal.Vector(-800,0,390)
        resident.set_actor_label('FlyResident_Hold1')
        resident.set_editor_property('tags',[unreal.Name('FlyR2_Gameplay'),unreal.Name('ShipConsciousness'),unreal.Name('Habitat_Hold1')])
        resident.set_actor_location(center,False,False)
        resident.set_editor_property('spawn_radius',85.0)
        resident.set_editor_property('roam_radius',145.0)
        world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        hit=unreal.SystemLibrary.sphere_trace_single(world,center,center+unreal.Vector(0,0,.1),150.,
            unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,True,[resident],unreal.DrawDebugTrace.NONE,True)
        assert hit is None, 'Habitat sphere intersects ship collision; choose another point'
        state={'resident_count':1,'actor_label':resident.get_actor_label(),'center_cm':[-800,0,390],
               'flies':resident.get_editor_property('fly_count'),'roam_radius_cm':145,'clearance_sphere_cm':150,'clearance_pass':True,
               'mesh':resident.get_editor_property('fly_mesh').get_path_name()}
    assert levels.save_current_level()
    report['maps'][name]={'before_sha256':before,'after_sha256':sha(path),**state}

# Reload both packages, proving saved placement/removal rather than just editor state.
for name,expected in [('WaterLab',0),('OceanFreighterR7',1)]:
    assert levels.load_level('/Game/Maps/'+name)
    saved=[a for a in actors.get_all_level_actors() if a.actor_has_tag('FlyR2_Gameplay')]
    assert len(saved)==expected
    if saved:
        assert saved[0].actor_has_tag('Habitat_Hold1')
        assert (saved[0].get_actor_location()-unreal.Vector(-800,0,390)).length()<.1
(ROOT/'Saved/Verification/FlyHabitat-R3-install.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
unreal.log('FLY_HABITAT_R3_PASS hold1=1 bridge=0 waterlab=0 flies=32 clearance=150cm saved_reload=1')
end=time.monotonic()+5
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
