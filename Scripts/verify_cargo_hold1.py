"""Verify HOLD1 cargo positions; quit."""
import unreal, time
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
cargo_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyCargo')
cargos = [a for a in actors.get_all_level_actors() if a.get_class() == cargo_cls]
assert len(cargos) == 4
ok = 0
for c in sorted(cargos, key=lambda a: int(a.get_editor_property('cargo_id') or 0)):
    p = c.get_actor_location()
    label = c.get_actor_label()
    in_hold = abs(p.z - 320) < 80 and p.x > -1300 and p.x < -400 and p.y < -200
    unreal.log('CARGO_HOLD1_VERIFY id=%s label=%s pos=(%.0f,%.0f,%.0f) hold=%s' % (
        c.get_editor_property('cargo_id'), label, p.x, p.y, p.z, in_hold))
    if in_hold: ok += 1
assert ok == 4, 'expected 4 cargo in HOLD1, got %d' % ok
unreal.log('CARGO_HOLD1_VERIFY_PASS n=4')
_after=None
def finish(_dt):
    global _after
    if _after is None: _after=time.monotonic()+5
    if time.monotonic()>=_after:
        unreal.unregister_slate_post_tick_callback(_handle); unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(finish)
