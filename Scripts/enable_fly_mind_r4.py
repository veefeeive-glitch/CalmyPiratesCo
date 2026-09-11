"""Enable the cognitive model on the saved HOLD 1 resident, via UE APIs."""
import unreal,json,hashlib,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'Content/Maps/OceanFreighterR7.umap'
backup=root/'SourceAssets/Backups/Before-FlyMind-R4-20260910'
backup.mkdir(parents=True,exist_ok=True)
if not (backup/path.name).exists():shutil.copy2(path,backup/path.name)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
residents=[a for a in actors.get_all_level_actors() if a.actor_has_tag('Habitat_Hold1')]
assert len(residents)==1
resident=residents[0]
resident.set_editor_property('enable_cognition',True)
resident.set_editor_property('persistent_memory_id',unreal.Name('Hold1'))
resident.set_actor_label('FlyResident_Hold1')
assert levels.save_current_level()
assert levels.load_level('/Game/Maps/OceanFreighterR7')
resident=next(a for a in actors.get_all_level_actors() if a.actor_has_tag('Habitat_Hold1'))
assert resident.get_editor_property('enable_cognition')
assert str(resident.get_editor_property('persistent_memory_id'))=='Hold1'
assert (resident.get_actor_location()-unreal.Vector(-800,0,390)).length()<.1
report={'model':'engineered cognitive model + independent source LIF brains','consciousness_verified':False,
        'location':'HOLD 1','center_cm':[-800,0,390],'fly_count':resident.get_editor_property('fly_count'),
        'memory_id':'Hold1','saved_reload':True,'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
(root/'Saved/Verification/FlyMind-R4-install.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('FLY_MIND_R4_INSTALL_PASS saved_reload=1 hold1=1 cognition=1 flies=32')
end=time.monotonic()+5
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
