"""Render a repeatable close-up inspection sweep of the Ocean Freighter."""
from pathlib import Path
import json
import os

import bpy
from mathutils import Vector


ROOT = Path(__file__).resolve().parents[1]
RUN_LABEL = os.environ.get("CALMY_SWEEP_LABEL", "20260904-R2")
OUT = ROOT / "Saved" / "Verification" / f"OceanFreighterSweep-{RUN_LABEL}"
OUT.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
camera = bpy.data.objects["CAM_ReviewCloseup"]
scene.camera = camera
scene.render.resolution_x = 960
scene.render.resolution_y = 540
scene.render.resolution_percentage = 100
camera.data.clip_start = 0.05

# A camera-side inspection light makes geometric defects readable regardless of
# the production lighting state. It exists only in this headless audit process;
# the master .blend is never saved by this script.
qa_light_data = bpy.data.lights.new("QA_InspectionKey", type="AREA")
qa_light_data.energy = 1800.0
qa_light_data.shape = "DISK"
qa_light_data.size = 9.0
qa_light = bpy.data.objects.new("QA_InspectionKey", qa_light_data)
scene.collection.objects.link(qa_light)

if scene.world and scene.world.use_nodes:
    background = scene.world.node_tree.nodes.get("Background")
    if background:
        background.inputs["Strength"].default_value = max(
            float(background.inputs["Strength"].default_value), 0.16
        )

for light in (obj for obj in scene.objects if obj.type == "LIGHT" and obj.name.startswith("InteriorLight_")):
    light.data.use_shadow = False
    light.data.energy = min(light.data.energy, 80.0)


def in_collection(obj, collection_name):
    return any(collection.name == collection_name for collection in obj.users_collection)


def point_camera(target):
    camera.rotation_euler = (Vector(target) - camera.location).to_track_quat("-Z", "Y").to_euler()
    qa_light.location = camera.location
    qa_light.rotation_euler = camera.rotation_euler


def hidden_for_mode(obj, mode):
    name = obj.name
    if mode == "exterior":
        return False
    if mode == "boat_cut":
        return (
            name == "BoatDeck_Roof"
            or "BoatDeck_SideWall_-1" in name
            or name.startswith(("Accommodation_", "Officers_", "Officer_", "Bridge_"))
        )
    if mode == "accommodation_cut":
        return (
            name == "Accommodation_Roof"
            or "Accommodation_SideWall_-1" in name
            or name.startswith(("Lifeboat_", "AccessTower_", "BridgeWing_"))
            or name.startswith(("Officers_", "Officer_", "Bridge_"))
        )
    if mode == "officers_cut":
        return name == "Officers_Roof" or "Officers_SideWall_-1" in name or name.startswith("Bridge_")
    if mode == "bridge_cut":
        return name == "Bridge_Roof" or "Bridge_SideWall_-1" in name
    if mode in {"engine_port", "engine_starboard", "engine_overhead"}:
        near_side = obj.location.y < 0 if mode == "engine_port" else obj.location.y > 0
        return (
            name == "Hull_Complete"
            or in_collection(obj, "30_SUPERSTRUCTURE")
            or (name.startswith("Deck_") and not name.startswith("Deck_Tank"))
            or name.startswith("Bulkhead_")
            or (name.startswith("Frame_") and (near_side or mode == "engine_overhead"))
        )
    if mode == "steering":
        return (
            name == "Hull_Complete"
            or name.startswith(("Deck_Lower", "Deck_Tween", "Deck_Weather", "Bulkhead_"))
            or (name.startswith("Frame_") and obj.location.y < 0)
            or in_collection(obj, "30_SUPERSTRUCTURE")
        )
    if mode == "bow_machinery":
        return (
            name == "Hull_Complete"
            or name.startswith(("Deck_Lower", "Deck_Tween", "Deck_Weather", "Bulkhead_"))
            or (name.startswith("Frame_") and obj.location.y < 0)
        )
    if mode == "hold1":
        return (
            name == "Hull_Complete"
            or (name.startswith("Frame_") and obj.location.y < 0)
            or (name.startswith("Hold1_Hatch") and "Coaming" not in name)
            or name.startswith(("Deck_Lower_PortWalk", "Deck_Tween_PortWalk", "Deck_Weather_PortWalk"))
        )
    if mode == "hold2":
        return (
            name == "Hull_Complete"
            or (name.startswith("Frame_") and obj.location.y > 0)
            or (name.startswith("Hold2_Hatch") and "Coaming" not in name)
            or name.startswith(("Deck_Lower_StarboardWalk", "Deck_Tween_StarboardWalk", "Deck_Weather_StarboardWalk"))
        )
    if mode == "engine_access":
        return (
            name == "Hull_Complete"
            or name.startswith(("Deck_Tween", "Deck_Weather", "Bulkhead_", "MainEngine_", "Generator_",
                                "EngineCatwalk_", "EngineRoom", "BilgeFirePump_"))
            or (name.startswith("Frame_") and obj.location.y < 0)
        )
    return False


