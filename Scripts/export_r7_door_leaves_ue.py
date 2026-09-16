"""Read-only: dump every CalmyDoor of OceanFreighterR7 (saved map pose, swing, leaf size) for the S2 guardrail builder.

UnrealEditor.exe CalmyPiratesCo.uproject -ExecutePythonScript=Scripts/export_r7_door_leaves_ue.py -unattended -nop4 -NoSound
Writes Saved/Verification/R7-Doors/r7-doors.json ([{id,x,y,z,yaw,sign,open_angle,width,height}], UE cm ship-local).
"""
import unreal, time, json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'Saved/Verification/R7-Doors/r7-doors.json'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
door_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyDoor')
rows = []
for d in [a for a in (actors.get_all_level_actors() or []) if a.get_class() == door_cls]:
    loc = d.get_actor_location()
    rows.append({'id': d.get_editor_property('door_id'), 'x': round(loc.x, 2), 'y': round(loc.y, 2), 'z': round(loc.z, 2),
                 'yaw': round(d.get_actor_rotation().yaw, 2), 'sign': d.get_editor_property('swing_sign'),
                 'open_angle': d.get_editor_property('open_angle'), 'width': d.get_editor_property('width_cm'),
                 'height': d.get_editor_property('height_cm')})
rows.sort(key=lambda r: r['id'])
assert len(rows) == 61, 'expected 61 doors, got %d' % len(rows)
OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_text(json.dumps(rows, indent=1), encoding='utf-8')
unreal.log('R7_DOOR_EXPORT_PASS count=%d out=%s' % (len(rows), OUT))
end = time.monotonic() + 4
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
