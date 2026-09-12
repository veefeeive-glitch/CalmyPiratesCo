"""Close all interactive R7 doors at start and install CalmyShipMotion with attachments."""
import unreal, hashlib, shutil, time
from pathlib import Path

root = Path(__file__).resolve().parents[1]
mp = root / 'Content/Maps/OceanFreighterR7.umap'
backup = root / 'SourceAssets/Backups/Before-ShipDrift-Doors-20260912'
backup.mkdir(parents=True, exist_ok=True)
if not (backup / mp.name).exists():
    shutil.copy2(mp, backup / mp.name)

levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')

door_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyDoor')
ship_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyShipMotion')
ocean_cls = unreal.load_class(None, '/Script/CalmyPiratesCo.CalmyOcean')
assert door_cls and ship_cls and ocean_cls

all_actors = list(actors.get_all_level_actors())
doors = [a for a in all_actors if a.get_class() == door_cls]
assert len(doors) == 61, 'expected 61 doors, got %d' % len(doors)
for d in doors:
    d.set_editor_property('start_open', False)
    # Refresh construction so leaf sits closed in the editor viewport.
    if hasattr(d, 'rebuild_geometry'):
        d.rebuild_geometry()

oceans = [a for a in all_actors if a.get_class() == ocean_cls]
assert len(oceans) == 1, 'expected one CalmyOcean'
ocean = oceans[0]
origin = ocean.get_editor_property('dry_hull_origin')

existing = [a for a in actors.get_all_level_actors() if a.get_class() == ship_cls]
for e in existing:
    e.destroy_actor()

ship = actors.spawn_actor_from_class(ship_cls, origin, unreal.Rotator(0, 0, 0))
ship.set_actor_label('ShipMotion_R7')
ship.set_folder_path('R7/ShipMotion')
# Keep authored heading; place at dry-hull origin XY with Z from current actor default.
ship.set_actor_location(unreal.Vector(origin.x, origin.y, origin.z), False, False)

skip_names = {
    'ShipMotion_R7',
}
skip_classes = {
    str(ocean_cls),
    str(ship_cls),
    '/Script/Engine.WorldSettings',
    '/Script/Engine.PlayerStart',
    '/Script/Engine.SkyLight',
    '/Script/Engine.DirectionalLight',
    '/Script/Engine.ExponentialHeightFog',
    '/Script/Engine.SkyAtmosphere',
    '/Script/Engine.VolumetricCloud',
    '/Script/Engine.PostProcessVolume',
    '/Script/Engine.DefaultPhysicsVolume',
    '/Script/Engine.Brush',
    '/Script/Engine.LevelBounds',
}

def should_attach(a):
    if not a or a == ship or a == ocean:
        return False
    label = a.get_actor_label() or ''
    if label in skip_names:
        return False
    cpath = str(a.get_class().get_path_name())
    for s in skip_classes:
        if s in cpath:
            return False
    # Geometry + doors only. Flies/cargo/characters ride via CarryWorldRiders / based movement.
    if a.get_class() == door_cls:
        return True
    if 'StaticMeshActor' in cpath or label.startswith('SM_R7'):
        return True
    folder = ''
    try:
        folder = str(a.get_folder_path() or '')
    except Exception:
        pass
    if folder.startswith('R7') and 'StaticMesh' in cpath:
        return True
    return False

attached = 0
for a in list(actors.get_all_level_actors()):
    if not should_attach(a):
        continue
    # Movable mobility required for based-movement / parent motion.
    for comp in a.get_components_by_class(unreal.SceneComponent):
        try:
            if hasattr(comp, 'set_mobility'):
                comp.set_mobility(unreal.ComponentMobility.MOVABLE)
        except Exception:
            pass
    ok = a.attach_to_actor(ship, unreal.Name(), unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)
    if ok:
        attached += 1

assert levels.save_current_level()
sha = hashlib.sha256(mp.read_bytes()).hexdigest()
unreal.log('SHIP_DRIFT_DOORS_INSTALL doors_closed=%d attached=%d ship=%s dry_hull=%s map_sha256=%s' % (
    len(doors), attached, ship.get_actor_label(), origin, sha))

end = time.monotonic() + 4
handle = None
def finish(dt):
    if time.monotonic() >= end:
        unreal.unregister_slate_post_tick_callback(handle)
        unreal.SystemLibrary.quit_editor()
handle = unreal.register_slate_post_tick_callback(finish)