SHOTS = [
    ("01_Exterior_Port_Profile", "exterior", (0, -112, 15), (0, 0, 8), 56),
    ("02_Exterior_Starboard_Profile", "exterior", (0, 112, 15), (0, 0, 8), 56),
    ("03_Exterior_Stern_Quarter", "exterior", (-78, -28, 11), (-61, 0, 6), 52),
    ("04_Exterior_Bow_Quarter", "exterior", (77, -27, 14), (54, 0, 9), 52),
    ("05_WeatherDeck_Aft", "exterior", (-42, -24, 18), (-35, 0, 12), 48),
    ("06_WeatherDeck_Hold1", "exterior", (-5, -24, 18), (-5, 0, 12), 48),
    ("07_WeatherDeck_Hold2", "exterior", (28, -24, 18), (28, 0, 12), 48),
    ("08_Forecastle_Equipment", "exterior", (52, -22, 16), (51, 0, 11), 52),
    ("09_CargoCrane_Aft", "exterior", (18, -28, 22), (31, 8.25, 16), 58),
    ("10_AccessTower_Port", "exterior", (-31, -22, 18), (-35, -8.5, 16), 50),
    ("11_Lifeboat_Port", "exterior", (-45, -23, 18), (-43, -9, 16), 55),
    ("12_BoatDeck_Mess", "boat_cut", (-53.2, -4.5, 11.85), (-43.5, -4.5, 11.35), 30),
    ("13_BoatDeck_Sanitary", "boat_cut", (-39.2, -4.0, 11.75), (-35.4, -5.35, 11.30), 32),
    ("14_Accommodation_Cabin", "accommodation_cut", (-52.0, -2.1, 14.55), (-52.0, -5.0, 14.15), 28),
    ("15_Accommodation_Corridor", "accommodation_cut", (-54, -0.1, 14.6), (-33, 0, 14.6), 34),
    ("16_Officer_Cabin", "officers_cut", (-49.5, -2.0, 17.35), (-49.5, -4.7, 17.15), 28),
    ("17_Bridge_Interior", "bridge_cut", (-38.0, 0.0, 20.25), (-32.5, 0.0, 20.05), 30),
    ("18_EngineRoom_Port", "engine_port", (-50, -7, 5.8), (-43, 0, 5.2), 30),
    ("19_EngineRoom_Starboard", "engine_starboard", (-48, 7.0, 5.8), (-42, 0, 5.2), 30),
    ("20_EngineRoom_HighOverview", "engine_port", (-54, -10, 10), (-43, 0, 4.8), 30),
    ("21_SteeringGear", "steering", (-62, -11, 7), (-56, 0, 3.4), 42),
    ("22_BowThrusterRoom", "bow_machinery", (57, -11, 7), (50, 0, 3.3), 42),
    ("23_Hold1_Longitudinal", "hold1", (-17, 0, 7.8), (14, 0, 4.8), 28),
    ("24_Hold1_Access", "hold1", (-17.4, 0, 7.2), (-17.4, -6.75, 6.2), 34),
    ("25_Hold2_Longitudinal", "hold2", (40, 0, 7.8), (17, 0, 4.8), 28),
    ("26_Hold2_Access", "hold2", (39.4, 0, 7.2), (39.4, 6.75, 6.2), 34),
    ("27_Engine_Main_Access_Top", "engine_access", (-48.5, -6.2, 6.6), (-51.2, -6.2, 4.45), 32),
    ("28_Engine_Emergency_Access_Top", "engine_access", (-41.5, 6.2, 6.6), (-38.8, 6.2, 4.45), 32),
    ("29_BoatDeck_Galley", "boat_cut", (-53.0, 4.25, 11.8), (-48.0, 5.9, 11.5), 30),
    ("30_BoatDeck_Laundry", "boat_cut", (-55.0, 2.7, 11.7), (-51.3, 5.4, 11.4), 30),
    ("31_Accommodation_Cabin_Starboard", "accommodation_cut", (-48.4, 2.1, 14.55), (-48.4, 5.0, 14.15), 28),
    ("32_Officer_Cabin_Starboard", "officers_cut", (-45.3, 2.0, 17.35), (-45.3, 4.7, 17.15), 28),
    ("33_Bridge_Console_Close", "bridge_cut", (-36.0, -2.5, 20.0), (-32.6, -1.2, 20.0), 34),
    ("34_Engine_Auxiliaries_Port", "engine_port", (-53.5, -8.0, 4.2), (-46.0, -5.3, 3.8), 34),
    ("35_Engine_Auxiliaries_Starboard", "engine_starboard", (-52.5, 8.0, 4.2), (-45.0, 5.3, 3.8), 34),
    ("36_Engine_Main_Access_Bottom", "engine_access", (-56.5, -3.6, 3.0), (-51.5, -6.2, 4.45), 32),
    ("37_Engine_Emergency_Access_Bottom", "engine_access", (-33.5, 3.6, 3.0), (-38.5, 6.2, 4.45), 32),
]

manifest = []
for shot_name, mode, location, target, lens in SHOTS:
    for obj in scene.objects:
        obj.hide_render = hidden_for_mode(obj, mode)
    camera.location = location
    camera.data.lens = lens
    point_camera(target)
    path = OUT / f"{shot_name}.png"
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    manifest.append({
        "name": shot_name,
        "mode": mode,
        "camera": location,
        "target": target,
        "lens_mm": lens,
        "path": str(path),
    })

(OUT / "inspection_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(f"CALMY_OCEAN_FREIGHTER_VISUAL_SWEEP_PASS shots={len(SHOTS)} output={OUT}")
