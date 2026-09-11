"""Restore real solar angular diameter; separate source softness from disk size."""
import unreal,hashlib,shutil,json,time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
expected={'OceanFreighterR7':'51cac5aeff57d894b63e44569d561ea950705f74ea21ba60cdbdb23bd2f86714','WaterLab':'36fc844f1e21014c653e07df65a660796cf36983f5caed3bf538768a1a07d31e'}
backup=root/'SourceAssets/Backups/Nightwatch-R1-BeforeSunDiscFix'
assert not backup.exists()
for name,digest in expected.items():assert hashlib.sha256((root/'Content/Maps'/(name+'.umap')).read_bytes()).hexdigest()==digest
backup.mkdir(parents=True)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={}
for name in expected:
    path=root/'Content/Maps'/(name+'.umap');shutil.copy2(path,backup/path.name)
    assert levels.load_level('/Game/Maps/'+name)
    for a in actors.get_all_level_actors():
        if isinstance(a,unreal.DirectionalLight):
            a.light_component.set_editor_property('light_source_angle',.5357)
            a.light_component.set_editor_property('light_source_soft_angle',5.)
    assert levels.save_current_level()
    report[name]=hashlib.sha256(path.read_bytes()).hexdigest()
(root/'Saved/Verification/Nightwatch-R1-20260908/final-map-hashes.json').write_text(json.dumps(report,indent=2))
unreal.log('NIGHTWATCH_SUN_DISC_FIX_PASS maps=2')
end=time.monotonic()+8
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
