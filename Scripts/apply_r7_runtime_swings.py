import unreal,re,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
log=(root/'Saved/Verification/Doors-R1/RuntimeFit.log').read_text(errors='replace')
rows=re.findall(r'DOOR_FIT id=(\d+) clear=(\d) x=([-\d.]+) y=([-\d.]+) z=([-\d.]+) yaw=([-\d.]+) sign=([-\d.]+) angle=([-\d.]+)',log)
assert len(rows)==61
assert all(r[1]=='1' for r in rows),'Do not apply an incomplete physical audit'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
cls=unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyDoor')
doors={a.get_editor_property('door_id'):a for a in actors.get_all_level_actors() if a.get_class()==cls}
for i,ok,x,y,z,yaw,sign,angle in rows:
    a=doors[int(i)];a.set_actor_location_and_rotation(unreal.Vector(float(x),float(y),float(z)),unreal.Rotator(yaw=float(yaw)),False,False)
    a.set_editor_property('swing_sign',float(sign));a.set_editor_property('open_angle',float(angle));a.rebuild_geometry()
assert levels.save_current_level()
(root/'Saved/Verification/Doors-R1/runtime-swings.json').write_text(json.dumps(rows,indent=2))
unreal.log('DOORS_RUNTIME_APPLY '+str(sum(int(r[1]) for r in rows))+'/61')
unreal.SystemLibrary.quit_editor()
