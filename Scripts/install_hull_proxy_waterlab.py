"""Place a G2 floating hull proxy on WaterLab for based-movement / buoyancy soak."""
import unreal, hashlib, shutil, time
from pathlib import Path

root = Path(__file__).resolve().parents[1]
mp = root / 'Content/Maps/WaterLab.umap'
backup = root / 'SourceAssets/Backups/Before-HullProxy-WaterLab-20260912'
backup.mkdir(parents=True, exist_ok=True)
if not (backup / mp.name).exists():
    shutil.copy2(mp, backup / mp.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/WaterLab')

proxy_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyHullProxy')
ocean_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyOcean')
assert proxy_cls and ocean_cls

all_actors = list(actors.get_all_level_actors())
oceans = [a for a in all_actors if a.get_class() == ocean_cls]
assert len(oceans) == 1, 'expected one CalmyOcean on WaterLab'
ocean = oceans[0]
origin = ocean.get_actor_location()

existing = [a for a in actors.get_all_level_actors() if a.get_class() == proxy_cls]
for e in existing:
    e.destroy_actor()

# Float beside the lab ramp, clear of the dry tutorial platform.
spawn = unreal.Vector(origin.x + 900.0, origin.y + 700.0, origin.z + 120.0)
proxy = actors.spawn_actor_from_class(proxy_cls, spawn, unreal.Rotator(0, 0, 0))
proxy.set_actor_label('HullProxy_G2')
proxy.set_folder_path('G2/HullProxy')
proxy.set_editor_property('hull_mass_kg', 1200.0)
proxy.set_editor_property('buoyancy_lift_ratio', 1.35)
proxy.set_editor_property('home_spring', 0.02)

assert levels.save_current_level()
sha = hashlib.sha256(mp.read_bytes()).hexdigest()
unreal.log('HULL_PROXY_INSTALL label=%s loc=%s map_sha256=%s' % (
    proxy.get_actor_label(), spawn, sha))

end = time.monotonic() + 4
handle = None
def finish(dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
