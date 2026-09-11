"""Author an editable modular coastal-freighter interior kit in Blender 5.2+.

The kit is deliberately stylised but dimensioned in metres around a 2.7 m deck
height. Every module is exported as a separate GLB for Unreal Interchange and the
complete source scene remains in an editable .blend file.
"""
from pathlib import Path
import math
import bpy

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "SourceAssets" / "Generated" / "FreighterInterior"
OUTPUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1.0


def material(name, rgba, metallic=0.0, roughness=0.55, emission=None):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = rgba
    mat.metallic = metallic
    mat.roughness = roughness
    if emission:
        mat.use_nodes = True
        principled = mat.node_tree.nodes.get("Principled BSDF")
        principled.inputs["Base Color"].default_value = rgba
        principled.inputs["Emission Color"].default_value = emission
        principled.inputs["Emission Strength"].default_value = 3.0
    return mat


M = {
    "wall": material("MAT_Interior_WarmGrey", (0.46, 0.51, 0.50, 1), 0.35, 0.55),
    "floor": material("MAT_Interior_Deck", (0.055, 0.10, 0.12, 1), 0.72, 0.40),
    "dark": material("MAT_Interior_Machinery", (0.025, 0.035, 0.04, 1), 0.85, 0.28),
    "orange": material("MAT_Interior_Safety", (0.95, 0.16, 0.02, 1), 0.28, 0.34),
    "red": material("MAT_Interior_Emergency", (0.62, 0.012, 0.008, 1), 0.35, 0.38),
    "blue": material("MAT_Interior_EquipmentBlue", (0.035, 0.19, 0.28, 1), 0.58, 0.36),
    "cream": material("MAT_Interior_Bedding", (0.62, 0.56, 0.43, 1), 0.0, 0.78),
    "wood": material("MAT_Interior_Worktop", (0.23, 0.11, 0.045, 1), 0.05, 0.68),
    "screen": material("MAT_Interior_Screen", (0.01, 0.22, 0.28, 1), 0.25, 0.15, (0.0, 0.55, 0.8, 1)),
    "light": material("MAT_Interior_Light", (0.9, 0.82, 0.62, 1), 0.0, 0.22, (1.0, 0.72, 0.38, 1)),
}


def bevel(obj, width=0.035, segments=2):
    mod = obj.modifiers.new("EdgeSoftening", "BEVEL")
    mod.width = width
    mod.segments = segments
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)


def box(name, location, dimensions, mat, edge=0.025):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if edge:
        bevel(obj, min(edge, min(dimensions) * 0.18))
    obj.data.materials.append(mat)
    return obj


def cylinder(name, location, radius, depth, mat, rotation=(0, 0, 0), vertices=20, edge=0.025):
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
    for face in result.data.polygons:
        face.use_smooth = False
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
assets.append(box("SM_InteriorFloor8x8", (0, 0, 0.06), (8.0, 8.0, 0.12), M["floor"], 0.02))
assets.append(box("SM_InteriorWall4m", (0, 0, 1.35), (4.0, 0.14, 2.70), M["wall"], 0.035))
assets.append(box("SM_InteriorCeiling4m", (0, 0, 0.07), (4.0, 4.0, 0.14), M["wall"], 0.025))

parts = [
    box("BulkheadLeft", (-1.35, 0, 1.35), (1.30, 0.16, 2.70), M["wall"]),
    box("BulkheadRight", (1.35, 0, 1.35), (1.30, 0.16, 2.70), M["wall"]),
    box("BulkheadTop", (0, 0, 2.48), (1.40, 0.16, 0.44), M["wall"]),
    box("DoorFrameL", (-0.72, -0.03, 1.13), (0.12, 0.22, 2.26), M["orange"], 0.018),
    box("DoorFrameR", (0.72, -0.03, 1.13), (0.12, 0.22, 2.26), M["orange"], 0.018),
    box("DoorFrameTop", (0, -0.03, 2.23), (1.56, 0.22, 0.12), M["orange"], 0.018),
]
assets.append(join(parts, "SM_BulkheadDoorFrame"))

parts = [
    box("Door", (0, 0, 1.05), (1.18, 0.16, 2.10), M["blue"], 0.08),
    box("DoorInset", (0, -0.09, 1.05), (0.82, 0.04, 1.58), M["dark"], 0.035),
    torus("DoorWheel", (0, -0.18, 1.05), 0.24, 0.045, M["orange"], (math.pi / 2, 0, 0)),
]
assets.append(join(parts, "SM_WatertightDoor"))

steps = []
for index in range(10):
    x = -1.35 + index * 0.30
    z = 0.13 + index * 0.24
    steps.append(box(f"Step{index:02d}", (x, 0, z), (0.32, 1.20, 0.24), M["floor"], 0.015))
