"""Rejected diagnostic: editor world did not have complete runtime collision ready.
Use CalmyDoorProbe=Audit in -game and apply_r7_runtime_swings.py instead.
Kept only to explain the failed first fit; do not run against a newer map.
"""
raise RuntimeError('Editor-only fit is not evidence. Use the runtime collision audit.')
import unreal,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
cls=unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyDoor')
doors=sorted([a for a in actors.get_all_level_actors() if a.get_class()==cls],key=lambda a:a.get_editor_property('door_id'))
report=[]
for a in doors:
    ok=a.fit_swing_to_world();p=a.get_actor_location();r=a.get_actor_rotation()
    report.append(dict(id=a.get_editor_property('door_id'),clear=ok,pivot=[p.x,p.y,p.z],yaw=r.yaw,sign=a.get_editor_property('swing_sign'),angle=a.get_editor_property('open_angle')))
(root/'Saved/Verification/Doors-R1/swings.json').write_text(json.dumps(report,indent=2))
unreal.log('DOORS_FIT clear='+str(sum(d['clear'] for d in report))+'/61')
assert levels.save_current_level()
unreal.SystemLibrary.quit_editor()
