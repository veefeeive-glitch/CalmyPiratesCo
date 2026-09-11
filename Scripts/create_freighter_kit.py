"""Build a deterministic stylised coastal-freighter environment kit in Blender.

Run with Blender 5.2+ in background mode. Each object is exported as a separate
GLB for Unreal Interchange, and the complete editable source is saved as a .blend.
"""
from pathlib import Path
import math
import bpy

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "SourceAssets" / "Generated" / "FreighterKit"
OUTPUT.mkdir(parents=True, exist_ok=True)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.scale_length = 1.0


def material(name, rgba, metallic=0.0, roughness=0.55):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = rgba
    mat.metallic = metallic
    mat.roughness = roughness
    return mat


MATS = {
    "navy": material("MAT_Hull_Navy", (0.025, 0.10, 0.16, 1), 0.75, 0.32),
    "deck": material("MAT_Deck_BlueGrey", (0.12, 0.21, 0.25, 1), 0.65, 0.48),
    "white": material("MAT_Bridge_WarmWhite", (0.78, 0.82, 0.79, 1), 0.35, 0.42),
    "orange": material("MAT_Safety_Orange", (1.0, 0.22, 0.025, 1), 0.25, 0.34),
    "glass": material("MAT_Window_Teal", (0.015, 0.20, 0.27, 1), 0.65, 0.15),
    "dark": material("MAT_Machinery_Dark", (0.035, 0.045, 0.05, 1), 0.85, 0.30),
    "red": material("MAT_Emergency_Red", (0.70, 0.018, 0.012, 1), 0.30, 0.35),
    "cream": material("MAT_Rope_Cream", (0.56, 0.43, 0.24, 1), 0.0, 0.75),
}


def apply_bevel(obj, width=0.06, segments=2):
    bevel = obj.modifiers.new("EdgeSoftening", "BEVEL")
    bevel.width = width
    bevel.segments = segments
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)


def box(name, location, dimensions, mat, bevel=0.04):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        apply_bevel(obj, min(bevel, min(dimensions) * 0.2))
    obj.data.materials.append(mat)
    return obj


def cylinder(name, location, radius, depth, mat, rotation=(0, 0, 0), vertices=24, bevel=0.035):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    if bevel:
        apply_bevel(obj, bevel)
    obj.data.materials.append(mat)
    return obj


