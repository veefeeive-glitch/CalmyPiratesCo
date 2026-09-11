"""Author the below-deck systems kit for the complete coastal freighter.

The existing exterior/interior source scenes stay untouched. This scene adds a
deck with real stairwell/gangway openings and ship-specific lower-deck systems.
Every asset is exported independently for deterministic Unreal import.
"""
from pathlib import Path
import math
import bpy

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "SourceAssets" / "Generated" / "FullFreighter"
OUTPUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1.0


def material(name, color, metallic=0.0, roughness=0.5, emission=None):
    result = bpy.data.materials.new(name)
    result.diffuse_color = color
    result.metallic = metallic
    result.roughness = roughness
    if emission:
        result.use_nodes = True
        node = result.node_tree.nodes.get("Principled BSDF")
        node.inputs["Base Color"].default_value = color
        node.inputs["Emission Color"].default_value = emission
        node.inputs["Emission Strength"].default_value = 2.5
    return result


M = {
    "hull": material("MAT_FullShip_Hull", (0.018, 0.065, 0.095, 1), 0.78, 0.34),
    "deck": material("MAT_FullShip_Deck", (0.055, 0.105, 0.12, 1), 0.72, 0.42),
    "steel": material("MAT_FullShip_Steel", (0.28, 0.34, 0.35, 1), 0.65, 0.46),
    "dark": material("MAT_FullShip_Machinery", (0.018, 0.026, 0.03, 1), 0.88, 0.28),
    "orange": material("MAT_FullShip_Safety", (0.95, 0.14, 0.012, 1), 0.30, 0.33),
    "blue": material("MAT_FullShip_Equipment", (0.025, 0.16, 0.24, 1), 0.58, 0.38),
    "red": material("MAT_FullShip_Emergency", (0.62, 0.01, 0.006, 1), 0.40, 0.36),
    "cream": material("MAT_FullShip_Crew", (0.55, 0.49, 0.37, 1), 0.05, 0.75),
    "screen": material("MAT_FullShip_Screen", (0.005, 0.25, 0.33, 1), 0.20, 0.16, (0.0, 0.65, 0.9, 1)),
}


def bevel(obj, width=0.035, segments=2):
    modifier = obj.modifiers.new("EdgeSoftening", "BEVEL")
    modifier.width = width
    modifier.segments = segments
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)


def box(name, location, dimensions, mat, edge=0.03):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if edge:
        bevel(obj, min(edge, min(dimensions) * 0.18))
    obj.data.materials.append(mat)
    return obj


def cylinder(name, location, radius, depth, mat, rotation=(0, 0, 0), vertices=24, edge=0.025):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    if edge:
        bevel(obj, edge)
    obj.data.materials.append(mat)
    return obj


