"""Audit the saved Ocean Freighter master for known layout regressions."""
import json
import bpy

HULL_STATIONS = [
    (-61.0, 6.4), (-57.0, 7.7), (-50.0, 9.3), (-38.0, 10.0),
    (-20.0, 10.2), (0.0, 10.2), (22.0, 10.15), (42.0, 9.7),
    (52.0, 8.2), (58.0, 4.4), (61.0, 0.45),
]


def half_beam_at(x):
    if x <= HULL_STATIONS[0][0]:
        return HULL_STATIONS[0][1]
    if x >= HULL_STATIONS[-1][0]:
        return HULL_STATIONS[-1][1]
    for (x0, beam0), (x1, beam1) in zip(HULL_STATIONS, HULL_STATIONS[1:]):
        if x0 <= x <= x1:
            alpha = (x - x0) / (x1 - x0)
            return beam0 + (beam1 - beam0) * alpha
    raise RuntimeError(f"No beam at {x}")


scene = bpy.context.scene
water_objects = [obj.name for obj in scene.objects if (
    obj.name.lower() == "water"
    or obj.name.lower().startswith(("water_", "ocean_surface", "sea_surface"))
)]
if water_objects:
    raise RuntimeError(f"Water remains in master: {water_objects}")

deck_objects = [obj for obj in scene.objects if obj.type == "MESH" and obj.name.startswith("Deck_")]
if len(deck_objects) < 10:
    raise RuntimeError(f"Expected modular deck plates, found only {len(deck_objects)}")

violations = []
clearance_min = 1000.0
for obj in deck_objects:
    for vertex in obj.data.vertices:
        world = obj.matrix_world @ vertex.co
        clearance = half_beam_at(world.x) * 0.95 - abs(world.y)
        clearance_min = min(clearance_min, clearance)
        if clearance < -0.001:
            violations.append({"object": obj.name, "x": round(world.x, 3), "y": round(world.y, 3), "clearance": round(clearance, 4)})
if violations:
    raise RuntimeError("Deck exceeds hull envelope: " + json.dumps(violations[:12]))

required = ("Hull_Complete", "MainPropeller", "MainPropeller_Hub", "Stern_ShaftBoss", "Rudder", "BulbousBow",
            "Lifeboat_Port", "Lifeboat_Starboard", "Bridge_MainConsole", "MainEngine_Block",
            "Funnel", "CAM_Walkthrough_FirstPerson", "CargoCrane_0_Boom", "CargoCrane_1_Boom",
            "Bow_Windlass_Gearbox", "Aft_Windlass_Gearbox", "Vent_0_Bell", "FireStation_0_Hose")
missing = [name for name in required if name not in scene.objects]
if missing:
    raise RuntimeError(f"Required ship systems missing: {missing}")

obsolete = [obj.name for obj in scene.objects if (
    obj.name == "BowThrusterTunnel"
    or obj.name.startswith("Propeller_Blade_")
    or obj.name.startswith("Crane_")
    or obj.name.startswith("FireHose_")
    or obj.name.endswith("_Cowl")
    or obj.name in ("Bow_WindlassBed", "Aft_WindlassBed")
)]
if obsolete:
    raise RuntimeError(f"Placeholder exterior parts returned: {obsolete}")

if scene.camera is None or scene.camera.name != "CAM_Walkthrough_FirstPerson":
    raise RuntimeError(f"Master does not open on walkthrough camera: {scene.camera}")
if scene.get("walkthrough_camera") != "CAM_Walkthrough_FirstPerson":
    raise RuntimeError("Walkthrough metadata missing")

propeller = scene.objects["MainPropeller"]
rudder = scene.objects["Rudder"]
propeller_x = sum((propeller.matrix_world @ vertex.co).x for vertex in propeller.data.vertices) / len(propeller.data.vertices)
rudder_forward_x = max((rudder.matrix_world @ vertex.co).x for vertex in rudder.data.vertices)
if propeller_x >= HULL_STATIONS[0][0]:
    raise RuntimeError(f"Propeller remains inside the transom envelope: x={propeller_x}")
if rudder_forward_x >= propeller_x:
    raise RuntimeError(f"Rudder/propeller order invalid: rudder_forward={rudder_forward_x} prop={propeller_x}")

external_frames = []
for obj in scene.objects:
    if not obj.name.startswith("Frame_"):
        continue
    allowed_y = half_beam_at(obj.location.x) * 0.80
    if abs(obj.location.y) > allowed_y:
        external_frames.append(obj.name)
if external_frames:
    raise RuntimeError(f"Internal frames pierce the shell: {external_frames[:12]}")

pbr_materials = ("MAT_Deck_Green_PBR", "MAT_Steel_PBR", "MAT_Interior_Wood_PBR", "MAT_DiamondPlate_PBR")
missing_pbr = [name for name in pbr_materials if name not in bpy.data.materials]
if missing_pbr:
    raise RuntimeError(f"PBR material contract failed: {missing_pbr}")
external_images = [image for image in bpy.data.images if "ambientCG" in image.filepath]
unpacked_images = [image.name for image in external_images if image.packed_file is None]
if unpacked_images:
    raise RuntimeError(f"External PBR images are not packed: {unpacked_images}")

lifeboat_windows = [obj for obj in scene.objects if obj.name.startswith("Lifeboat_") and "Window" in obj.name]
access_steps = [obj for obj in scene.objects if obj.name.startswith("AccessTower_") and "Step_" in obj.name]
corridor_frames = [obj for obj in scene.objects if "Corridor_" in obj.name and "Frame_" in obj.name]
if len(lifeboat_windows) < 8 or len(access_steps) < 128 or len(corridor_frames) < 80:
    raise RuntimeError(f"Ship detail contract failed: lifeboat_windows={len(lifeboat_windows)} "
                       f"access_steps={len(access_steps)} corridor_frames={len(corridor_frames)}")

print(f"CALMY_OCEAN_FREIGHTER_AUDIT_PASS water=0 deck_plates={len(deck_objects)} "
      f"minimum_hull_clearance_m={clearance_min:.3f} lifeboat_windows={len(lifeboat_windows)} "
      f"access_steps={len(access_steps)} corridor_frames={len(corridor_frames)} "
      f"walkthrough_camera={scene.camera.name} internal_frames={len(external_frames)} "
      f"propeller_x={propeller_x:.2f} rudder_forward_x={rudder_forward_x:.2f} "
      f"packed_pbr_images={len(external_images)} objects={len(scene.objects)}")
