"""Apply lightweight saved-state settings without rebuilding ship geometry."""
from pathlib import Path
import math
import bpy

ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / "SourceAssets" / "Generated" / "OceanFreighter" / "CalmyOceanFreighter.blend"

for obj in bpy.context.scene.objects:
    if obj.type == "LIGHT" and obj.name.startswith("InteriorLight_"):
        obj.data.use_shadow = False

collision = bpy.data.collections.get("90_COLLISION")
if collision is None:
    collision = bpy.data.collections.new("90_COLLISION")
    bpy.context.scene.collection.children.link(collision)

lod_collection = bpy.data.collections.get("95_EXPORT_LOD")
if lod_collection is None:
    lod_collection = bpy.data.collections.new("95_EXPORT_LOD")
    bpy.context.scene.collection.children.link(lod_collection)


def proxy_cube(name, location, dimensions, collection, rotation=(0, 0, 0), role="collision"):
    if name in bpy.data.objects:
        return bpy.data.objects[name]
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for source in list(obj.users_collection):
        source.objects.unlink(obj)
    collection.objects.link(obj)
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["export_role"] = role
    return obj


for level_name, x, z, length, width in (
    ("BoatDeck", -43.0, 10.58, 24.0, 17.0),
    ("Accommodation", -43.0, 13.33, 24.0, 16.0),
    ("Officers", -42.0, 16.08, 21.0, 14.4),
    ("Bridge", -40.5, 18.83, 18.0, 13.2),
):
    proxy_cube(f"UCX_{level_name}_Floor", (x, 0, z - 0.05), (length, width, 0.10), collision)

for name, start, direction, rise, run, width in (
    ("AftLower", (-31.5, -6.8, 4.58), 1, 2.58, 4.4, 1.25),
    ("AftWeather", (-31.5, -6.8, 7.33), 1, 3.08, 4.8, 1.25),
    ("ForwardLower", (47.0, 5.8, 4.58), -1, 2.58, 4.4, 1.25),
    ("ForwardWeather", (47.0, 5.8, 7.33), -1, 3.08, 4.8, 1.25),
    ("EngineMain", (-55.0, -6.2, 2.08), 1, 2.34, 3.8, 1.15),
    ("EngineEscape", (-35.0, 6.2, 2.08), -1, 2.34, 3.8, 1.15),
):
    angle = math.atan2(rise, run)
    center = (start[0] + direction * run * 0.5, start[1], start[2] + rise * 0.5)
    proxy_cube(f"UCX_StairRamp_{name}", center, (math.hypot(run, rise), width, 0.12),
               collision, (0, -direction * angle, 0))

proxy_cube("UCX_HullBoundary_Port", (0, -9.85, 5.4), (108.0, 0.24, 9.8), collision)
proxy_cube("UCX_HullBoundary_Starboard", (0, 9.85, 5.4), (108.0, 0.24, 9.8), collision)

for name, location, dimensions in (
    ("Hull_Complete_LOD1", (0, 0, 5.2), (118.0, 19.0, 10.2)),
    ("Accommodation_HLOD1", (-43.0, 0, 16.2), (24.0, 16.0, 11.2)),
    ("CargoZone_HLOD1", (11.0, 0, 8.0), (63.0, 18.0, 8.0)),
    ("BowEquipment_HLOD1", (50.0, 0, 11.7), (18.0, 16.0, 3.0)),
):
    proxy_cube(name, location, dimensions, lod_collection, role="HLOD proxy baseline")

collision.hide_render = True
collision.hide_viewport = True
lod_collection.hide_render = True
lod_collection.hide_viewport = True

walk_camera = bpy.data.objects.get("CAM_Walkthrough_FirstPerson")
if walk_camera:
    bpy.context.scene.camera = walk_camera

bpy.ops.wm.save_as_mainfile(filepath=str(MASTER))
print("CALMY_OCEAN_FREIGHTER_RUNTIME_FINALIZE_PASS collision_hidden=1 interior_light_shadows=0 lod_hidden=1")