def torus(name, location, major, minor, mat, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=28, minor_segments=10,
                                    location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    return obj


def join(parts, name):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    result = parts[0]
    result.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    for polygon in result.data.polygons:
        polygon.use_smooth = False
    return result


def export_asset(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.export_scene.gltf(filepath=str(OUTPUT / f"{obj.name}.glb"), export_format="GLB", use_selection=True,
                              export_apply=True, export_yup=True)


assets = []

# Joined plates preserve an actual stairwell and the existing open gangway.
deck_parts = [
    box("DeckAft", (-14.95, 0, -0.06), (5.1, 19.0, 0.12), M["deck"], 0.015),
    box("DeckHatchPort", (-10.8, -6.9, -0.06), (3.2, 5.2, 0.12), M["deck"], 0.015),
    box("DeckHatchStarboard", (-10.8, 3.4, -0.06), (3.2, 12.2, 0.12), M["deck"], 0.015),
    box("DeckMidAft", (-7.1, 0, -0.06), (4.2, 19.0, 0.12), M["deck"], 0.015),
    box("DeckGangway", (-1.0, 2.25, -0.06), (8.0, 14.5, 0.12), M["deck"], 0.015),
    box("DeckMidBow", (8.25, 0, -0.06), (10.5, 19.0, 0.12), M["deck"], 0.015),
    box("DeckBow", (15.75, 0, -0.06), (4.5, 8.0, 0.12), M["deck"], 0.015),
]
assets.append(join(deck_parts, "SM_MainDeckFull"))
assets.append(box("SM_LowerDeckFloor", (0, 0, 0.06), (28.0, 13.0, 0.12), M["deck"], 0.018))

parts = [
    box("EngineBed", (0, 0, 0.18), (6.2, 3.1, 0.36), M["dark"], 0.10),
    box("EngineBlock", (0, 0, 1.15), (5.5, 2.55, 1.70), M["blue"], 0.18),
    box("EngineHead", (-0.3, 0, 2.18), (4.2, 2.15, 0.42), M["steel"], 0.10),
    cylinder("Flywheel", (2.25, 0, 1.18), 0.82, 2.75, M["orange"], (math.pi / 2, 0, 0), 32),
]
for x in (-1.65, -0.55, 0.55, 1.65):
    parts.append(cylinder("CylinderHead", (x, 0, 2.55), 0.20, 0.62, M["dark"], vertices=20))
assets.append(join(parts, "SM_MainEngine"))

parts = [cylinder("TankBody", (0, 0, 1.05), 1.02, 3.8, M["steel"], (0, math.pi / 2, 0), 32)]
for x in (-1.55, 1.55):
    parts.append(torus("TankBand", (x, 0, 1.05), 1.04, 0.055, M["orange"], (0, math.pi / 2, 0)))
    parts.append(box("TankFoot", (x, 0, 0.18), (0.22, 1.45, 0.36), M["dark"], 0.04))
parts.append(cylinder("TankValve", (0, 0, 2.18), 0.12, 0.42, M["red"], vertices=16))
assets.append(join(parts, "SM_FuelTank"))

parts = [box("PumpBase", (0, 0, 0.14), (3.2, 1.6, 0.28), M["dark"], 0.08)]
for x in (-0.92, 0.0, 0.92):
    parts += [cylinder("Pump", (x, 0, 0.72), 0.39, 0.75, M["blue"], (math.pi / 2, 0, 0), 24),
              torus("PumpWheel", (x, -0.52, 0.80), 0.25, 0.045, M["orange"], (math.pi / 2, 0, 0))]
assets.append(join(parts, "SM_PumpSkid"))

parts = []
for x in (-0.8, 0, 0.8):
    parts += [box("Cabinet", (x, 0, 1.05), (0.72, 0.48, 2.10), M["blue"], 0.06),
              box("Panel", (x, -0.26, 1.15), (0.50, 0.045, 1.35), M["dark"], 0.025),
              cylinder("Lamp", (x, -0.30, 1.62), 0.05, 0.04, M["red"], (math.pi / 2, 0, 0), 12, 0)]
assets.append(join(parts, "SM_ElectricalCabinets"))

parts = [box("Table", (0, 0, 0.82), (2.3, 1.25, 0.16), M["cream"], 0.07)]
for x in (-0.88, 0.88):
    for y in (-0.45, 0.45):
        parts.append(box("Leg", (x, y, 0.40), (0.10, 0.10, 0.80), M["dark"], 0.02))
for x, y in ((-1.45, 0), (1.45, 0), (0, -1.02), (0, 1.02)):
    parts += [box("Seat", (x, y, 0.48), (0.62, 0.62, 0.12), M["orange"], 0.05),
              cylinder("SeatPost", (x, y, 0.23), 0.055, 0.46, M["dark"], vertices=12, edge=0)]
assets.append(join(parts, "SM_MessTableSet"))

parts = [box("WashroomShell", (0, 0, 1.25), (2.4, 1.7, 2.5), M["steel"], 0.08),
         box("WashroomDoor", (0, -0.88, 1.10), (0.88, 0.08, 2.12), M["blue"], 0.04),
         box("Sink", (-0.62, -0.96, 0.92), (0.72, 0.36, 0.18), M["cream"], 0.05),
         cylinder("DoorLamp", (0.62, -0.96, 1.72), 0.07, 0.05, M["screen"], (math.pi / 2, 0, 0), 14, 0)]
assets.append(join(parts, "SM_WashroomUnit"))

parts = [box("AirHandler", (0, 0, 0.92), (3.1, 1.15, 1.84), M["steel"], 0.10)]
for x in (-0.92, 0, 0.92):
    parts.append(cylinder("Fan", (x, -0.62, 1.05), 0.36, 0.08, M["dark"], (math.pi / 2, 0, 0), 24, 0.01))
parts.append(box("AirDuct", (0, 0, 2.02), (2.1, 0.76, 0.36), M["blue"], 0.06))
assets.append(join(parts, "SM_AirHandler"))

parts = [box("Pallet", (0, 0, 0.10), (2.4, 1.6, 0.20), M["cream"], 0.03)]
for z in (0.52, 1.30):
    for x in (-0.62, 0.62):
        parts.append(box("Cargo", (x, 0, z), (1.05, 1.25, 0.70), M["blue" if z < 1 else "orange"], 0.07))
assets.append(join(parts, "SM_CargoPalletStack"))

parts = [box("BilgeBase", (0, 0, 0.12), (1.8, 1.1, 0.24), M["dark"], 0.07),
         cylinder("BilgeMotor", (-0.35, 0, 0.58), 0.35, 0.82, M["blue"], (0, math.pi / 2, 0), 24),
         cylinder("BilgePipe", (0.48, 0, 0.72), 0.13, 1.10, M["orange"], vertices=16),
         torus("BilgeValve", (0.48, -0.18, 1.08), 0.22, 0.04, M["red"], (math.pi / 2, 0, 0))]
assets.append(join(parts, "SM_BilgePump"))

parts = [box("Shaft", (0, 0, 0.58), (2.4, 0.7, 1.16), M["dark"], 0.09),
         cylinder("ShaftLine", (0, 0, 0.58), 0.16, 5.4, M["orange"], (0, math.pi / 2, 0), 20),
         cylinder("BearingL", (-1.65, 0, 0.58), 0.34, 0.62, M["steel"], (0, math.pi / 2, 0), 20),
         cylinder("BearingR", (1.65, 0, 0.58), 0.34, 0.62, M["steel"], (0, math.pi / 2, 0), 20)]
assets.append(join(parts, "SM_PropellerShaft"))

for asset in assets:
    export_asset(asset)

for index, asset in enumerate(assets):
    asset.location = ((index % 4) * 8.0, (index // 4) * 7.0, 0)

bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "CalmyFullFreighter.blend"))
print(f"CALMY_FULL_FREIGHTER_KIT_PASS assets={len(assets)} output={OUTPUT}")
