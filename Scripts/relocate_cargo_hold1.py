"""Move R7 deck-task cargo into HOLD 1; keep delivery on weather deck. Backup map first."""
import unreal, shutil, time
from pathlib import Path

root = Path(__file__).resolve().parents[1]
mp = root / 'Content/Maps/OceanFreighterR7.umap'
backup = root / 'SourceAssets/Backups/Before-CargoHold1-20260913'
backup.mkdir(parents=True, exist_ok=True)
if not (backup / mp.name).exists():
    shutil.copy2(mp, backup / mp.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')

cargo_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyCargo')
assert cargo_cls
cargos = [a for a in actors.get_all_level_actors() if a.get_class() == cargo_cls]
assert len(cargos) == 4, 'expected 4 cargo, got %d' % len(cargos)

# HOLD 1 floor near (-800,0,~320), offset from fly colony at (-800,0,390).
# Place along -Y wall so circulation and flies stay clear.
slots = [
    unreal.Vector(-1150, -420, 320),
    unreal.Vector(-950, -420, 320),
    unreal.Vector(-750, -420, 320),
    unreal.Vector(-550, -420, 320),
]
cargos_sorted = sorted(cargos, key=lambda a: int(a.get_editor_property('cargo_id') or 0))
for i, c in enumerate(cargos_sorted):
    c.set_actor_location(slots[i], False, False)
    c.set_actor_label('R7_Hold1_Cargo_' + str(i + 1))
    unreal.log('CARGO_HOLD1 id=%s kind=%s pos=%s' % (
        c.get_editor_property('cargo_id'), c.get_editor_property('kind'), slots[i]))

assert levels.save_current_level()
unreal.log('CARGO_HOLD1_RELOCATE cargos=4 delivery=deck_green_zone')

_after = None
def finish(_dt):
    global _after
    if _after is None:
        _after = time.monotonic() + 8
    if time.monotonic() >= _after:
        unreal.unregister_slate_post_tick_callback(_handle)
        unreal.SystemLibrary.quit_editor()
_handle = unreal.register_slate_post_tick_callback(finish)
