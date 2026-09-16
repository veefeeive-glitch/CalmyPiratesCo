"""Limit the six port hold-landing bulkhead doors of OceanFreighterR7 to a 90 degree swing.

Doors 52/53 (X 10.1 m), 55/56 (X 14.1 m) and 58/59 (X 42.1 m) hinge exactly on the inboard edge of their landings.
Opening 180 degrees swings the heavy leaf out over the hold, so no guardrail can stand beside the door and players
could step off the landing. At 90 degrees the leaf parks along the landing edge (walkway and doorway clear), which
lets the guardrails (S2) close the edge. Backs up the map, verifies the 0..90 swing is clear, rebuilds, saves, quits.
"""
import unreal, shutil, time, hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MAP = '/Game/Maps/OceanFreighterR7'
MAP_FILE = ROOT / 'Content/Maps/OceanFreighterR7.umap'
DOOR_IDS = {52, 53, 55, 56, 58, 59}
AFT_SWING = {58, 59}
backup = ROOT / ('SourceAssets/Backups/Before-LandingDoorSwing90-' + time.strftime('%Y%m%d'))
backup.mkdir(parents=True, exist_ok=True)
if not (backup / MAP_FILE.name).exists():
    shutil.copy2(MAP_FILE, backup / MAP_FILE.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level(MAP)
door_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyDoor')
changed = []
for door in [a for a in actors.get_all_level_actors() if a.get_class() == door_cls]:
    did = door.get_editor_property('door_id')
    if did not in DOOR_IDS:
        continue
    before = door.get_editor_property('open_angle')
    door.set_editor_property('open_angle', 90.0)
    # 58/59 stand at the forward end of their landing: swinging toward +X parked the leaf across the passage to the
    # hold crossing, so they swing aft and park along their own landing like 52/53 and 55/56 do.
    if did in AFT_SWING:
        door.set_editor_property('swing_sign', 1.0)
    door.rebuild_geometry()
    changed.append('%d:%s->90 sign=%.0f' % (did, before, door.get_editor_property('swing_sign')))
assert len(changed) == len(DOOR_IDS), 'expected six landing doors, got %s' % changed
assert levels.save_current_level()
unreal.log('LANDING_DOOR_SWING_PASS doors=%s map_sha256=%s backup=%s' % (','.join(changed), hashlib.sha256(MAP_FILE.read_bytes()).hexdigest(), backup))
end = time.monotonic() + 5
handle = None
def finish(_dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