def torus(name, location, major_radius, minor_radius, mat, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=32,
        minor_segments=12,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    return obj


def prism(name, outline, z_bottom, z_top, mat):
    vertices = [(x, y, z_bottom) for x, y in outline] + [(x, y, z_top) for x, y in outline]
    count = len(outline)
    faces = [tuple(range(count - 1, -1, -1)), tuple(range(count, count * 2))]
    for i in range(count):
        n = (i + 1) % count
        faces.append((i, n, n + count, i + count))
    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    apply_bevel(obj, 0.08, 3)
    return obj


def join(parts, name):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    result = parts[0]
    result.name = name
    for polygon in result.data.polygons:
        polygon.use_smooth = False
    return result


def export_asset(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=math.radians(66.0), island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    bpy.ops.export_scene.gltf(
        filepath=str(OUTPUT / f"{obj.name}.glb"),
        export_format="GLB",
        use_selection=True,
        export_apply=True,
        export_yup=True,
    )


assets = []

deck_outline = [(-17.5, -9.5), (13.5, -9.5), (18.0, -4.0), (18.0, 4.0), (13.5, 9.5), (-17.5, 9.5)]
assets.append(prism("SM_MainDeck", deck_outline, -0.16, 0.0, MATS["deck"]))

top = [(-18.0, -10.0), (14.0, -10.0), (19.0, -4.3), (19.0, 4.3), (14.0, 10.0), (-18.0, 10.0)]
bottom = [(-16.8, -5.2), (12.5, -5.2), (17.0, -2.4), (17.0, 2.4), (12.5, 5.2), (-16.8, 5.2)]
verts = [(x, y, -0.25) for x, y in top] + [(x, y, -4.4) for x, y in bottom]
count = len(top)
faces = [tuple(range(count, count * 2))]
for i in range(count):
    n = (i + 1) % count
    faces.append((i, n, n + count, i + count))
mesh = bpy.data.meshes.new("SM_FreighterHull_Mesh")
mesh.from_pydata(verts, [], faces)
mesh.update()
hull = bpy.data.objects.new("SM_FreighterHull", mesh)
bpy.context.collection.objects.link(hull)
hull.data.materials.append(MATS["navy"])
apply_bevel(hull, 0.14, 3)
assets.append(hull)

parts = [
    box("BridgeBody", (0, 0, 1.65), (5.4, 8.8, 3.3), MATS["white"], 0.18),
    box("BridgeRoof", (0, 0, 3.43), (6.0, 9.3, 0.28), MATS["navy"], 0.12),
    box("BridgeFrontGlass", (2.73, 0, 2.28), (0.10, 6.8, 1.12), MATS["glass"], 0.02),
    box("BridgePortGlass", (0.4, -4.43, 2.28), (3.6, 0.10, 1.1), MATS["glass"], 0.02),
    box("BridgeStarboardGlass", (0.4, 4.43, 2.28), (3.6, 0.10, 1.1), MATS["glass"], 0.02),
    box("BridgeDoor", (-2.73, 2.3, 1.15), (0.10, 1.45, 2.3), MATS["dark"], 0.03),
]
assets.append(join(parts, "SM_Bridge"))

parts = [cylinder("RailTop", (0, 0, 1.05), 0.06, 4.0, MATS["orange"], rotation=(0, math.pi / 2, 0), vertices=16)]
for x in (-1.9, 0.0, 1.9):
    parts.append(cylinder("RailPost", (x, 0, 0.52), 0.055, 1.05, MATS["orange"], vertices=16))
parts.append(cylinder("RailMid", (0, 0, 0.55), 0.045, 4.0, MATS["orange"], rotation=(0, math.pi / 2, 0), vertices=16))
assets.append(join(parts, "SM_Railing4m"))

parts = [
    box("BollardBase", (0, 0, 0.10), (1.05, 0.62, 0.20), MATS["dark"], 0.08),
    cylinder("BollardLeft", (-0.28, 0, 0.48), 0.18, 0.76, MATS["orange"], vertices=20),
    cylinder("BollardRight", (0.28, 0, 0.48), 0.18, 0.76, MATS["orange"], vertices=20),
    cylinder("BollardCapL", (-0.28, 0, 0.88), 0.24, 0.10, MATS["dark"], vertices=20),
    cylinder("BollardCapR", (0.28, 0, 0.88), 0.24, 0.10, MATS["dark"], vertices=20),
]
assets.append(join(parts, "SM_Bollard"))

parts = [
    box("WinchBase", (0, 0, 0.18), (2.6, 1.8, 0.36), MATS["dark"], 0.10),
    box("WinchSupportL", (-0.82, 0, 0.80), (0.22, 1.30, 1.25), MATS["orange"], 0.07),
    box("WinchSupportR", (0.82, 0, 0.80), (0.22, 1.30, 1.25), MATS["orange"], 0.07),
    cylinder("WinchDrum", (0, 0, 0.88), 0.62, 1.35, MATS["cream"], rotation=(math.pi / 2, 0, 0), vertices=28),
    cylinder("WinchAxle", (0, 0, 0.88), 0.12, 1.82, MATS["dark"], rotation=(math.pi / 2, 0, 0), vertices=20),
]
assets.append(join(parts, "SM_DeckWinch"))

parts = [
    cylinder("VentStem", (0, 0, 0.75), 0.30, 1.50, MATS["white"], vertices=24),
    torus("VentLip", (0.0, 0.0, 1.52), 0.43, 0.09, MATS["orange"]),
    cylinder("VentCap", (0, 0, 1.58), 0.42, 0.12, MATS["dark"], vertices=24),
]
assets.append(join(parts, "SM_DeckVent"))

parts = [
    torus("LifeRing", (0, 0, 0), 0.58, 0.13, MATS["orange"], rotation=(math.pi / 2, 0, 0)),
    box("LifeRingStripeTop", (0, 0.0, 0.54), (0.24, 0.16, 0.26), MATS["white"], 0.025),
    box("LifeRingStripeBottom", (0, 0.0, -0.54), (0.24, 0.16, 0.26), MATS["white"], 0.025),
    box("LifeRingStripeLeft", (-0.54, 0.0, 0), (0.26, 0.16, 0.24), MATS["white"], 0.025),
    box("LifeRingStripeRight", (0.54, 0.0, 0), (0.26, 0.16, 0.24), MATS["white"], 0.025),
]
assets.append(join(parts, "SM_Lifebuoy"))

parts = [
    box("HatchFrame", (0, 0, 0.10), (2.7, 2.2, 0.20), MATS["orange"], 0.10),
    box("HatchInset", (0, 0, 0.20), (2.35, 1.85, 0.20), MATS["dark"], 0.08),
    box("HatchHandle", (0.65, 0, 0.42), (0.55, 0.15, 0.18), MATS["white"], 0.04),
]
assets.append(join(parts, "SM_DeckHatch"))

parts = [
    box("ContainerBody", (0, 0, 1.30), (5.9, 2.45, 2.60), MATS["red"], 0.08),
    box("ContainerTop", (0, 0, 2.62), (6.05, 2.58, 0.12), MATS["dark"], 0.04),
]
for x in (-2.70, -1.80, -0.90, 0, 0.90, 1.80, 2.70):
    parts.append(box("ContainerRib", (x, -1.235, 1.30), (0.10, 0.08, 2.40), MATS["orange"], 0.015))
assets.append(join(parts, "SM_CargoContainer"))

parts = [
    box("CraneBase", (0, 0, 0.22), (2.3, 2.3, 0.44), MATS["dark"], 0.12),
    cylinder("CraneColumn", (0, 0, 2.25), 0.62, 4.1, MATS["orange"], vertices=28),
    box("CraneBoom", (2.35, 0, 4.2), (5.2, 0.72, 0.72), MATS["orange"], 0.16),
    box("CraneHead", (4.80, 0, 3.65), (0.38, 0.58, 1.15), MATS["dark"], 0.10),
    cylinder("CraneCable", (4.80, 0, 2.40), 0.035, 1.50, MATS["dark"], vertices=12),
    torus("CraneHook", (4.80, 0, 1.58), 0.18, 0.055, MATS["orange"], rotation=(math.pi / 2, 0, 0)),
]
assets.append(join(parts, "SM_DeckCrane"))

parts = [
    cylinder("StackBody", (0, 0, 1.55), 0.68, 3.10, MATS["navy"], vertices=28),
    cylinder("StackTop", (0, 0, 3.12), 0.82, 0.32, MATS["orange"], vertices=28),
    cylinder("StackCap", (0, 0, 3.34), 0.92, 0.18, MATS["dark"], vertices=28),
]
assets.append(join(parts, "SM_ExhaustStack"))

for asset in assets:
    export_asset(asset)

for index, asset in enumerate(assets):
    asset.location = ((index % 4) * 8.0, (index // 4) * 12.0, 0)

bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "CalmyFreighterKit.blend"))
print(f"CALMY_FREIGHTER_KIT_PASS assets={len(assets)} output={OUTPUT}")
