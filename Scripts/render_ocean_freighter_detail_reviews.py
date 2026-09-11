"""Render focused QA frames from the generated Ocean Freighter master."""
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Saved" / "Verification"
OUT.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
camera = bpy.data.objects["CAM_ReviewCloseup"]
scene.camera = camera
for light in (obj for obj in scene.objects if obj.type == "LIGHT" and obj.name.startswith("InteriorLight_")):
    light.data.use_shadow = False
    light.data.energy = min(light.data.energy, 80.0)


def point_camera(target):
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()


def show_all():
    for item in scene.objects:
        item.hide_render = False


show_all()

camera.location = (18.0, -28.0, 22.0)
camera.data.lens = 58
point_camera((31.0, 8.25, 16.0))
scene.render.filepath = str(OUT / "OceanFreighter-Blender-Crane.png")
bpy.ops.render.render(write_still=True)

for obj in scene.objects:
    obj.hide_render = (
        obj.name == "Accommodation_Roof"
        or "Accommodation_SideWall_-1" in obj.name
        or "Accommodation_Corridor_-1" in obj.name
        or obj.name.startswith("Officers_")
        or obj.name.startswith("Officer_")
        or obj.name.startswith("Bridge_")
    )
# High three-quarter view reads the room layout without placing the lens behind
# a bunk, desk or corridor opening.
camera.location = (-46.5, -7.2, 21.0)
camera.data.lens = 45
point_camera((-46.5, -4.9, 13.65))
scene.render.filepath = str(OUT / "OceanFreighter-Blender-Cabin.png")
bpy.ops.render.render(write_still=True)

show_all()
for obj in scene.objects:
    obj.hide_render = (
        obj.name == "Hull_Complete"
        or (obj.name.startswith("Frame_") and obj.location.y < 0)
        or (obj.name.startswith("Deck_") and not obj.name.startswith("Deck_Tank"))
        or any(collection.name == "30_SUPERSTRUCTURE" for collection in obj.users_collection)
        or obj.name.startswith("Bulkhead_")
    )
camera.location = (-50.0, -7.0, 5.8)
camera.data.lens = 30
point_camera((-43.0, 0.0, 5.2))
scene.render.filepath = str(OUT / "OceanFreighter-Blender-EngineRoom.png")
bpy.ops.render.render(write_still=True)

show_all()
for obj in scene.objects:
    obj.hide_render = (
        obj.name == "Hull_Complete"
        or (obj.name.startswith("Frame_") and obj.location.y < 0)
        or (obj.name.startswith("Hold1_Hatch") and "Coaming" not in obj.name)
        or obj.name.startswith("Deck_Lower_PortWalk")
        or obj.name.startswith("Deck_Tween_PortWalk")
        or obj.name.startswith("Deck_Weather_PortWalk")
    )
camera.location = (-17.4, -15.5, 11.5)
camera.data.lens = 46
point_camera((-17.4, -6.7, 6.2))
scene.render.filepath = str(OUT / "OceanFreighter-Blender-HoldAccess.png")
bpy.ops.render.render(write_still=True)

show_all()
for obj in scene.objects:
    obj.hide_render = (
        obj.name == "Hull_Complete"
        or (obj.name.startswith("Frame_") and obj.location.y < 0)
        or obj.name.startswith("Deck_Tween")
        or obj.name.startswith("Deck_Weather")
        or obj.name.startswith("Bulkhead_")
        or obj.name.startswith(("MainEngine_", "Generator_", "EngineCatwalk_", "EngineRoom", "BilgeFirePump_"))
    )
camera.location = (-52.0, -12.0, 8.8)
camera.data.lens = 45
point_camera((-52.3, -6.2, 3.65))
scene.render.filepath = str(OUT / "OceanFreighter-Blender-EngineAccess.png")
bpy.ops.render.render(write_still=True)

print("CALMY_OCEAN_FREIGHTER_DETAIL_REVIEWS_PASS")