steps += [
    cylinder("StairRailL", (0, -0.66, 1.42), 0.035, 3.55, M["orange"], (0, math.pi / 2 - math.atan2(2.16, 2.7), 0), 12, 0),
    cylinder("StairRailR", (0, 0.66, 1.42), 0.035, 3.55, M["orange"], (0, math.pi / 2 - math.atan2(2.16, 2.7), 0), 12, 0),
]
assets.append(join(steps, "SM_StairFlight"))

parts = [box("ConsoleBody", (0, 0, 0.55), (2.8, 0.75, 1.10), M["blue"], 0.09),
         box("ConsoleTop", (0, -0.15, 1.13), (2.9, 0.85, 0.16), M["dark"], 0.04)]
for x in (-0.92, 0, 0.92):
    parts.append(box("Display", (x, -0.49, 1.30), (0.64, 0.07, 0.42), M["screen"], 0.035))
    for dx in (-0.17, 0, 0.17):
        parts.append(cylinder("Button", (x + dx, -0.51, 0.92), 0.035, 0.04, M["orange"], (math.pi / 2, 0, 0), 12, 0))
assets.append(join(parts, "SM_BridgeConsole"))

parts = [cylinder("HelmPost", (0, 0, 0.63), 0.18, 1.26, M["dark"], vertices=20),
         box("HelmHead", (0, 0, 1.25), (0.62, 0.42, 0.25), M["blue"], 0.06),
         torus("HelmWheel", (0, -0.27, 1.32), 0.34, 0.045, M["orange"], (math.pi / 2, 0, 0))]
for angle in range(0, 360, 60):
    radians = math.radians(angle)
    parts.append(cylinder("HelmSpoke", (math.cos(radians) * 0.17, -0.27, 1.32 + math.sin(radians) * 0.17),
                          0.018, 0.42, M["orange"], (0, math.pi / 2 - radians, math.pi / 2), 10, 0))
assets.append(join(parts, "SM_HelmStation"))

parts = [box("ChartTable", (0, 0, 0.92), (1.75, 1.05, 0.18), M["wood"], 0.055)]
for x in (-0.72, 0.72):
    for y in (-0.38, 0.38):
        parts.append(box("TableLeg", (x, y, 0.43), (0.10, 0.10, 0.86), M["dark"], 0.02))
parts.append(box("ChartScreen", (0, 0, 1.04), (1.30, 0.72, 0.05), M["screen"], 0.02))
assets.append(join(parts, "SM_NavigationTable"))

parts = [box("GeneratorBase", (0, 0, 0.14), (2.7, 1.25, 0.28), M["dark"], 0.08),
         box("GeneratorBody", (0, 0, 0.92), (2.45, 1.05, 1.38), M["blue"], 0.13),
         box("GeneratorPanel", (-0.75, -0.56, 1.04), (0.72, 0.08, 0.70), M["dark"], 0.04),
         cylinder("GeneratorRotor", (0.60, 0, 0.95), 0.43, 1.15, M["orange"], (math.pi / 2, 0, 0), 24)]
for x in (-0.92, -0.72, -0.52):
    parts.append(cylinder("Gauge", (x, -0.62, 1.18), 0.075, 0.05, M["light"], (math.pi / 2, 0, 0), 16, 0))
assets.append(join(parts, "SM_EngineGenerator"))

parts = [box("ManifoldBack", (0, 0.12, 1.15), (2.5, 0.18, 2.15), M["dark"], 0.05)]
for x in (-0.85, -0.28, 0.28, 0.85):
    parts.append(cylinder("Pipe", (x, -0.08, 1.12), 0.095, 1.85, M["orange"], vertices=16))
    parts.append(torus("Valve", (x, -0.26, 1.48), 0.20, 0.035, M["red"], (math.pi / 2, 0, 0)))
assets.append(join(parts, "SM_EngineManifold"))

parts = []
for z, radius, mat in ((0.42, 0.11, M["orange"]), (0.80, 0.085, M["blue"]), (1.16, 0.07, M["red"])):
    parts.append(cylinder("PipeRun", (0, 0, z), radius, 4.0, mat, (0, math.pi / 2, 0), 16, 0.018))
for x in (-1.7, 0, 1.7):
    parts.append(box("PipeBracket", (x, 0, 0.78), (0.10, 0.22, 1.10), M["dark"], 0.015))
assets.append(join(parts, "SM_PipeRun4m"))

parts = [box("BenchTop", (0, 0, 0.90), (2.4, 0.78, 0.16), M["wood"], 0.04),
         box("BenchBack", (0, 0.36, 1.55), (2.4, 0.10, 1.20), M["dark"], 0.03)]
for x in (-1.0, 1.0):
    parts.append(box("BenchLeg", (x, 0, 0.43), (0.12, 0.60, 0.86), M["dark"], 0.02))
for x in (-0.75, 0, 0.75):
    parts.append(box("Tool", (x, 0.29, 1.62), (0.42, 0.08, 0.18), M["orange"], 0.025))
assets.append(join(parts, "SM_Workbench"))

