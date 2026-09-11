"""Correct VSM per-pixel overflow found in round2, without hiding warnings/shadows."""
import unreal,hashlib,json,time,shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'Content/Maps/OceanFreighterR7.umap'
assert hashlib.sha256(path.read_bytes()).hexdigest()=='0af9c4dd558caea0951097d796d7c143ffd0bc93204f67f5542c288fc80ff967'
backup=root/'SourceAssets/Backups/Nightwatch-R1-LightingRound2'
assert not backup.exists();backup.mkdir(parents=True);shutil.copy2(path,backup/path.name)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
report=[]
for a in actors.get_all_level_actors():
    if not isinstance(a,unreal.PointLight):continue
    label=a.get_actor_label();c=a.point_light_component
    if 'Engine_Worklight' in label:radius=350.
    elif 'Nightwatch_Service_Fill' in label:radius=370.
    elif 'Nightwatch_DeckPool' in label:radius=700.
    else:continue
    c.set_editor_property('attenuation_radius',radius)
    report.append([label,radius])
assert levels.save_current_level()
(root/'Saved/Verification/Nightwatch-R1-20260908/overlap-fix.json').write_text(json.dumps({'changes':report,'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'shadows_disabled':False,'vsm_limit_changed':False},indent=2))
unreal.log('NIGHTWATCH_OVERLAP_FIX_PASS')
end=time.monotonic()+8
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