parts = []
for index, x in enumerate((-0.90, -0.30, 0.30, 0.90)):
    parts.append(box("Locker", (x, 0, 1.05), (0.56, 0.58, 2.10), M["blue"], 0.045))
    parts.append(box("LockerVent", (x, -0.31, 1.55), (0.28, 0.035, 0.12), M["dark"], 0.01))
    parts.append(cylinder("LockerHandle", (x + 0.17, -0.34, 1.02), 0.025, 0.10, M["orange"], (math.pi / 2, 0, 0), 10, 0))
assets.append(join(parts, "SM_LockerBank"))

parts = []
for z in (0.46, 1.58):
    parts += [box("BunkFrame", (0, 0, z), (2.05, 0.92, 0.14), M["dark"], 0.035),
              box("Mattress", (0, 0, z + 0.12), (1.92, 0.82, 0.16), M["cream"], 0.05),
              box("Pillow", (-0.72, 0, z + 0.24), (0.42, 0.62, 0.16), M["wall"], 0.06)]
for x in (-0.94, 0.94):
    parts.append(box("BunkPost", (x, 0, 1.05), (0.08, 0.08, 2.10), M["orange"], 0.015))
assets.append(join(parts, "SM_BunkPair"))

parts = [box("GalleyBase", (0, 0, 0.52), (2.4, 0.78, 1.04), M["wall"], 0.06),
         box("GalleyTop", (0, 0, 1.08), (2.46, 0.84, 0.12), M["dark"], 0.035),
         box("GalleyUpper", (0.62, 0.12, 1.75), (1.12, 0.58, 1.12), M["blue"], 0.06),
         cylinder("Sink", (-0.58, -0.10, 1.16), 0.25, 0.06, M["floor"], vertices=20, edge=0),
         cylinder("Tap", (-0.58, 0.04, 1.42), 0.035, 0.48, M["orange"], vertices=12, edge=0)]
assets.append(join(parts, "SM_GalleyUnit"))

parts = []
for z in (0.18, 1.08, 1.98):
    parts.append(box("RackShelf", (0, 0, z), (2.5, 0.78, 0.12), M["dark"], 0.025))
for x in (-1.18, 1.18):
    parts.append(box("RackPost", (x, 0, 1.08), (0.10, 0.72, 2.16), M["orange"], 0.018))
for x in (-0.72, 0, 0.72):
    parts.append(box("RackCrate", (x, 0, 0.62), (0.58, 0.58, 0.58), M["wood"], 0.045))
assets.append(join(parts, "SM_CargoRack"))

parts = [cylinder("LadderLeft", (-0.34, 0, 1.35), 0.045, 2.7, M["orange"], vertices=12, edge=0),
         cylinder("LadderRight", (0.34, 0, 1.35), 0.045, 2.7, M["orange"], vertices=12, edge=0)]
for z in [0.20 + i * 0.31 for i in range(8)]:
    parts.append(cylinder("LadderRung", (0, 0, z), 0.035, 0.68, M["orange"], (0, math.pi / 2, 0), 12, 0))
assets.append(join(parts, "SM_Ladder"))

parts = [box("DuctMain", (0, 0, 0), (3.4, 0.62, 0.52), M["wall"], 0.06)]
for x in (-1.15, 0, 1.15):
    parts.append(box("DuctBand", (x, 0, 0), (0.10, 0.70, 0.60), M["dark"], 0.015))
assets.append(join(parts, "SM_VentDuct"))

parts = [box("LightHousing", (0, 0, 0), (1.25, 0.34, 0.12), M["dark"], 0.04),
         box("LightTube", (0, -0.04, -0.08), (1.02, 0.22, 0.08), M["light"], 0.03),
         box("LightGuardL", (-0.52, -0.05, -0.10), (0.035, 0.30, 0.18), M["orange"], 0.01),
         box("LightGuardR", (0.52, -0.05, -0.10), (0.035, 0.30, 0.18), M["orange"], 0.01)]
assets.append(join(parts, "SM_CeilingLight"))

parts = [box("JunctionBody", (0, 0, 0.55), (0.72, 0.28, 1.10), M["blue"], 0.07),
         box("JunctionPanel", (0, -0.16, 0.58), (0.48, 0.05, 0.72), M["dark"], 0.03),
         cylinder("JunctionLamp", (0, -0.20, 0.80), 0.065, 0.05, M["red"], (math.pi / 2, 0, 0), 14, 0),
         cylinder("JunctionSwitch", (0, -0.20, 0.48), 0.045, 0.05, M["orange"], (math.pi / 2, 0, 0), 12, 0)]
assets.append(join(parts, "SM_JunctionBox"))

for asset in assets:
    export_asset(asset)

for index, asset in enumerate(assets):
    asset.location = ((index % 5) * 5.5, (index // 5) * 6.0, 0)

bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "CalmyFreighterInterior.blend"))
print(f"CALMY_FREIGHTER_INTERIOR_KIT_PASS assets={len(assets)} output={OUTPUT}")
