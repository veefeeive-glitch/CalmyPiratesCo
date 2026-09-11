"""Create the standalone 122 m Calmy Ocean Freighter master scene.

This is a deterministic, editable general-arrangement model for first-person game
development. It is dimensionally plausible but is not a class-approved naval
architecture drawing. The Blender master stays authoritative until review.
"""
from pathlib import Path
import json
import math
import os
import runpy
import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = Path(os.environ.get("CALMY_OUTPUT_DIR", str(ROOT / "SourceAssets" / "Generated" / "OceanFreighter" / "R4-Candidate")))
PREVIEW = ROOT / "Saved" / "Verification" / "OceanFreighter-Blender-Overview.png"
OUTPUT.mkdir(parents=True, exist_ok=True)
PREVIEW.parent.mkdir(parents=True, exist_ok=True)

LENGTH = 122.0
BEAM = 20.4
DEPTH = 10.5
DECK_Z = {"tank_top": 2.0, "lower": 4.5, "tween": 7.25, "weather": 10.5}
HULL_CONTROL_STATIONS = [
    (-61.0, 6.4), (-57.0, 7.7), (-50.0, 9.3), (-38.0, 10.0),
    (-20.0, 10.2), (0.0, 10.2), (22.0, 10.15), (42.0, 9.7),
    (52.0, 8.2), (58.0, 4.4), (61.0, 0.45),
]


def subdivide_stations(stations, steps=4):
    result = []
    for (x0, beam0), (x1, beam1) in zip(stations, stations[1:]):
        for step in range(steps):
            t = step / steps
            result.append((x0 + (x1 - x0) * t, beam0 + (beam1 - beam0) * t))
    result.append(stations[-1])
    return result


HULL_STATIONS = subdivide_stations(HULL_CONTROL_STATIONS, 4)

bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
for collection in list(bpy.data.collections):
    if collection.name != "Collection":
        bpy.data.collections.remove(collection)
root_collection = bpy.context.scene.collection
default_collection = bpy.data.collections.get("Collection")
if default_collection:
    root_collection.children.unlink(default_collection)
    bpy.data.collections.remove(default_collection)

bpy.context.scene.unit_settings.system = "METRIC"
bpy.context.scene.unit_settings.length_unit = "METERS"
bpy.context.scene.unit_settings.scale_length = 1.0
bpy.context.scene.render.engine = "BLENDER_EEVEE"
bpy.context.scene.render.resolution_x = 1920
bpy.context.scene.render.resolution_y = 1080
bpy.context.scene.render.resolution_percentage = 100
bpy.context.scene.render.image_settings.file_format = "PNG"
bpy.context.scene.render.film_transparent = False


def collection(name):
    result = bpy.data.collections.new(name)
    root_collection.children.link(result)
    return result


COL = {name: collection(name) for name in (
    "00_REFERENCE", "10_HULL", "20_DECKS_BULKHEADS", "30_SUPERSTRUCTURE",
    "40_MACHINERY_SYSTEMS", "50_CARGO_EQUIPMENT", "60_DECK_FITTINGS",
    "70_NAVIGATION", "80_LIGHTS_CAMERAS", "90_COLLISION", "95_EXPORT_LOD")}


def material(name, color, metallic=0.0, roughness=0.5, emission=None):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.metallic = metallic
    mat.roughness = roughness
    mat.use_nodes = True
    node = mat.node_tree.nodes.get("Principled BSDF")
    node.inputs["Base Color"].default_value = color
    node.inputs["Metallic"].default_value = metallic
    node.inputs["Roughness"].default_value = roughness
    if emission:
        node.inputs["Emission Color"].default_value = emission
        node.inputs["Emission Strength"].default_value = 3.0
    return mat


def ambientcg_material(name, asset_id, tint, metallic_default, roughness_default, scale=3.0, blend=0.55):
    """Shared box-projected 1K PBR material with a deterministic flat fallback."""
    asset_dir = ROOT / "SourceAssets" / "External" / "ambientCG" / asset_id
    prefix = f"{asset_id}_1K-JPG"
    color_path = asset_dir / f"{prefix}_Color.jpg"
    normal_path = asset_dir / f"{prefix}_NormalGL.jpg"
    rough_path = asset_dir / f"{prefix}_Roughness.jpg"
    metal_path = asset_dir / f"{prefix}_Metalness.jpg"
    if not color_path.exists():
        return material(name, tint, metallic_default, roughness_default)

    mat = material(name, tint, metallic_default, roughness_default)
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    texcoord = nodes.new("ShaderNodeTexCoord")
    mapping = nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (scale, scale, scale)
    # Metric object space: long decks must not stretch one tile over 100 metres.
    links.new(texcoord.outputs["Object"], mapping.inputs["Vector"])

    color_tex = nodes.new("ShaderNodeTexImage")
    color_tex.image = bpy.data.images.load(str(color_path), check_existing=True)
    color_tex.projection = "BOX"
    color_tex.projection_blend = 0.18
    links.new(mapping.outputs["Vector"], color_tex.inputs["Vector"])
    tint_mix = nodes.new("ShaderNodeMixRGB")
    tint_mix.blend_type = "MULTIPLY"
    tint_mix.inputs[0].default_value = blend
    tint_mix.inputs[1].default_value = tint
    links.new(color_tex.outputs["Color"], tint_mix.inputs[2])
    links.new(tint_mix.outputs["Color"], bsdf.inputs["Base Color"])

    if rough_path.exists():
        rough_tex = nodes.new("ShaderNodeTexImage")
        rough_tex.image = bpy.data.images.load(str(rough_path), check_existing=True)
        rough_tex.image.colorspace_settings.name = "Non-Color"
        rough_tex.projection = "BOX"
        links.new(mapping.outputs["Vector"], rough_tex.inputs["Vector"])
        links.new(rough_tex.outputs["Color"], bsdf.inputs["Roughness"])
    if metal_path.exists():
        metal_tex = nodes.new("ShaderNodeTexImage")
        metal_tex.image = bpy.data.images.load(str(metal_path), check_existing=True)
        metal_tex.image.colorspace_settings.name = "Non-Color"
        metal_tex.projection = "BOX"
        links.new(mapping.outputs["Vector"], metal_tex.inputs["Vector"])
        links.new(metal_tex.outputs["Color"], bsdf.inputs["Metallic"])
    if normal_path.exists():
        normal_tex = nodes.new("ShaderNodeTexImage")
        normal_tex.image = bpy.data.images.load(str(normal_path), check_existing=True)
        normal_tex.image.colorspace_settings.name = "Non-Color"
        normal_tex.projection = "BOX"
        normal_tex.projection_blend = 0.18
        normal_map = nodes.new("ShaderNodeNormalMap")
        normal_map.inputs["Strength"].default_value = 0.32
        links.new(mapping.outputs["Vector"], normal_tex.inputs["Vector"])
        links.new(normal_tex.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], bsdf.inputs["Normal"])
    return mat


MAT = {
    "hull": material("MAT_Hull_Navy", (0.018, 0.055, 0.075, 1), 0.78, 0.3),
    "boot": material("MAT_BootTop_Black", (0.008, 0.012, 0.014, 1), 0.7, 0.34),
    "antifoul": material("MAT_Antifouling_Red", (0.22, 0.015, 0.012, 1), 0.52, 0.42),
    "deck": ambientcg_material("MAT_Deck_Green_PBR", "MetalWalkway014", (0.12, 0.42, 0.32, 1), 0.55, 0.62, 5.0, 0.52),
    "steel": ambientcg_material("MAT_Steel_PBR", "Metal049A", (0.62, 0.68, 0.70, 1), 0.72, 0.34, 3.0, 0.34),
    "white": material("MAT_Superstructure", (0.72, 0.76, 0.74, 1), 0.24, 0.62),
    "dark": material("MAT_Machinery", (0.018, 0.025, 0.028, 1), 0.82, 0.3),
    "blue": material("MAT_Machinery_Blue", (0.025, 0.13, 0.22, 1), 0.55, 0.38),
    "orange": material("MAT_Safety_Orange", (0.95, 0.19, 0.018, 1), 0.32, 0.4),
    "yellow": material("MAT_Hazard_Yellow", (0.92, 0.62, 0.025, 1), 0.3, 0.45),
    "wood": ambientcg_material("MAT_Interior_Wood_PBR", "WoodFloor051", (0.75, 0.60, 0.42, 1), 0.05, 0.58, 2.5, 0.75),
    "glass": material("MAT_Bridge_Glass", (0.015, 0.16, 0.23, 1), 0.15, 0.08),
    "screen": material("MAT_Screen", (0.005, 0.15, 0.20, 1), 0.15, 0.16, (0.0, 0.65, 0.9, 1)),
    "red": material("MAT_Emergency_Red", (0.66, 0.015, 0.008, 1), 0.35, 0.4),
    "pipe_green": material("MAT_SeaWaterPipe", (0.018, 0.34, 0.16, 1), 0.48, 0.42),
    "pipe_red": material("MAT_FireMainPipe", (0.72, 0.018, 0.012, 1), 0.42, 0.38),
    "brass": material("MAT_Brass", (0.48, 0.25, 0.045, 1), 0.85, 0.25),
    "diamond": ambientcg_material("MAT_DiamondPlate_PBR", "DiamondPlate009", (0.55, 0.60, 0.60, 1), 0.82, 0.38, 4.0, 0.42),
}


def move_to(obj, target):
    for source in list(obj.users_collection):
        source.objects.unlink(obj)
    target.objects.link(obj)
    return obj


def bevel(obj, width=0.04, segments=2):
    mod = obj.modifiers.new("FabricationEdge", "BEVEL")
    mod.width = width
    mod.segments = segments
    return obj


def box(name, loc, dims, mat, col, edge=0.035, rotation=(0, 0, 0)):
    # Data API avoids an O(scene size) dependency-graph update per primitive.
    corners=[(-1,-1,-1),(-1,-1,1),(-1,1,-1),(-1,1,1),(1,-1,-1),(1,-1,1),(1,1,-1),(1,1,1)]
    vertices=[tuple(v[i]*dims[i]*.5 for i in range(3)) for v in corners]
    faces=[(0,2,6,4),(1,5,7,3),(0,4,5,1),(2,3,7,6),(0,1,3,2),(4,6,7,5)]
    obj = mesh_object(name, vertices, faces, mat, col)
    obj.location=loc
    obj.rotation_euler=rotation
    if edge:
        bevel(obj, min(edge, min(dims) * 0.15))
    return obj


def cylinder(name, loc, radius, depth, mat, col, rotation=(0, 0, 0), vertices=20):
    verts=[(radius*math.cos(i*math.tau/vertices),radius*math.sin(i*math.tau/vertices),z)
           for z in (-depth/2,depth/2) for i in range(vertices)]
    faces=[(i,(i+1)%vertices,(i+1)%vertices+vertices,i+vertices) for i in range(vertices)]
    faces += [tuple(reversed(range(vertices))),tuple(range(vertices,vertices*2))]
    obj=mesh_object(name,verts,faces,mat,col)
    obj.location=loc
    obj.rotation_euler=rotation
    for p in obj.data.polygons[:vertices]: p.use_smooth=True
    bevel(obj, min(0.035, radius * 0.12))
    return obj


def sphere(name, loc, radius, mat, col, scale=(1, 1, 1)):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, radius=radius, location=loc)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    obj.data.materials.append(mat)
    return move_to(obj, col)


def torus(name, loc, major, minor, mat, col, rotation=(0, 0, 0)):
    verts=[((major+minor*math.cos(j*math.tau/8))*math.cos(i*math.tau/24),
            (major+minor*math.cos(j*math.tau/8))*math.sin(i*math.tau/24),minor*math.sin(j*math.tau/8))
           for i in range(24) for j in range(8)]
    faces=[(i*8+j,((i+1)%24)*8+j,((i+1)%24)*8+(j+1)%8,i*8+(j+1)%8) for i in range(24) for j in range(8)]
    obj=mesh_object(name,verts,faces,mat,col,smooth=True)
    obj.location=loc
    obj.rotation_euler=rotation
    return obj


def cylinder_between(name, start, end, radius, mat, col, vertices=12):
    start_v = Vector(start)
    end_v = Vector(end)
    direction = end_v - start_v
    obj = cylinder(name, (start_v + end_v) * 0.5, radius, direction.length, mat, col, vertices=vertices)
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    return obj


def half_beam_at(x):
    if x <= HULL_STATIONS[0][0]:
        return HULL_STATIONS[0][1]
    if x >= HULL_STATIONS[-1][0]:
        return HULL_STATIONS[-1][1]
    for (x0, beam0), (x1, beam1) in zip(HULL_STATIONS, HULL_STATIONS[1:]):
        if x0 <= x <= x1:
            alpha = (x - x0) / (x1 - x0)
            return beam0 + (beam1 - beam0) * alpha
    raise RuntimeError(f"No hull beam interpolation at x={x}")


def deck_plate(name, x0, x1, inner_fraction, outer_fraction, z, thickness, mat, col):
    """Create a tapered plate that follows the hull planform between stations."""
    xs = [x0] + [x for x, _ in HULL_STATIONS if x0 < x < x1] + [x1]
    vertices = []
    for x in xs:
        beam = half_beam_at(x)
        for level in (-thickness * 0.5, thickness * 0.5):
            vertices.extend(((x, beam * inner_fraction, z + level), (x, beam * outer_fraction, z + level)))
    faces = []
    for index in range(len(xs) - 1):
        base = index * 4
        nxt = (index + 1) * 4
        faces.extend((
            (base + 2, nxt + 2, nxt + 3, base + 3),
            (base + 0, base + 1, nxt + 1, nxt + 0),
            (base + 0, nxt + 0, nxt + 2, base + 2),
            (base + 1, base + 3, nxt + 3, nxt + 1),
        ))
    faces.extend(((0, 2, 3, 1), (len(vertices) - 4, len(vertices) - 3, len(vertices) - 1, len(vertices) - 2)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    col.objects.link(obj)
    obj.data.materials.append(mat)
    bevel(obj, 0.025, 2)
    return obj


def add_text(name, body, loc, size, col):
    curve = bpy.data.curves.new(name, "FONT")
    curve.body = body
    curve.align_x = "CENTER"
    curve.size = size
    obj = bpy.data.objects.new(name, curve)
    obj.location = loc
    obj.rotation_euler = (math.pi / 2, 0, 0)
    curve.materials.append(MAT["yellow"])
    col.objects.link(obj)
    return obj


def mesh_object(name, vertices, faces, mat, col, edge=0.0, smooth=False):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    col.objects.link(obj)
    obj.data.materials.append(mat)
    if smooth:
        for polygon in mesh.polygons:
            polygon.use_smooth = True
    if edge:
        bevel(obj, edge, 3)
    return obj


def tapered_volume(name, x, y, z0, z1, lower_dims, upper_dims, upper_x_offset, mat, col, edge=0.06):
    """Eight-corner tapered casing used where a plain cube reads as placeholder art."""
    lower_x, lower_y = lower_dims
    upper_x, upper_y = upper_dims
    vertices = [
        (x - lower_x * 0.5, y - lower_y * 0.5, z0),
        (x - lower_x * 0.5, y + lower_y * 0.5, z0),
        (x + lower_x * 0.5, y + lower_y * 0.5, z0),
        (x + lower_x * 0.5, y - lower_y * 0.5, z0),
        (x + upper_x_offset - upper_x * 0.5, y - upper_y * 0.5, z1),
        (x + upper_x_offset - upper_x * 0.5, y + upper_y * 0.5, z1),
        (x + upper_x_offset + upper_x * 0.5, y + upper_y * 0.5, z1),
        (x + upper_x_offset + upper_x * 0.5, y - upper_y * 0.5, z1),
    ]
    faces = ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
             (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7))
    return mesh_object(name, vertices, faces, mat, col, edge, True)


def cone(name, loc, radius1, radius2, depth, mat, col, rotation=(0, 0, 0), vertices=24):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1, radius2=radius2,
                                    depth=depth, location=loc, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.materials.append(mat)
    bevel(obj, min(0.025, min(radius1, radius2) * 0.1))
    return move_to(obj, col)


def tapered_beam_between(name, start, end, root_section, tip_section, mat, col):
    """Tapered solid beam aligned to an arbitrary 3D vector."""
    start_v, end_v = Vector(start), Vector(end)
    axis = (end_v - start_v).normalized()
    side = axis.cross(Vector((0, 0, 1)))
    if side.length < 0.01:
        side = Vector((0, 1, 0))
    side.normalize()
    up = side.cross(axis).normalized()
    vertices = []
    for center, (width, height) in ((start_v, root_section), (end_v, tip_section)):
        vertices.extend((
            center - side * width * 0.5 - up * height * 0.5,
            center + side * width * 0.5 - up * height * 0.5,
            center + side * width * 0.5 + up * height * 0.5,
            center - side * width * 0.5 + up * height * 0.5,
        ))
    faces = ((0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
             (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7))
    return mesh_object(name, vertices, faces, mat, col, 0.055, False)


def curve_tube(name, points, radius, mat, col, cyclic=False):
    curve = bpy.data.curves.new(name, "CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 2
    curve.bevel_depth = radius
    curve.bevel_resolution = 3
    spline = curve.splines.new("POLY")
    spline.points.add(len(points) - 1)
    for point, coordinate in zip(spline.points, points):
        point.co = (*coordinate, 1.0)
    spline.use_cyclic_u = cyclic
    obj = bpy.data.objects.new(name, curve)
    obj.data.materials.append(mat)
    col.objects.link(obj)
    return obj


def gooseneck_vent(name, x, y, deck_z, outlet_x):
    """Compact weather cowl with flange, neck, elbow and dark outlet."""
    col = COL["60_DECK_FITTINGS"]
    torus(f"{name}_BaseFlange", (x, y, deck_z + 0.08), 0.25, 0.045, MAT["steel"], col)
    cylinder(f"{name}_Riser", (x, y, deck_z + 0.68), 0.16, 1.15, MAT["steel"], col, vertices=20)
    sphere(f"{name}_Elbow", (x + outlet_x * 0.18, y, deck_z + 1.24), 0.28,
           MAT["steel"], col, (1.25, 0.72, 0.9))
    cylinder_between(f"{name}_OutletNeck", (x + outlet_x * 0.16, y, deck_z + 1.24),
                     (x + outlet_x * 0.48, y, deck_z + 1.24), 0.17, MAT["steel"], col, 20)
    cone(f"{name}_Bell", (x + outlet_x * 0.58, y, deck_z + 1.24), 0.25, 0.17, 0.28,
         MAT["orange"], col, (0, outlet_x * math.pi / 2, 0), 24)
    cylinder(f"{name}_Opening", (x + outlet_x * 0.73, y, deck_z + 1.24), 0.20, 0.025,
             MAT["dark"], col, (0, math.pi / 2, 0), 24)


def fire_hose_cabinet(name, x, y, deck_z, side):
    """Open-front weather cabinet with coiled hose, nozzle and mounting feet."""
    col = COL["60_DECK_FITTINGS"]
    face_y = y - side * 0.13
    box(f"{name}_Back", (x, y, deck_z + 0.64), (0.72, 0.18, 1.18), MAT["red"], col, 0.055)
    for suffix, dx, dz, dims in (
        ("L", -0.31, 0.64, (0.09, 0.10, 1.08)),
        ("R", 0.31, 0.64, (0.09, 0.10, 1.08)),
        ("T", 0.0, 1.13, (0.55, 0.10, 0.10)),
        ("B", 0.0, 0.16, (0.55, 0.10, 0.10)),
    ):
        box(f"{name}_Frame_{suffix}", (x + dx, face_y, deck_z + dz), dims, MAT["red"], col, 0.025)
    torus(f"{name}_Hose", (x, face_y - side * 0.015, deck_z + 0.66), 0.21, 0.038,
          MAT["orange"], col, (math.pi / 2, 0, 0))
    cylinder_between(f"{name}_Nozzle", (x + 0.14, face_y - side * 0.03, deck_z + 0.48),
                     (x + 0.28, face_y - side * 0.03, deck_z + 0.30), 0.028,
                     MAT["brass"], col, 12)
    for dx in (-0.24, 0.24):
        box(f"{name}_Foot_{dx}", (x + dx, y, deck_z + 0.05), (0.11, 0.26, 0.10), MAT["steel"], col, 0.02)


def mooring_bollard(name, x, y, deck_z):
    col = COL["60_DECK_FITTINGS"]
    box(f"{name}_Bed", (x, y, deck_z + 0.06), (1.25, 0.62, 0.12), MAT["diamond"], col, 0.06)
    for dx in (-0.34, 0.34):
        cylinder(f"{name}_Post_{dx}", (x + dx, y, deck_z + 0.40), 0.15, 0.68,
                 MAT["yellow"], col, vertices=20)
        cone(f"{name}_Cap_{dx}", (x + dx, y, deck_z + 0.76), 0.16, 0.23, 0.14,
             MAT["yellow"], col, vertices=20)
        box(f"{name}_Gusset_{dx}", (x + dx, y, deck_z + 0.16), (0.42, 0.10, 0.22),
            MAT["yellow"], col, 0.025)


def anchor_windlass(name, x, deck_z):
    """Twin-wildcat electric windlass with brakes, motor and chain stoppers."""
    col = COL["60_DECK_FITTINGS"]
    box(f"{name}_Foundation", (x, 0, deck_z + 0.16), (5.4, 3.7, 0.32), MAT["diamond"], col, 0.10)
    cylinder(f"{name}_MainShaft", (x, 0, deck_z + 0.95), 0.16, 3.45,
             MAT["steel"], col, (math.pi / 2, 0, 0), 24)
    for side in (-1, 1):
        y = side * 0.92
        cylinder(f"{name}_Wildcat_{side}", (x, y, deck_z + 0.95), 0.57, 0.62,
                 MAT["steel"], col, (math.pi / 2, 0, 0), 28)
        for flange_y in (y - side * 0.35, y + side * 0.35):
            torus(f"{name}_WildcatFlange_{side}_{flange_y}", (x, flange_y, deck_z + 0.95),
                  0.58, 0.065, MAT["orange"], col, (math.pi / 2, 0, 0))
        brake_y = side * 1.48
        torus(f"{name}_BrakeWheel_{side}", (x - 0.10, brake_y, deck_z + 1.05),
              0.38, 0.045, MAT["orange"], col, (math.pi / 2, 0, 0))
        for angle in range(0, 180, 45):
            a = math.radians(angle)
            start = (x - math.cos(a) * 0.34, brake_y, deck_z + 1.05 - math.sin(a) * 0.34)
            end = (x + math.cos(a) * 0.34, brake_y, deck_z + 1.05 + math.sin(a) * 0.34)
            cylinder_between(f"{name}_BrakeSpoke_{side}_{angle}", start, end, 0.018,
                             MAT["orange"], col, 8)
        box(f"{name}_ChainStopper_{side}", (x + 2.0, side * 1.15, deck_z + 0.34),
            (1.0, 0.52, 0.34), MAT["yellow"], col, 0.07)
    tapered_volume(f"{name}_Gearbox", x - 1.20, 0, deck_z + 0.31, deck_z + 1.42,
                   (1.05, 1.05), (0.78, 0.82), 0.05, MAT["blue"], col, 0.10)
    cylinder(f"{name}_Motor", (x - 2.02, 0, deck_z + 0.72), 0.40, 1.15,
             MAT["blue"], col, (0, math.pi / 2, 0), 24)
    box(f"{name}_Control", (x - 1.55, -1.45, deck_z + 0.78), (0.46, 0.38, 1.02),
        MAT["blue"], col, 0.07)
    box(f"{name}_ControlScreen", (x - 1.55, -1.66, deck_z + 0.95), (0.23, 0.02, 0.24),
        MAT["screen"], col, 0.01)


def cargo_crane(name, x, y, deck_z):
    """Luffing cargo crane with tapered boom, operator cab and hydraulic gear."""
    col = COL["50_CARGO_EQUIPMENT"]
    cylinder(f"{name}_Pedestal", (x, y, deck_z + 1.25), 0.92, 2.5,
             MAT["yellow"], col, vertices=32)
    torus(f"{name}_SlewRing", (x, y, deck_z + 2.52), 0.98, 0.12, MAT["dark"], col)
    tapered_volume(f"{name}_MachineryHouse", x, y, deck_z + 2.55, deck_z + 4.45,
                   (2.45, 2.65), (2.18, 2.28), -0.10, MAT["orange"], col, 0.18)
    box(f"{name}_Cab", (x + 0.62, y - 1.26, deck_z + 3.55), (1.02, 0.62, 1.36),
        MAT["orange"], col, 0.10)
    box(f"{name}_CabGlass", (x + 0.62, y - 1.59, deck_z + 3.70), (0.74, 0.025, 0.70),
        MAT["glass"], col, 0.02)
    pivot = Vector((x + 0.55, y, deck_z + 4.10))
    tip = Vector((x + 12.0, y, deck_z + 10.1))
    tapered_beam_between(f"{name}_Boom", pivot, tip, (0.92, 0.92), (0.38, 0.46),
                         MAT["yellow"], col)
    cylinder_between(f"{name}_HydraulicRam_A", (x - 0.35, y - 0.42, deck_z + 3.8),
                     (x + 4.8, y - 0.42, deck_z + 6.35), 0.14, MAT["steel"], col, 20)
    cylinder_between(f"{name}_HydraulicRam_B", (x - 0.35, y + 0.42, deck_z + 3.8),
                     (x + 4.8, y + 0.42, deck_z + 6.35), 0.14, MAT["steel"], col, 20)
    torus(f"{name}_TipSheave", tip, 0.34, 0.06, MAT["dark"], col, (math.pi / 2, 0, 0))
    cable_end = Vector((tip.x, tip.y, tip.z - 2.0))
    cylinder_between(f"{name}_Cable", tip, cable_end, 0.022, MAT["dark"], col, 10)
    box(f"{name}_HookBlock", cable_end, (0.30, 0.34, 0.42), MAT["red"], col, 0.06)
    hook_points = []
    center = cable_end + Vector((0, 0, -0.43))
    for angle in range(45, 321, 35):
        a = math.radians(angle)
        hook_points.append((center.x + math.cos(a) * 0.24, center.y, center.z + math.sin(a) * 0.30))
    curve_tube(f"{name}_Hook", hook_points, 0.045, MAT["red"], col)


def chair(name, x, y, floor_z, facing_x=1):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"{name}_Seat", (x, y, floor_z + 0.48), (0.46, 0.48, 0.10), MAT["blue"], col, 0.05)
    box(f"{name}_Back", (x - facing_x * 0.21, y, floor_z + 0.82),
        (0.09, 0.48, 0.68), MAT["blue"], col, 0.05)
    for dx in (-0.16, 0.16):
        for dy in (-0.16, 0.16):
            cylinder(f"{name}_Leg_{dx}_{dy}", (x + dx, y + dy, floor_z + 0.23),
                     0.022, 0.46, MAT["steel"], col, vertices=10)


def locker(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"{name}_Body", (x, y, floor_z + 0.92), (0.68, 0.54, 1.84), MAT["steel"], col, 0.055)
    face_y = y - side * 0.28
    for door in (-0.16, 0.16):
        box(f"{name}_Door_{door}", (x + door, face_y, floor_z + 0.96),
            (0.29, 0.025, 1.62), MAT["white"], col, 0.025)
        cylinder(f"{name}_Handle_{door}", (x + door + 0.09, face_y - side * 0.025, floor_z + 1.0),
                 0.018, 0.11, MAT["dark"], col, (math.pi / 2, 0, 0), 10)
        for slot in range(3):
            box(f"{name}_Vent_{door}_{slot}", (x + door, face_y - side * 0.02, floor_z + 1.52 + slot * 0.07),
                (0.15, 0.012, 0.015), MAT["dark"], col, 0)


def crew_bunk(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    frame_y = y
    box(f"{name}_Frame", (x, frame_y, floor_z + 0.35), (2.05, 0.88, 0.16), MAT["steel"], col, 0.045)
    box(f"{name}_Mattress", (x, frame_y, floor_z + 0.48), (1.93, 0.80, 0.18), MAT["white"], col, 0.08)
    box(f"{name}_Blanket", (x + 0.34, frame_y, floor_z + 0.59), (1.12, 0.76, 0.08), MAT["blue"], col, 0.04)
    box(f"{name}_Pillow", (x - 0.72, frame_y, floor_z + 0.62), (0.42, 0.62, 0.13), MAT["white"], col, 0.08)
    for dx in (-0.84, 0.84):
        cylinder(f"{name}_Leg_{dx}", (x + dx, frame_y - side * 0.32, floor_z + 0.18),
                 0.028, 0.36, MAT["steel"], col, vertices=10)


def desk_station(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"{name}_Top", (x, y, floor_z + 0.76), (1.35, 0.62, 0.10), MAT["wood"], col, 0.045)
    for dx in (-0.54, 0.54):
        box(f"{name}_Pedestal_{dx}", (x + dx, y, floor_z + 0.38),
            (0.18, 0.50, 0.72), MAT["steel"], col, 0.035)
    box(f"{name}_Monitor", (x - 0.15, y + side * 0.18, floor_z + 1.12),
        (0.54, 0.08, 0.36), MAT["screen"], col, 0.035)
    cylinder(f"{name}_Lamp", (x + 0.48, y, floor_z + 1.08), 0.035, 0.48,
             MAT["steel"], col, vertices=12)
    before=set(bpy.data.objects)
    chair_y=y-side*.78
    chair(f"{name}_Chair", x, chair_y, floor_z, 1)
    from mathutils import Matrix
    angle=side*math.pi/2
    pivot=Vector((x,chair_y,floor_z))
    for obj in set(bpy.data.objects)-before:
        obj.location=pivot+Matrix.Rotation(angle,3,'Z')@(obj.location-pivot)
        obj.rotation_euler.z+=angle


def corridor_light(name, x, z):
    box(name, (x, 0, z), (1.1, 0.46, 0.055), MAT["screen"], COL["30_SUPERSTRUCTURE"], 0.025)


def area_light(name, location, energy=160.0, size=2.0, color=(0.72, 0.88, 1.0)):
    """Inspection light whose name/location can later map to an Unreal fixture."""
    bpy.ops.object.light_add(type="AREA", location=location)
    obj = bpy.context.object
    obj.name = name
    obj.data.energy = energy
    obj.data.shape = "RECTANGLE"
    obj.data.size = size
    obj.data.size_y = 0.55
    obj.data.color = color
    obj.data.use_shadow = False
    return move_to(obj, COL["80_LIGHTS_CAMERAS"])


def ladder(name, x, y, z0, z1, facing=1):
    """Vertical marine ladder with two stiles, rungs and a small safety cage."""
    col = COL["20_DECKS_BULKHEADS"]
    height = z1 - z0
    for side in (-1, 1):
        cylinder(f"{name}_Stile_{side}", (x, y + side * 0.26, z0 + height * 0.5),
                 0.028, height, MAT["steel"], col, vertices=10)
    rung_count = max(2, int(height / 0.32))
    for rung in range(rung_count + 1):
        z = z0 + height * rung / rung_count
        cylinder_between(f"{name}_Rung_{rung:02d}", (x, y - 0.26, z), (x, y + 0.26, z),
                         0.022, MAT["steel"], col, 10)
    for cage_z in (z0 + height * 0.42, z0 + height * 0.70, z0 + height * 0.94):
        torus(f"{name}_Cage_{cage_z:.2f}", (x + facing * 0.34, y, cage_z), 0.42, 0.025,
              MAT["orange"], col, (math.pi / 2, 0, 0))


def pump_skid(name, x, y, z, pipe_mat):
    col = COL["40_MACHINERY_SYSTEMS"]
    box(f"{name}_Foundation", (x, y, z + 0.10), (2.55, 1.15, 0.20), MAT["diamond"], col, 0.04)
    cylinder(f"{name}_PumpCasing", (x - 0.55, y, z + 0.63), 0.46, 0.48,
             MAT["steel"], col, (math.pi / 2, 0, 0), 28)
    cylinder(f"{name}_Motor", (x + 0.48, y, z + 0.61), 0.38, 1.05,
             MAT["blue"], col, (0, math.pi / 2, 0), 24)
    cylinder_between(f"{name}_Coupling", (x - 0.05, y, z + 0.62), (x + 0.18, y, z + 0.62),
                     0.12, MAT["orange"], col, 16)
    cylinder_between(f"{name}_Suction", (x - 0.55, y - 0.24, z + 0.63),
                     (x - 0.55, y - 1.05, z + 0.63), 0.10, pipe_mat, col, 16)
    cylinder_between(f"{name}_Discharge", (x - 0.55, y, z + 1.05),
                     (x - 0.55, y, z + 1.58), 0.085, pipe_mat, col, 16)
    torus(f"{name}_Valve", (x - 0.55, y, z + 1.56), 0.17, 0.025, MAT["orange"], col)


def heat_exchanger(name, x, y, z):
    col = COL["40_MACHINERY_SYSTEMS"]
    box(f"{name}_Frame", (x, y, z + 0.85), (1.20, 0.92, 1.70), MAT["steel"], col, 0.06)
    for plate in range(8):
        box(f"{name}_Plate_{plate:02d}", (x - 0.42 + plate * 0.12, y, z + 0.86),
            (0.055, 0.78, 1.48), MAT["orange"], col, 0.012)
    for side in (-1, 1):
        cylinder_between(f"{name}_Header_{side}", (x, y + side * 0.54, z + 0.45),
                         (x, y + side * 0.54, z + 1.34), 0.08, MAT["pipe_green"], col, 14)


def air_compressor(name, x, y, z):
    col = COL["40_MACHINERY_SYSTEMS"]
    box(f"{name}_Skid", (x, y, z + 0.10), (2.4, 1.2, 0.20), MAT["diamond"], col, 0.04)
    cylinder(f"{name}_Motor", (x - 0.58, y, z + 0.65), 0.38, 0.95,
             MAT["blue"], col, (0, math.pi / 2, 0), 24)
    for offset in (0.28, 0.78):
        cylinder(f"{name}_Cylinder_{offset}", (x + offset, y, z + 0.78), 0.27, 0.92,
                 MAT["steel"], col, vertices=20)
        box(f"{name}_Head_{offset}", (x + offset, y, z + 1.27), (0.52, 0.52, 0.16),
            MAT["orange"], col, 0.04)


def toilet_module(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"Toilet_{name}_Cistern", (x, y + side * 0.22, floor_z + 0.82),
        (0.50, 0.28, 0.68), MAT["white"], col, 0.10)
    sphere(f"Toilet_{name}_Bowl", (x, y - side * 0.18, floor_z + 0.43), 0.34,
           MAT["white"], col, (0.78, 1.0, 0.62))
    torus(f"Toilet_{name}_Seat", (x, y - side * 0.30, floor_z + 0.55), 0.25, 0.035,
          MAT["dark"], col)


def washbasin(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"Washbasin_{name}_Cabinet", (x, y, floor_z + 0.46), (0.86, 0.50, 0.82), MAT["white"], col, 0.08)
    sphere(f"Washbasin_{name}_Basin", (x, y - side * 0.28, floor_z + 0.90), 0.38,
           MAT["white"], col, (1.0, 0.72, 0.24))
    cylinder_between(f"Washbasin_{name}_Tap", (x, y - side * 0.10, floor_z + 0.92),
                     (x, y - side * 0.27, floor_z + 1.10), 0.026, MAT["steel"], col, 10)


def shower_module(name, x, y, floor_z, side):
    col = COL["30_SUPERSTRUCTURE"]
    box(f"Shower_{name}_Tray", (x, y, floor_z + 0.06), (1.0, 1.0, 0.12), MAT["white"], col, 0.08)
    cylinder_between(f"Shower_{name}_Riser", (x, y + side * 0.40, floor_z + 0.20),
                     (x, y + side * 0.40, floor_z + 1.85), 0.025, MAT["steel"], col, 10)
    sphere(f"Shower_{name}_Head", (x, y + side * 0.28, floor_z + 1.84), 0.10,
           MAT["steel"], col, (1.0, 0.55, 0.35))


def collision_box(name, location, dimensions, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.display_type = "WIRE"
    obj.hide_render = True
    return move_to(obj, COL["90_COLLISION"])


def lod_proxy_box(name, location, dimensions):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.display_type = "WIRE"
    obj.hide_render = True
    obj["export_role"] = "HLOD proxy baseline"
    return move_to(obj, COL["95_EXPORT_LOD"])


def cut_mesh_opening(target_name, opening_name, location, dimensions):
    """Apply a deterministic rectangular stair/ladder opening to a deck mesh."""
    target = bpy.data.objects[target_name]
    bpy.ops.mesh.primitive_cube_add(location=location)
    cutter = bpy.context.object
    cutter.name = opening_name
    cutter.dimensions = dimensions
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = target.modifiers.new(f"{opening_name}_Boolean", "BOOLEAN")
    modifier.operation = "DIFFERENCE"
    modifier.solver = "EXACT"
    modifier.object = cutter
    bpy.context.view_layer.objects.active = target
    target.select_set(True)
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    bpy.data.objects.remove(cutter, do_unlink=True)


def wall_with_doors_x(name, x_center, length, y, z0, z1, door_xs, mat, col):
    """Longitudinal wall with real full-height circulation openings."""
    wall_start = x_center - length * 0.5
    wall_end = x_center + length * 0.5
    door_width = 1.15
    cursor = wall_start
    for index, door_x in enumerate(sorted(door_xs)):
        opening_start = max(wall_start, door_x - door_width * 0.5)
        opening_end = min(wall_end, door_x + door_width * 0.5)
        if opening_start - cursor > 0.08:
            box(f"{name}_Panel_{index}", ((cursor + opening_start) * 0.5, y, (z0 + z1) * 0.5),
                (opening_start - cursor, 0.14, z1 - z0), mat, col, 0.018)
        box(f"{name}_Header_{index}", (door_x, y, z1 - 0.28),
            (door_width, 0.14, 0.56), mat, col, 0.018)
        watertight_door(f"{name}_Frame_{index}", door_x, y, z0, 90)
        cursor = opening_end
    if wall_end - cursor > 0.08:
        box(f"{name}_Panel_End", ((cursor + wall_end) * 0.5, y, (z0 + z1) * 0.5),
            (wall_end - cursor, 0.14, z1 - z0), mat, col, 0.018)


def enclosed_lifeboat(name, location, side):
    """Tapered enclosed SOLAS-style visual lifeboat with cabin and glazing."""
    cx, cy, cz = location
    stations = [(-3.8, 0.12), (-3.25, 0.72), (-2.2, 1.02), (1.9, 1.02), (3.1, 0.7), (3.8, 0.12)]
    ring = 10
    vertices = []
    for x, half_width in stations:
        for index in range(ring):
            angle = 2 * math.pi * index / ring
            y = math.cos(angle) * half_width
            z = math.sin(angle) * (0.62 if math.sin(angle) < 0 else 0.78)
            vertices.append((cx + x, cy + y, cz + z))
    faces = []
    for station in range(len(stations) - 1):
        for index in range(ring):
            nxt = (index + 1) % ring
            a = station * ring + index
            b = station * ring + nxt
            c = (station + 1) * ring + nxt
            d = (station + 1) * ring + index
            faces.append((a, b, c, d))
    faces.extend((tuple(range(ring - 1, -1, -1)), tuple((len(stations) - 1) * ring + i for i in range(ring))))
    mesh_object(name, vertices, faces, MAT["orange"], COL["60_DECK_FITTINGS"], 0.055, True)
    box(f"{name}_Cabin", (cx - 0.15, cy, cz + 0.62), (4.6, 1.5, 0.72), MAT["orange"], COL["60_DECK_FITTINGS"], 0.18)
    for index, x in enumerate((-1.55, -0.5, 0.55, 1.6)):
            box(f"{name}_Window_{index}", (cx + x, cy + side * 0.765, cz + 0.72),
            (0.72, 0.035, 0.34), MAT["glass"], COL["60_DECK_FITTINGS"], 0.025)
    box(f"{name}_Keel", (cx, cy, cz - 0.72), (5.6, 0.16, 0.22), MAT["dark"], COL["60_DECK_FITTINGS"], 0.04)
    cylinder(f"{name}_Propulsor", (cx - 3.55, cy, cz), 0.22, 0.34, MAT["dark"], COL["60_DECK_FITTINGS"], (0, math.pi / 2, 0), 18)


def marine_propeller(name, center, radius=2.2, blades=4):
    """Create ship-like skewed blades with narrowing tips around the X-axis."""
    cx, cy, cz = center
    vertices = []
    faces = []
    radial_steps = 9
    for blade in range(blades):
        base_angle = 2 * math.pi * blade / blades
        blade_start = len(vertices)
        for radial in range(radial_steps):
            t = radial / (radial_steps - 1)
            r = 0.52 + (radius - 0.52) * t
            skew = 0.08 * t + 0.24 * t * t
            half_chord = 0.34 * (1.0 - t) + 0.09
            leading = base_angle + skew - half_chord
            trailing = base_angle + skew + half_chord
            thickness = 0.11 * (1.0 - 0.45 * t)
            for x_offset in (-thickness, thickness):
                for angle in (leading, trailing):
                    vertices.append((cx + x_offset, cy + math.cos(angle) * r, cz + math.sin(angle) * r))
        for radial in range(radial_steps - 1):
            a = blade_start + radial * 4
            b = a + 4
            faces.extend(((a, b, b + 1, a + 1), (a + 2, a + 3, b + 3, b + 2),
                          (a, a + 2, b + 2, b), (a + 1, b + 1, b + 3, a + 3)))
        faces.extend(((blade_start, blade_start + 1, blade_start + 3, blade_start + 2),
                      (blade_start + (radial_steps - 1) * 4, blade_start + (radial_steps - 1) * 4 + 2,
                       blade_start + (radial_steps - 1) * 4 + 3, blade_start + (radial_steps - 1) * 4 + 1)))
    mesh_object(name, vertices, faces, MAT["brass"], COL["40_MACHINERY_SYSTEMS"], 0.045, True)
    cylinder(f"{name}_Hub", center, 0.56, 1.45, MAT["brass"], COL["40_MACHINERY_SYSTEMS"], (0, math.pi / 2, 0), 32)
    bpy.ops.mesh.primitive_cone_add(vertices=32, radius1=0.05, radius2=0.5, depth=1.15,
                                    location=(cx - 0.95, cy, cz), rotation=(0, -math.pi / 2, 0))
    spinner = bpy.context.object
    spinner.name = f"{name}_Spinner"
    spinner.data.materials.append(MAT["brass"])
    move_to(spinner, COL["40_MACHINERY_SYSTEMS"])


def anchor_assembly(name, location, side):
    cx, cy, cz = location
    cylinder_between(f"{name}_Shank", (cx, cy, cz - 1.0), (cx, cy, cz + 1.1), 0.12,
                     MAT["dark"], COL["60_DECK_FITTINGS"], 16)
    cylinder_between(f"{name}_Stock", (cx, cy - side * 0.72, cz + 0.72), (cx, cy + side * 0.72, cz + 0.72),
                     0.1, MAT["dark"], COL["60_DECK_FITTINGS"], 14)
    for offset in (-0.52, 0.52):
        box(f"{name}_Fluke_{offset}", (cx, cy + side * offset, cz - 1.05),
            (0.75, 0.7, 0.18), MAT["dark"], COL["60_DECK_FITTINGS"], 0.08,
            (0, math.radians(28), side * math.radians(18)))


def hull_mesh(name, z_min, z_max, mat):
    section = [
        (-0.46, 0.0), (-0.63, 0.35), (-0.78, 1.0), (-0.90, 1.9),
        (-0.97, 3.2), (-1.0, 6.8), (-0.995, 9.0), (-0.97, DEPTH),
        (0.97, DEPTH), (0.995, 9.0), (1.0, 6.8), (0.97, 3.2),
        (0.90, 1.9), (0.78, 1.0), (0.63, 0.35), (0.46, 0.0),
    ]
    verts = []
    for x, half_beam in HULL_STATIONS:
        for y_factor, z in section:
            z_clamped = min(max(z, z_min), z_max)
            verts.append((x, half_beam * y_factor, z_clamped))
    count = len(section)
    faces = []
    for station in range(len(HULL_STATIONS) - 1):
        for point in range(count):
            if point == 7:  # Open sheer: weather deck/hold covers own the top, not a hidden hull lid.
                continue
            nxt = (point + 1) % count
            a = station * count + point
            b = station * count + nxt
            c = (station + 1) * count + nxt
            d = (station + 1) * count + point
            faces.append((a, b, c, d))
    faces.append(tuple(range(count - 1, -1, -1)))
    last = (len(HULL_STATIONS) - 1) * count
    faces.append(tuple(last + i for i in range(count)))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    COL["10_HULL"].objects.link(obj)
    obj.data.materials.append(mat)
    obj.data.materials.append(MAT["boot"])
    obj.data.materials.append(MAT["antifoul"])
    for polygon in obj.data.polygons:
        mean_z = sum(mesh.vertices[index].co.z for index in polygon.vertices) / len(polygon.vertices)
        polygon.material_index = 2 if mean_z < 7.05 else (1 if mean_z < 7.65 else 0)
        polygon.use_smooth = True
    bevel(obj, 0.08, 3)
    return obj


def watertight_door(name, x, y, z, rotation=0):
    col = COL["20_DECKS_BULKHEADS"]
    frame = []
    for suffix, local_y, local_z, dims in (
        ("L", -0.58, 1.1, (0.16, 0.16, 2.2)),
        ("R", 0.58, 1.1, (0.16, 0.16, 2.2)),
        ("T", 0.0, 2.14, (0.16, 1.32, 0.16)),
    ):
        if rotation == 0:
            frame.append(box(f"{name}_{suffix}", (x, y + local_y, z + local_z), dims, MAT["orange"], col, 0.025))
        else:
            frame.append(box(f"{name}_{suffix}", (x + local_y, y, z + local_z), (dims[1], dims[0], dims[2]), MAT["orange"], col, 0.025))
    # Door is parked open against the adjacent bulkhead so the generated master
    # remains walkable while still showing a real leaf, hinges and wheel handle.
    if rotation == 0:
        leaf = box(f"{name}_DoorLeaf", (x + 0.56, y - 0.61, z + 1.07),
                   (1.02, 0.075, 1.92), MAT["steel"], col, 0.055)
        wheel_loc = (x + 0.56, y - 0.655, z + 1.12)
        wheel_rot = (math.pi / 2, 0, 0)
        for hinge_z in (z + 0.35, z + 1.78):
            cylinder(f"{name}_Hinge_{hinge_z:.2f}", (x + 0.06, y - 0.59, hinge_z),
                     0.035, 0.18, MAT["orange"], col, vertices=12)
    else:
        leaf = box(f"{name}_DoorLeaf", (x - 0.61, y + 0.56, z + 1.07),
                   (0.075, 1.02, 1.92), MAT["steel"], col, 0.055)
        wheel_loc = (x - 0.655, y + 0.56, z + 1.12)
        wheel_rot = (0, math.pi / 2, 0)
        for hinge_z in (z + 0.35, z + 1.78):
            cylinder(f"{name}_Hinge_{hinge_z:.2f}", (x - 0.59, y + 0.06, hinge_z),
                     0.035, 0.18, MAT["orange"], col, vertices=12)
    torus(f"{name}_DoorWheel", wheel_loc, 0.20, 0.027, MAT["orange"], col, wheel_rot)
    leaf["interaction"] = "hinged_door_placeholder"
    return frame


def stairs(name, start, direction, rise, run, steps=14, width=1.25):
    sx, sy, sz = start
    dx, dy = direction
    for i in range(steps):
        t = (i + 0.5) / steps
        box(f"{name}_Step_{i:02d}", (sx + dx * run * t, sy + dy * run * t, sz + rise * t),
            (run / steps if dx else width, width if dx else run / steps, rise / steps),
            MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.018)
    # Construct rails from exact endpoints. The previous Euler-rotated cylinders
    # overshot flights and produced the orange spikes visible in review captures.
    for side in (-1, 1):
        side_x = side * width * 0.48 if not dx else 0
        side_y = side * width * 0.48 if dx else 0
        cylinder_between(f"{name}_Stringer_{side}",
                         (sx + side_x, sy + side_y, sz + 0.03),
                         (sx + dx * run + side_x, sy + dy * run + side_y, sz + rise + 0.03),
                         0.055, MAT["steel"], COL["20_DECKS_BULKHEADS"], 12)
        rail_start = (sx + side_x, sy + side_y, sz + 0.9)
        rail_end = (sx + dx * run + side_x, sy + dy * run + side_y, sz + rise + 0.9)
        cylinder_between(f"{name}_Rail_{side}", rail_start, rail_end, 0.025,
                         MAT["orange"], COL["20_DECKS_BULKHEADS"], 12)
        for post_index, fraction in enumerate((0.0, 0.33, 0.66, 1.0)):
            base = (sx + dx * run * fraction + side_x,
                    sy + dy * run * fraction + side_y,
                    sz + rise * fraction)
            cylinder_between(f"{name}_RailPost_{side}_{post_index}",
                             base, (base[0], base[1], base[2] + 0.9), 0.025,
                             MAT["orange"], COL["20_DECKS_BULKHEADS"], 12)


def deck_with_holds(name, z, thickness=0.16):
    col = COL["20_DECKS_BULKHEADS"]
    # Side walkways and transverse decks follow the faired hull instead of
    # protruding as rectangular slabs at the bow and stern.
    deck_plate(f"{name}_PortWalk", -57, 58, -0.94, -0.70, z, thickness, MAT["deck"], col)
    deck_plate(f"{name}_StarboardWalk", -57, 58, 0.70, 0.94, z, thickness, MAT["deck"], col)
    for suffix, x0, x1 in (("Aft", -57, -20), ("Cross", 10, 14), ("Bow", 42, 58)):
        deck_plate(f"{name}_{suffix}", x0, x1, -0.70, 0.70, z, thickness, MAT["deck"], col)


def bulkhead(name, x, z0, z1, door_y=0.0):
    height = z1 - z0
    width = half_beam_at(x) * 1.88
    opening = 1.35
    left_width = width / 2 + door_y - opening / 2
    right_width = width / 2 - door_y - opening / 2
    box(f"{name}_Port", (x, -width / 2 + left_width / 2, z0 + height / 2), (0.16, left_width, height), MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.02)
    box(f"{name}_Starboard", (x, door_y + opening / 2 + right_width / 2, z0 + height / 2), (0.16, right_width, height), MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.02)
    box(f"{name}_Header", (x, door_y, z1 - 0.32), (0.16, opening, 0.64), MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.02)
    watertight_door(f"{name}_Door", x, door_y, z0, 90)


# Hull shell bands establish waterline and sheer without pretending to be hydrostatic data.
hull_mesh("Hull_Complete", 0.0, DEPTH, MAT["hull"])

for deck_name, z in DECK_Z.items():
    if deck_name in ("lower", "tween", "weather"):
        deck_with_holds(f"Deck_{deck_name.title()}", z)
    else:
        deck_plate(f"Deck_{deck_name.title()}", -57, 58, -0.94, 0.94, z, 0.16, MAT["deck"], COL["20_DECKS_BULKHEADS"])

for x in (-52, -35, -20, 10, 14, 42, 52):
    for z0, z1 in ((2.08, 4.5), (4.58, 7.25), (7.33, 10.5)):
        bulkhead(f"Bulkhead_X{x}_{z0:.0f}", x, z0, z1, door_y=0.0 if x < -20 or x > 42 else 7.8)

# Longitudinal framing, hold ribs and bilge access make the large spaces readable at human scale.
for index, x in enumerate(range(-56, 57, 4)):
    for side in (-1, 1):
        # Frames belong behind the shell. At 0.93 beam they pierced the curved
        # lower hull and looked like a kilometre-long external guard rail.
        y = side * (half_beam_at(x) * 0.76)
        box(f"Frame_{index:02d}_{'P' if side < 0 else 'S'}", (x, y, 5.5),
            (0.18, 0.28, 8.0), MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.018)
for hold, (x0, x1) in enumerate(((-20, 10), (14, 42)), 1):
    for side in (-1, 1):
        box(f"Hold{hold}_CoamingSide_{side}", ((x0 + x1) / 2, side * 7.35, 11.15), (x1 - x0, 0.35, 1.3), MAT["steel"], COL["50_CARGO_EQUIPMENT"], 0.04)
    for x in (x0, x1):
        box(f"Hold{hold}_CoamingEnd_{x}", (x, 0, 11.15), (0.35, 14.35, 1.3), MAT["steel"], COL["50_CARGO_EQUIPMENT"], 0.04)
    for panel in range(6):
        px = x0 + (panel + 0.5) * (x1 - x0) / 6
        box(f"Hold{hold}_Hatch_{panel:02d}", (px, 0, 11.86), ((x1 - x0) / 6 - 0.12, 13.9, 0.18), MAT["hull"], COL["50_CARGO_EQUIPMENT"], 0.035)
        for rib in (-4.5, 0, 4.5):
            box(f"Hold{hold}_HatchRib_{panel:02d}_{rib}", (px, rib, 12.0),
                ((x1 - x0) / 6 - 0.22, 0.09, 0.16), MAT["steel"], COL["50_CARGO_EQUIPMENT"], 0.018)
    # Full-depth hold framing, lashing points and bilge wells establish a useful
    # playable volume instead of an empty shaft below the hatch covers.
    for frame_index, frame_x in enumerate(range(int(x0) + 2, int(x1) - 1, 4)):
        for side in (-1, 1):
            box(f"Hold{hold}_InternalFrame_{frame_index}_{side}",
                (frame_x, side * 7.05, 6.25), (0.20, 0.24, 8.15),
                MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.02)
            for deck_index, deck_z in enumerate((2.28, 4.78, 7.53)):
                torus(f"Hold{hold}_LashingEye_{frame_index}_{side}_{deck_index}",
                      (frame_x, side * 6.90, deck_z), 0.12, 0.025,
                      MAT["orange"], COL["50_CARGO_EQUIPMENT"], (math.pi / 2, 0, 0))
    box(f"Hold{hold}_BilgeWell", ((x0 + x1) * 0.5, 0, 2.10),
        (2.2, 1.5, 0.18), MAT["diamond"], COL["40_MACHINERY_SYSTEMS"], 0.04)
    for light_index, light_x in enumerate((x0 + 5, (x0 + x1) * 0.5, x1 - 5)):
        area_light(f"InteriorLight_Hold{hold}_{light_index}", (light_x, 0, 9.95), 260, 3.0)

# Lower-deck service rooms and stores give the non-cargo spaces a clear purpose.
for rack_index, x in enumerate((44.5, 47.0, 49.5)):
    box(f"BosunStore_Rack_{rack_index}", (x, -5.8, 3.25), (1.8, 0.65, 2.0),
        MAT["steel"], COL["30_SUPERSTRUCTURE"], 0.06)
    for shelf in (2.65, 3.25, 3.85):
        box(f"BosunStore_Rack_{rack_index}_Shelf_{shelf}", (x, -5.8, shelf),
            (1.65, 0.58, 0.07), MAT["diamond"], COL["30_SUPERSTRUCTURE"], 0.02)
for locker_index, y in enumerate((-5.8, -3.8, 3.8, 5.8)):
    locker(f"LowerDeck_ServiceLocker_{locker_index}", 46.0, y, 2.08, -1 if y < 0 else 1)
area_light("InteriorLight_BosunStore", (47.0, -5.0, 4.25), 160, 2.0)

# Interior vertical circulation. Every flight terminates at an authored opening;
# the engine room also has a separated emergency escape.
stairs("Stair_Aft_LowerToTween", (-31.5, -6.8, 4.58), (1, 0), 2.58, 4.4)
stairs("Stair_Aft_TweenToWeather", (-31.5, -6.8, 7.33), (1, 0), 3.08, 4.8)
stairs("Stair_Forward_LowerToTween", (47.0, 5.8, 4.58), (-1, 0), 2.58, 4.4)
stairs("Stair_Forward_TweenToWeather", (47.0, 5.8, 7.33), (-1, 0), 3.08, 4.8)
stairs("Engine_TankTopToLower_Main", (-55.0, -6.2, 2.08), (1, 0), 2.34, 3.8, steps=13, width=1.15)
stairs("EngineEscape_TankTopToLower_Aft", (-35.0, 6.2, 2.08), (-1, 0), 2.34, 3.8, steps=13, width=1.15)

for target, name, location in (
    ("Deck_Lower_PortWalk", "Opening_EngineMain", (-51.2, -6.2, 4.5)),
    ("Deck_Lower_StarboardWalk", "Opening_EngineEscape", (-38.8, 6.2, 4.5)),
    ("Deck_Tween_PortWalk", "Opening_AftTween", (-27.1, -6.8, 7.25)),
    ("Deck_Weather_PortWalk", "Opening_AftWeather", (-26.7, -6.8, 10.5)),
    ("Deck_Tween_StarboardWalk", "Opening_ForwardTween", (42.6, 5.8, 7.25)),
    ("Deck_Weather_StarboardWalk", "Opening_ForwardWeather", (42.2, 5.8, 10.5)),
):
    cut_mesh_opening(target, name, location, (1.65, 1.62, 0.52))
    for side in (-1, 1):
        cylinder_between(f"{name}_Guard_{side}",
                         (location[0] - 0.78, location[1] + side * 0.86, location[2] + 0.04),
                         (location[0] + 0.78, location[1] + side * 0.86, location[2] + 0.04),
                         0.035, MAT["orange"], COL["20_DECKS_BULKHEADS"], 10)

# Dedicated protected access from weather deck down each cargo hold.
for hold_name, x, y in (("HoldAccess_1", -17.4, -6.75), ("HoldAccess_2", 39.4, 6.75)):
    ladder(hold_name, x, y, 2.25, 10.40, -1 if y > 0 else 1)
    box(f"{hold_name}_TrunkA", (x - 0.48, y, 6.25), (0.08, 1.12, 8.25),
        MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.025)
    box(f"{hold_name}_TrunkB", (x + 0.48, y, 6.25), (0.08, 1.12, 8.25),
        MAT["steel"], COL["20_DECKS_BULKHEADS"], 0.025)

# Low-complexity walkable collision modules already use Unreal's naming
# convention. They remain hidden from renders and can be exported separately.
for deck_name, z in DECK_Z.items():
    for section, x, length, width in (
        ("Aft", -39.0, 34.0, 14.0),
        ("Mid", 10.5, 61.0, 2.2 if deck_name in ("lower", "tween", "weather") else 14.0),
        ("Bow", 49.5, 15.0, 10.0),
    ):
        collision_box(f"UCX_Deck_{deck_name}_{section}", (x, 0, z - 0.06), (length, width, 0.12))

for level_name, z, length, width in (
    ("BoatDeck", 10.58, 24.0, 17.0), ("Accommodation", 13.33, 24.0, 16.0),
    ("Officers", 16.08, 21.0, 14.4), ("Bridge", 18.83, 18.0, 13.2),
):
    collision_box(f"UCX_{level_name}_Floor", (-43.0 if level_name != "Bridge" else -40.5, 0, z - 0.05),
                  (length, width, 0.10))

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
    collision_box(f"UCX_StairRamp_{name}", center, (math.hypot(run, rise), width, 0.12),
                  (0, -direction * angle, 0))

collision_box("UCX_HullBoundary_Port", (0, -9.85, 5.4), (108.0, 0.24, 9.8))
collision_box("UCX_HullBoundary_Starboard", (0, 9.85, 5.4), (108.0, 0.24, 9.8))

lod_proxy_box("Hull_Complete_LOD1", (0, 0, 5.2), (118.0, 19.0, 10.2))
lod_proxy_box("Accommodation_HLOD1", (-43.0, 0, 16.2), (24.0, 16.0, 11.2))
lod_proxy_box("CargoZone_HLOD1", (11.0, 0, 8.0), (63.0, 18.0, 8.0))
lod_proxy_box("BowEquipment_HLOD1", (50.0, 0, 11.7), (18.0, 16.0, 3.0))

# Aft accommodation block: four usable tiers and a separate wheelhouse.
accom_levels = [
    ("BoatDeck", 10.58, 13.25, -43.0, 24.0, 17.0),
    ("Accommodation", 13.33, 16.0, -43.0, 24.0, 16.0),
    ("Officers", 16.08, 18.75, -42.0, 21.0, 14.4),
    ("Bridge", 18.83, 22.0, -40.5, 18.0, 13.2),
]
for level_name, z0, z1, x, length, width in accom_levels:
    height = z1 - z0
    room_count = max(3, int(length // 4))
    room_centers = [x - length / 2 + 2.0 + (room + 0.5) * (length - 4.0) / room_count for room in range(room_count)]
    partition_positions = [(a + b) * 0.5 for a, b in zip(room_centers, room_centers[1:])]
    floor_mat = MAT["wood"] if level_name in ("Accommodation", "Officers") else MAT["diamond"]
    box(f"{level_name}_Floor", (x, 0, z0), (length, width, 0.18), floor_mat, COL["30_SUPERSTRUCTURE"], 0.03)
    box(f"{level_name}_Roof", (x, 0, z1), (length, width, 0.16), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.035)
    box(f"{level_name}_AftWall", (x - length / 2, 0, (z0 + z1) / 2), (0.18, width, height), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.03)
    box(f"{level_name}_FrontWall", (x + length / 2, 0, (z0 + z1) / 2), (0.18, width, height), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.03)
    for side in (-1, 1):
        wall_with_doors_x(f"{level_name}_SideWall_{side}", x, length, side * width / 2, z0, z1,
                          [x + length / 2 - 2.0], MAT["white"], COL["30_SUPERSTRUCTURE"])
    # Central corridor walls have genuine openings into every room.
    for side in (-1, 1):
        wall_with_doors_x(f"{level_name}_Corridor_{side}", x, length - 1.0, side * 1.05, z0, z1 - 0.08,
                          room_centers, MAT["white"], COL["30_SUPERSTRUCTURE"])
    for room, rx in enumerate(partition_positions):
        for side in (-1, 1):
            box(f"{level_name}_Partition_{room}_{side}", (rx, side * (width / 4 + 0.55), z0 + height / 2),
                (0.12, width / 2 - 1.1, height - 0.15), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.018)
    # Windows retain the outer wall as a blockout but give correct bridge/accommodation rhythm.
    for window in range(room_count + 1):
        wx = x - length / 2 + 1.6 + window * ((length - 3.2) / room_count)
        for side in (-1, 1):
            box(f"{level_name}_Window_{window}_{side}", (wx, side * (width / 2 + 0.095), z0 + height * 0.63),
                (1.15, 0.035, 0.68), MAT["glass"], COL["30_SUPERSTRUCTURE"], 0.015)
    for light_index, light_x in enumerate(room_centers):
        corridor_light(f"{level_name}_CorridorLight_{light_index}", light_x, z1 - 0.13)

# Twin supported switchback access towers with alternating flights and landings.
access_levels = (10.66, 13.41, 16.16, 18.91, 22.08)
access_widths = (8.5, 8.0, 7.2, 6.6)
for side in (-1, 1):
    for tier, (z0, z1) in enumerate(zip(access_levels, access_levels[1:])):
        y = side * (access_widths[tier] + 0.72)
        start_x = -33.4 if tier % 2 == 0 else -38.0
        direction_x = -1 if tier % 2 == 0 else 1
        box(f"AccessTower_{side}_LandingLow_{tier}", (start_x, y, z0), (1.7, 1.55, 0.16), MAT["deck"], COL["20_DECKS_BULKHEADS"], 0.03)
        stairs(f"AccessTower_{side}_Flight_{tier}", (start_x, y, z0 + 0.08), (direction_x, 0), z1 - z0 - 0.16, 4.25, steps=16, width=1.2)
        box(f"AccessTower_{side}_LandingHigh_{tier}", (start_x + direction_x * 4.25, y, z1),
            (1.7, 1.55, 0.16), MAT["deck"], COL["20_DECKS_BULKHEADS"], 0.03)
        # Short transverse bridge ties each landing into the stepped façade.
        wall_y = side * access_widths[tier]
        cylinder_between(f"AccessTower_{side}_LandingTie_{tier}",
                         (start_x, wall_y, z0 + 0.06), (start_x, y, z0 + 0.06),
                         0.065, MAT["steel"], COL["20_DECKS_BULKHEADS"], 14)
for side in (-1, 1):
    wing_y = side * 8.2
    box(f"BridgeWing_{side}", (-31.0, wing_y, 18.92), (4.8, 3.4, 0.18), MAT["deck"], COL["70_NAVIGATION"], 0.035)
    box(f"BridgeWingConsole_{side}", (-29.6, side * 8.45, 19.55), (0.85, 1.55, 1.05), MAT["dark"], COL["70_NAVIGATION"], 0.08)
    box(f"BridgeWingScreen_{side}", (-29.15, side * 8.45, 19.92), (0.025, 0.75, 0.42), MAT["screen"], COL["70_NAVIGATION"], 0.01)

# Bridge front windows and navigation consoles.
for index, y in enumerate((-5.1, -3.4, -1.7, 0, 1.7, 3.4, 5.1)):
    box(f"Bridge_FrontWindow_{index}", (-31.41, y, 20.72), (0.035, 1.35, 0.92), MAT["glass"], COL["70_NAVIGATION"], 0.012)
box("Bridge_MainConsole", (-33.0, 0, 19.55), (1.0, 7.8, 1.0), MAT["dark"], COL["70_NAVIGATION"], 0.08)
for y in (-2.4, 0, 2.4):
    box(f"Bridge_Screen_{y}", (-32.47, y, 19.92), (0.035, 1.1, 0.5), MAT["screen"], COL["70_NAVIGATION"], 0.015)
cylinder("Bridge_Wheel", (-32.38, 0, 20.25), 0.48, 0.10, MAT["wood"], COL["70_NAVIGATION"], (0, math.pi / 2, 0), 24)

# Main propulsion plant, generators, shaft line, switchboards and service systems.
box("MainEngine_Bed", (-45.0, 0, 2.35), (12.0, 4.8, 0.55), MAT["dark"], COL["40_MACHINERY_SYSTEMS"], 0.12)
box("MainEngine_Block", (-44.0, 0, 4.15), (9.2, 4.0, 3.1), MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.22)
for cyl_index, x in enumerate((-47.0, -45.5, -44.0, -42.5, -41.0)):
    cylinder(f"MainEngine_Cylinder_{cyl_index}", (x, 0, 6.2), 0.48, 1.55, MAT["steel"], COL["40_MACHINERY_SYSTEMS"], vertices=24)
    cylinder(f"MainEngine_Exhaust_{cyl_index}", (x, 0.65, 7.8), 0.16, 2.4, MAT["dark"], COL["40_MACHINERY_SYSTEMS"], vertices=16)
cylinder("Propeller_Shaft", (-54.5, 0, 2.55), 0.26, 15.0, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], (0, math.pi / 2, 0), 20)
for index, x in enumerate((-56, -52, -48)):
    box(f"Shaft_Bearing_{index}", (x, 0, 2.38), (0.8, 1.2, 0.75), MAT["steel"], COL["40_MACHINERY_SYSTEMS"], 0.08)
for index, y in enumerate((-6.0, 6.0)):
    box(f"Generator_{index}_Bed", (-46, y, 2.25), (4.2, 2.2, 0.35), MAT["dark"], COL["40_MACHINERY_SYSTEMS"], 0.08)
    cylinder(f"Generator_{index}", (-46, y, 3.2), 0.72, 3.5, MAT["blue"], COL["40_MACHINERY_SYSTEMS"], (0, math.pi / 2, 0), 24)
for index, y in enumerate((-6.5, -4.8, 4.8, 6.5)):
    box(f"Switchboard_{index}", (-36.2, y, 3.3), (0.65, 1.35, 2.25), MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.07)
    box(f"Switchboard_Screen_{index}", (-35.86, y, 3.75), (0.025, 0.55, 0.38), MAT["screen"], COL["40_MACHINERY_SYSTEMS"], 0.01)
for index, y in enumerate((-7.1, -5.2, 5.2, 7.1)):
    cylinder(f"ServiceTank_{index}", (-28.0, y, 3.35), 0.86, 2.5, MAT["steel"], COL["40_MACHINERY_SYSTEMS"], vertices=24)
    torus(f"ServiceTank_Band_{index}", (-28.0, y, 3.35), 0.88, 0.045, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], (math.pi / 2, 0, 0))

# Engine-room catwalks and colour-coded service/fire mains.
for side in (-1, 1):
    box(f"EngineCatwalk_{side}", (-44.0, side * 3.15, 5.25), (12.0, 1.05, 0.16), MAT["steel"], COL["40_MACHINERY_SYSTEMS"], 0.03)
    for x in range(-49, -38, 2):
        cylinder(f"EngineCatwalkPost_{side}_{x}", (x, side * 3.62, 5.82), 0.028, 1.1, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], vertices=10)
    cylinder_between(f"EngineCatwalkRail_{side}", (-49, side * 3.62, 6.25), (-39, side * 3.62, 6.25),
                     0.026, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], 10)
for pipe_index, (y, z, mat_key) in enumerate(((-7.4, 6.65, "pipe_green"), (-6.9, 7.0, "pipe_red"),
                                              (6.9, 6.65, "pipe_green"), (7.4, 7.0, "pipe_red"))):
    cylinder_between(f"EngineRoomMain_{pipe_index}", (-55, y, z), (-22, y, z), 0.11, MAT[mat_key], COL["40_MACHINERY_SYSTEMS"], 16)
    for branch, x in enumerate((-50, -44, -38, -30)):
        cylinder_between(f"EngineRoomBranch_{pipe_index}_{branch}", (x, y, z), (x, y * 0.62, z - 1.7),
                         0.075, MAT[mat_key], COL["40_MACHINERY_SYSTEMS"], 14)
        torus(f"EngineRoomValve_{pipe_index}_{branch}", (x, y * 0.62, z - 1.55), 0.18, 0.035,
              MAT["brass"], COL["40_MACHINERY_SYSTEMS"], (math.pi / 2, 0, 0))

# Production-readable engine auxiliaries replace the empty perimeter around the
# propulsion block. Each skid preserves a central maintenance route.
for index, (x, y, mat_key) in enumerate(((-51.0, -5.25, "pipe_green"), (-51.0, 5.25, "pipe_red"),
                                         (-40.0, -5.25, "pipe_green"), (-40.0, 5.25, "pipe_red"))):
    pump_skid(f"BilgeFirePump_{index}", x, y, 2.18, MAT[mat_key])
for index, y in enumerate((-5.45, 5.45)):
    heat_exchanger(f"JacketWaterHeatExchanger_{index}", -43.0, y, 4.62)
    air_compressor(f"StartingAirCompressor_{index}", -37.8, y, 2.18)
    cylinder(f"StartingAirBottle_{index}", (-34.8, y, 3.35), 0.68, 2.35,
             MAT["steel"], COL["40_MACHINERY_SYSTEMS"], vertices=28)
    torus(f"StartingAirBottleBand_{index}", (-34.8, y, 3.35), 0.70, 0.045,
          MAT["orange"], COL["40_MACHINERY_SYSTEMS"])

for index, y in enumerate((-4.4, 4.4)):
    cylinder(f"FuelPurifier_{index}_Bowl", (-29.5, y, 3.05), 0.46, 1.40,
             MAT["steel"], COL["40_MACHINERY_SYSTEMS"], vertices=28)
    cone(f"FuelPurifier_{index}_Hood", (-29.5, y, 3.82), 0.52, 0.25, 0.28,
         MAT["orange"], COL["40_MACHINERY_SYSTEMS"], vertices=28)
    box(f"FuelPurifier_{index}_Skid", (-29.5, y, 2.28), (1.35, 1.12, 0.20),
        MAT["diamond"], COL["40_MACHINERY_SYSTEMS"], 0.04)

# Cylinder-head covers, common exhaust manifold and turbocharger make the main
# engine legible as propulsion machinery rather than a stack of primitives.
for index, x in enumerate((-47.0, -45.5, -44.0, -42.5, -41.0)):
    box(f"MainEngine_HeadCover_{index}", (x, 0, 7.05), (1.12, 2.15, 0.52),
        MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.14)
    cylinder_between(f"MainEngine_FuelRail_{index}", (x, -1.22, 6.25), (x, -1.22, 7.10),
                     0.045, MAT["yellow"], COL["40_MACHINERY_SYSTEMS"], 12)
cylinder_between("MainEngine_ExhaustManifold", (-48.0, 1.18, 8.05), (-40.0, 1.18, 8.05),
                 0.28, MAT["dark"], COL["40_MACHINERY_SYSTEMS"], 24)
cylinder("MainEngine_Turbocharger", (-39.3, 1.18, 7.35), 0.62, 0.72,
         MAT["steel"], COL["40_MACHINERY_SYSTEMS"], (math.pi / 2, 0, 0), 32)
torus("MainEngine_TurboIntake", (-39.3, 0.80, 7.35), 0.44, 0.09,
      MAT["orange"], COL["40_MACHINERY_SYSTEMS"], (math.pi / 2, 0, 0))

# Local control station and labelled manifolds along the engine-room boundary.
box("EngineControlConsole", (-33.0, -6.95, 3.05), (3.4, 0.72, 1.72),
    MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.10)
for screen_index, x in enumerate((-34.0, -33.0, -32.0)):
    box(f"EngineControlScreen_{screen_index}", (x, -7.33, 3.35), (0.62, 0.025, 0.42),
        MAT["screen"], COL["40_MACHINERY_SYSTEMS"], 0.015)
for manifold_index, (x, mat_key) in enumerate(((-55.0, "pipe_green"), (-53.5, "pipe_red"))):
    cylinder_between(f"EngineManifold_{manifold_index}", (x, -7.2, 2.65), (x, -7.2, 4.35),
                     0.11, MAT[mat_key], COL["40_MACHINERY_SYSTEMS"], 18)
    for branch in range(4):
        z = 2.85 + branch * 0.42
        cylinder_between(f"EngineManifold_{manifold_index}_Branch_{branch}",
                         (x, -7.2, z), (x + 0.72, -7.2, z), 0.065,
                         MAT[mat_key], COL["40_MACHINERY_SYSTEMS"], 14)
        torus(f"EngineManifold_{manifold_index}_Valve_{branch}", (x + 0.72, -7.2, z),
              0.14, 0.025, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], (math.pi / 2, 0, 0))

# Twin-ram steering gear and hydraulic power packs around the rudder stock.
cylinder_between("SteeringGear_PortRam", (-58.0, -2.0, 3.35), (-55.8, -0.45, 3.35),
                 0.24, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], 24)
cylinder_between("SteeringGear_StarboardRam", (-58.0, 2.0, 3.35), (-55.8, 0.45, 3.35),
                 0.24, MAT["orange"], COL["40_MACHINERY_SYSTEMS"], 24)
cylinder("SteeringGear_RudderStock", (-56.0, 0, 3.75), 0.30, 3.10,
         MAT["steel"], COL["40_MACHINERY_SYSTEMS"], vertices=28)
box("SteeringGear_Tiller", (-56.0, 0, 3.45), (1.0, 4.4, 0.34),
    MAT["steel"], COL["40_MACHINERY_SYSTEMS"], 0.10)
for index, y in enumerate((-3.4, 3.4)):
    pump_skid(f"SteeringGear_HydraulicPump_{index}", -54.4, y, 2.18, MAT["pipe_red"])

# Real inspection lights for all game-critical vertical zones.
for index, (x, y, z) in enumerate(((-53, 0, 9.7), (-47, 0, 9.7), (-41, 0, 9.7), (-35, 0, 9.7),
                                    (-50, -5.7, 4.25), (-44, 5.7, 4.25), (-33, -5.7, 4.25))):
    area_light(f"InteriorLight_Engine_{index}", (x, y, z), 220, 2.4)

# Interior hotel-service modules: made at player-readable scale instead of
# placeholder boxes, with bedding, storage, desks, chairs and service details.
for cabin in range(12):
    x = -52.0 + (cabin % 6) * 3.6
    side = -1 if cabin < 6 else 1
    y = side * 5.0
    z = 13.42
    crew_bunk(f"Cabin_{cabin}_Bunk", x, y, z, side)
    locker(f"Cabin_{cabin}_Locker", x + 0.78, y + side * 0.82, z, side)
    desk_station(f"Cabin_{cabin}_Desk", x - 0.40, y - side * 1.15, z, side)
for table in range(3):
    x = -49 + table * 3.6
    box(f"Mess_Table_{table}", (x, -4.5, 11.42), (2.2, 1.1, 0.14), MAT["wood"], COL["30_SUPERSTRUCTURE"], 0.06)
    for leg_x in (-0.82, 0.82):
        for leg_y in (-0.36, 0.36):
            cylinder(f"Mess_Table_{table}_Leg_{leg_x}_{leg_y}", (x + leg_x, -4.5 + leg_y, 11.0),
                     0.035, 0.76, MAT["steel"], COL["30_SUPERSTRUCTURE"], vertices=10)
    for side in (-1, 1):
        box(f"Mess_Bench_{table}_{side}", (x, -4.5 + side * 0.92, 11.08),
            (2.0, 0.42, 0.16), MAT["blue"], COL["30_SUPERSTRUCTURE"], 0.05)
        box(f"Mess_BenchBack_{table}_{side}", (x, -4.5 + side * 1.08, 11.42),
            (2.0, 0.12, 0.62), MAT["blue"], COL["30_SUPERSTRUCTURE"], 0.05)

box("Galley_RangeBody", (-48, 5.9, 11.22), (3.2, 0.9, 1.12), MAT["steel"], COL["30_SUPERSTRUCTURE"], 0.08)
for burner in (-1.05, -0.35, 0.35, 1.05):
    torus(f"Galley_Burner_{burner}", (-48 + burner, 5.9, 11.80), 0.20, 0.035,
          MAT["dark"], COL["30_SUPERSTRUCTURE"])
box("Galley_ExtractionHood", (-48, 6.15, 12.52), (3.6, 1.0, 0.42), MAT["steel"], COL["30_SUPERSTRUCTURE"], 0.08)
box("Galley_Sink", (-52.0, 5.9, 11.20), (2.0, 0.9, 1.08), MAT["steel"], COL["30_SUPERSTRUCTURE"], 0.07)
box("Galley_SinkBasin", (-52.0, 5.65, 11.78), (1.35, 0.50, 0.10), MAT["dark"], COL["30_SUPERSTRUCTURE"], 0.025)
box("Galley_Fridge", (-44.8, 6.0, 11.58), (1.15, 0.86, 1.85), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.08)

crew_bunk("Medical_Bed", -34, 5.2, 13.42, 1)
box("Medical_ExamLamp", (-33.2, 4.5, 14.75), (0.36, 0.20, 0.16), MAT["screen"], COL["30_SUPERSTRUCTURE"], 0.05)
locker("Medical_SupplyLocker", -36.0, 5.7, 13.42, 1)
for officer in range(8):
    x = -49.5 + (officer % 4) * 4.2
    side = -1 if officer < 4 else 1
    y = side * 4.7
    crew_bunk(f"Officer_{officer}_Bed", x, y, 16.17, side)
    desk_station(f"Officer_{officer}_Desk", x + 0.45, y - side * 1.0, 16.17, side)
    locker(f"Officer_{officer}_Locker", x - 0.78, y + side * 0.82, 16.17, side)
for machine in range(4):
    box(f"LaundryMachine_{machine}", (-53.0 + machine * 1.15, 5.4, 11.35),
        (0.9, 0.82, 1.35), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.08)
    cylinder(f"LaundryDoor_{machine}", (-53.0 + machine * 1.15, 4.97, 11.45),
             0.27, 0.04, MAT["glass"], COL["30_SUPERSTRUCTURE"], (math.pi / 2, 0, 0), 20)

# Two complete sanitary rooms on boat deck. They intentionally sit outside the
# galley/laundry footprint and use human-scale fixtures visible in first person.
for sanitary_index, side in enumerate((-1, 1)):
    base_y = side * 5.35
    toilet_module(f"Crew_{sanitary_index}", -36.8, base_y, 10.67, side)
    washbasin(f"Crew_{sanitary_index}", -35.5, base_y, 10.67, side)
    shower_module(f"Crew_{sanitary_index}", -34.0, base_y, 10.67, side)
    box(f"Sanitary_{sanitary_index}_PrivacyPanel", (-35.4, base_y + side * 0.58, 11.65),
        (4.4, 0.08, 1.95), MAT["white"], COL["30_SUPERSTRUCTURE"], 0.03)

# Accommodation and bridge fixtures have real area lights in addition to the
# emissive lenses, keeping first-person inspection readable.
for index, (x, y, z, energy) in enumerate(((-50, 0, 15.78, 115), (-43, 0, 15.78, 115), (-36, 0, 15.78, 115),
                                           (-48, 0, 18.53, 125), (-41, 0, 18.53, 125), (-45, 0, 21.75, 150),
                                           (-38, 0, 21.75, 150), (-43, -4.8, 13.02, 120), (-43, 4.8, 13.02, 120))):
    area_light(f"InteriorLight_Accommodation_{index}", (x, y, z), energy, 1.8)
for bench, y in enumerate((-5.1, 5.1)):
    box(f"EngineWorkshopBench_{bench}", (-32.0, y, 4.9), (4.5, 1.0, 0.85),
        MAT["steel"], COL["40_MACHINERY_SYSTEMS"], 0.08)
    box(f"EngineWorkshopToolboard_{bench}", (-32.0, y + (-0.55 if y < 0 else 0.55), 5.9),
        (4.2, 0.08, 1.2), MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.04)

# Deck cranes sized for the two holds.
for crane_index, x in enumerate((-5.0, 31.0)):
    cargo_crane(f"CargoCrane_{crane_index}", x, 8.25, 10.58)

# Mooring stations, railings and safety equipment.
for index, x in enumerate((-56, -51, 47, 54)):
    for side in (-1, 1):
        fitting_y = side * min(7.7, half_beam_at(x) * 0.72)
        mooring_bollard(f"Bollard_{index}_{side}", x, fitting_y, 10.58)
for side in (-1, 1):
    rail_positions = list(range(-56, 57, 3))
    rail_points = [(x, side * (half_beam_at(x) * 0.91), 11.05) for x in rail_positions]
    for x, y, z in rail_points:
        cylinder(f"RailPost_{side}_{x}", (x, y, z), 0.035, 1.15, MAT["steel"], COL["60_DECK_FITTINGS"], vertices=10)
    for z in (11.25, 11.65):
        for segment, (a, b) in enumerate(zip(rail_points, rail_points[1:])):
            cylinder_between(f"RailRun_{side}_{z}_{segment}", (a[0], a[1], z), (b[0], b[1], z),
                             0.025, MAT["steel"], COL["60_DECK_FITTINGS"], 10)
for side in (-1, 1):
    enclosed_lifeboat(f"Lifeboat_{'Port' if side < 0 else 'Starboard'}", (-47, side * 9.15, 15.45), side)
    for x in (-49.7, -44.3):
        cylinder_between(f"Lifeboat_Davit_{side}_{x}", (x, side * 7.95, 15.0),
                         (x, side * 9.2, 18.0), 0.09, MAT["steel"], COL["60_DECK_FITTINGS"], 16)
        cylinder_between(f"Lifeboat_Fall_{side}_{x}", (x, side * 9.2, 17.95),
                         (x, side * 9.2, 16.0), 0.025, MAT["dark"], COL["60_DECK_FITTINGS"], 10)
for side in (-1, 1):
    anchor_assembly(f"Anchor_{'Port' if side < 0 else 'Starboard'}", (53.5, side * 8.15, 7.0), side)

# Deck plating, ventilators and fire stations provide repeatable scale detail.
for seam_index, x in enumerate(range(-54, 56, 3)):
    beam = half_beam_at(x) * 0.9
    box(f"DeckSeam_{seam_index:02d}", (x, 0, 10.583), (0.018, beam * 2, 0.006), MAT["dark"], COL["60_DECK_FITTINGS"], 0)
for vent_index, (x, y) in enumerate(((-55, -5.8), (-55, 5.8), (-32, -7.5), (-32, 7.5),
                                     (8, -8.1), (8, 8.1), (43, -6.7), (43, 6.7))):
    gooseneck_vent(f"Vent_{vent_index}", x, y, 10.58, 1 if x < 0 else -1)
for station_index, (x, side) in enumerate(((-28, -1), (-28, 1), (12, -1), (12, 1), (45, -1), (45, 1))):
    y = side * half_beam_at(x) * 0.82
    fire_hose_cabinet(f"FireStation_{station_index}", x, y, 10.58, side)

# Working mooring stations at both ends.
for station_name, x in (("Aft", -53.0), ("Bow", 49.0)):
    anchor_windlass(f"{station_name}_Windlass", x, 10.58)
    if station_name == "Bow":
        for side in (-1, 1):
            # Deck chain is represented by short supported links rather than a
            # single black diagonal pole floating across the forecastle.
            chain_start = Vector((x + 0.25, side * 0.92, 11.68))
            chain_end = Vector((53.0, side * 5.25, 10.86))
            for link in range(12):
                a = chain_start.lerp(chain_end, link / 12)
                b = chain_start.lerp(chain_end, (link + 0.82) / 12)
                lift = 0.025 if link % 2 else 0.0
                a.z += lift
                b.z += lift
                cylinder_between(f"Bow_AnchorChain_{side}_Link_{link:02d}", a, b, 0.06,
                                 MAT["dark"], COL["60_DECK_FITTINGS"], 10)
for fairlead_index, (x, side) in enumerate(((-56, -1), (-56, 1), (-49, -1), (-49, 1), (48, -1), (48, 1), (55, -1), (55, 1))):
    y = side * half_beam_at(x) * 0.78
    box(f"Fairlead_{fairlead_index}_Base", (x, y, 10.82), (0.72, 0.42, 0.22), MAT["yellow"], COL["60_DECK_FITTINGS"], 0.06)
    for x_offset in (-0.25, 0.25):
        cylinder(f"Fairlead_{fairlead_index}_Roller_{x_offset}", (x + x_offset, y, 11.2), 0.09, 0.62,
                 MAT["steel"], COL["60_DECK_FITTINGS"], vertices=14)

# Funnel, radar mast and navigation equipment. A raked, tapered casing replaces
# the oversized black cuboid that intersected several accommodation decks.
box("Funnel_VentCasing", (-50.2, 0, 18.05), (5.8, 4.6, 1.25), MAT["white"], COL["70_NAVIGATION"], 0.18)
tapered_volume("Funnel", -50.55, 0, 18.55, 23.55, (4.1, 3.75), (2.9, 2.75),
               -0.55, MAT["hull"], COL["70_NAVIGATION"], 0.22)
box("Funnel_Band", (-50.93, 0, 21.15), (3.55, 3.25, 0.48), MAT["orange"], COL["70_NAVIGATION"], 0.12)
for y in (-0.82, 0.82):
    cylinder(f"FunnelPipe_{y}", (-51.1, y, 24.15), 0.32, 1.55, MAT["dark"], COL["70_NAVIGATION"], vertices=24)
cone("RadarMast", (-38.5, 0, 25.7), 0.24, 0.11, 7.2, MAT["steel"], COL["70_NAVIGATION"], vertices=24)
for platform_index, (z, size) in enumerate(((24.25, 2.4), (26.55, 1.75))):
    box(f"MastPlatform_{platform_index}", (-38.5, 0, z), (1.15, size, 0.11),
        MAT["diamond"], COL["70_NAVIGATION"], 0.04)
    for side in (-1, 1):
        cylinder_between(f"MastPlatformRail_{platform_index}_{side}",
                         (-39.0, side * size * 0.48, z + 0.55),
                         (-38.0, side * size * 0.48, z + 0.55), 0.025,
                         MAT["steel"], COL["70_NAVIGATION"], 10)
for yard_index, (z, length) in enumerate(((24.75, 6.4), (27.15, 3.8))):
    cylinder(f"MastYard_{yard_index}", (-38.5, 0, z), 0.065, length,
             MAT["steel"], COL["70_NAVIGATION"], (math.pi / 2, 0, 0), 16)
    for side in (-1, 1):
        cylinder_between(f"MastYardBrace_{yard_index}_{side}",
                         (-38.5, 0, z - 0.62), (-38.5, side * length * 0.43, z),
                         0.035, MAT["steel"], COL["70_NAVIGATION"], 12)
box("RadarScanner", (-38.5, 0, 28.75), (0.32, 4.2, 0.24), MAT["white"], COL["70_NAVIGATION"], 0.08)
cylinder("RadarDrive", (-38.5, 0, 28.45), 0.22, 0.45, MAT["dark"], COL["70_NAVIGATION"], vertices=20)
sphere("GPS_Radome", (-38.5, -1.1, 27.45), 0.24, MAT["white"], COL["70_NAVIGATION"], (1, 1, 0.82))
for aerial_index, y in enumerate((-0.65, 0.65)):
    cylinder(f"VHFAntenna_{aerial_index}", (-38.5, y, 29.75), 0.025, 1.9,
             MAT["white"], COL["70_NAVIGATION"], vertices=10)
for side, color in ((-1, "red"), (1, "screen")):
    sphere(f"NavLight_{side}", (-38.5, side * 3.2, 24.75), 0.11, MAT[color], COL["70_NAVIGATION"])

# Bow-thruster machinery stays inside the forepeak; the temporary black side
# circles are removed until a proper recessed shell opening is authored.
box("BowThrusterMotor", (50.0, 0, 3.1), (3.6, 2.2, 1.8), MAT["blue"], COL["40_MACHINERY_SYSTEMS"], 0.18)
box("BowThruster_Foundation", (50.0, 0, 2.18), (4.6, 3.0, 0.24), MAT["diamond"], COL["40_MACHINERY_SYSTEMS"], 0.06)
cylinder("BowThruster_Coupling", (48.0, 0, 3.1), 0.38, 0.72, MAT["orange"],
         COL["40_MACHINERY_SYSTEMS"], (0, math.pi / 2, 0), 24)
box("BowThruster_ControlCabinet", (52.2, 2.4, 3.12), (0.72, 1.1, 1.95), MAT["blue"],
    COL["40_MACHINERY_SYSTEMS"], 0.08)
box("BowThruster_ControlScreen", (52.2, 1.83, 3.45), (0.42, 0.025, 0.36), MAT["screen"],
    COL["40_MACHINERY_SYSTEMS"], 0.012)
box("SteeringGear_CenterFoundation", (-56.0, 0, 2.18), (5.4, 5.2, 0.26),
    MAT["diamond"], COL["40_MACHINERY_SYSTEMS"], 0.06)

# Four skewed blades, bossing, spinner and balanced rudder form a readable stern
# assembly with actual clearance instead of a propeller embedded in a slab.
cylinder("Stern_ShaftBoss", (-61.25, 0, 3.15), 0.74, 1.7, MAT["hull"], COL["10_HULL"], (0, math.pi / 2, 0), 32)
marine_propeller("MainPropeller", (-62.35, 0, 3.15), 2.05, 4)
rudder_vertices = [(-65.0, -0.13, 1.2), (-63.25, -0.13, 1.65), (-63.15, -0.13, 5.85), (-64.3, -0.13, 7.0),
                   (-65.0, 0.13, 1.2), (-63.25, 0.13, 1.65), (-63.15, 0.13, 5.85), (-64.3, 0.13, 7.0)]
rudder_faces = [(0, 1, 2, 3), (4, 7, 6, 5), (0, 4, 5, 1), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 4, 0)]
mesh_object("Rudder", rudder_vertices, rudder_faces, MAT["hull"], COL["40_MACHINERY_SYSTEMS"], 0.08, True)
tapered_volume("SternSkeg", -62.1, 0, 5.95, 7.55, (3.2, 0.56), (2.4, 0.42),
               0.55, MAT["hull"], COL["10_HULL"], 0.16)
sphere("BulbousBow", (59.0, 0, 4.15), 1.0, MAT["antifoul"], COL["10_HULL"], (3.5, 1.75, 1.45))

# Human-scale labels and deck reference marks retained in a hideable collection.
for label, x, z in (("ENGINE", -45, 4.5), ("HOLD 1", -5, 7.25), ("HOLD 2", 28, 7.25), ("FOREPEAK", 51, 4.5)):
    add_text(f"Label_{label}", label, (x, -9.42, z + 0.1), 0.55, COL["00_REFERENCE"])

# Studio lighting and two cameras for reproducible review images. No water or
# preview ground geometry is stored in the ship master.
runpy.run_path(str(ROOT / "Scripts" / "ocean_freighter_refit.py"))["apply_refit"](globals())
runpy.run_path(str(ROOT / "Scripts" / "ocean_freighter_machinery.py"))["apply_machinery"](globals())

def render_review():
    if os.environ.get("CALMY_SKIP_RENDER") != "1":
        bpy.ops.render.render(write_still=True)

world = bpy.context.scene.world or bpy.data.worlds.new("World")
bpy.context.scene.world = world
world.use_nodes = True
world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.025, 0.045, 0.065, 1)
world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.35

bpy.ops.object.light_add(type="SUN", location=(0, 0, 70))
sun = bpy.context.object
sun.name = "Sun_Key"
sun.rotation_euler = (math.radians(28), math.radians(-18), math.radians(-32))
sun.data.energy = 1.7
sun.data.angle = math.radians(4)
move_to(sun, COL["80_LIGHTS_CAMERAS"])
bpy.ops.object.light_add(type="AREA", location=(-25, -35, 42))
area = bpy.context.object
area.name = "Area_Fill"
area.data.energy = 650
area.data.shape = "DISK"
area.data.size = 45
move_to(area, COL["80_LIGHTS_CAMERAS"])


def point_camera(obj, target):
    obj.rotation_euler = (Vector(target) - obj.location).to_track_quat("-Z", "Y").to_euler()


bpy.ops.object.camera_add(location=(-108, -138, 50))
camera = bpy.context.object
camera.name = "CAM_Overview"
camera.data.lens = 44
point_camera(camera, (0, 0, 9))
move_to(camera, COL["80_LIGHTS_CAMERAS"])
bpy.context.scene.camera = camera

bpy.ops.object.camera_add(location=(-24.0, -6.35, 12.18))
walk_camera = bpy.context.object
walk_camera.name = "CAM_Walkthrough_FirstPerson"
walk_camera.data.lens = 28
walk_camera.data.clip_start = 0.05
walk_camera.data.clip_end = 500.0
point_camera(walk_camera, (8.0, -6.0, 11.75))
move_to(walk_camera, COL["80_LIGHTS_CAMERAS"])

bpy.ops.object.camera_add(location=(-72, -24, 10.5))
review_camera = bpy.context.object
review_camera.name = "CAM_ReviewCloseup"
review_camera.data.lens = 55
point_camera(review_camera, (-60, 0, 4.0))
move_to(review_camera, COL["80_LIGHTS_CAMERAS"])

# Metadata survives in the .blend and makes scale/design intent machine-readable.
scene = bpy.context.scene
scene["asset_name"] = "Calmy Ocean Freighter"
scene["design_status"] = "R4 machinery art pass over R3 circulation; Blender candidate, Unreal validation outstanding"
scene["length_overall_m"] = LENGTH
scene["beam_m"] = BEAM
scene["depth_m"] = DEPTH
scene["design_draft_reference_m"] = 7.2
scene["player_capacity_target"] = "2-12"
scene["source_reference"] = "Original game general arrangement; not a certified naval-architecture design"
scene["walkthrough_camera"] = walk_camera.name
scene["walkthrough_start_m"] = "-24.0, -6.35, 12.18"
scene["walkthrough_controls"] = "Camera view, Shift+` then WASD; mouse look; wheel speed; Shift fast; Esc exits"
COL["90_COLLISION"].hide_render = True
COL["90_COLLISION"].hide_viewport = True
COL["95_EXPORT_LOD"].hide_render = True
COL["95_EXPORT_LOD"].hide_viewport = True

# Render deterministic reviews first, then restore and save the first-person
# inspection state so opening the master starts at human eye height.
bpy.context.scene.camera = camera
scene.render.filepath = str(PREVIEW)
render_review()

# A second reproducible render exposes the internal general arrangement without
# changing the saved master scene's normal visibility.
for obj in scene.objects:
    obj.hide_render = (
        obj.name == "Hull_Complete"
        or obj.name.startswith("Deck_Weather")
        or obj.name.startswith("Deck_Tween")
        or (obj.name.startswith("Hold") and "Hatch" in obj.name)
        or obj.name.endswith("_Roof")
        or "SideWall_-1" in obj.name
    )
bpy.context.scene.camera = walk_camera
walk_camera.location = (-82, -76, 44)
walk_camera.data.lens = 42
point_camera(walk_camera, (-2, 0, 7))
cutaway_preview = PREVIEW.with_name("OceanFreighter-Blender-Cutaway.png")
scene.render.filepath = str(cutaway_preview)
render_review()

# Focused reviews make the previously reported stern, stair and bow defects
# inspectable without relying on the distant overview render.
for obj in scene.objects:
    obj.hide_render = False

review_camera.location = (-61.2, -24.0, 7.0)
review_camera.data.lens = 55
point_camera(review_camera, (-62.8, 0, 3.8))
scene.camera = review_camera
stern_preview = PREVIEW.with_name("OceanFreighter-Blender-Stern.png")
scene.render.filepath = str(stern_preview)
render_review()

review_camera.location = (-22, -32, 25.5)
review_camera.data.lens = 58
point_camera(review_camera, (-42.0, 0, 17.0))
stairs_preview = PREVIEW.with_name("OceanFreighter-Blender-Access.png")
scene.render.filepath = str(stairs_preview)
render_review()

review_camera.location = (68, -30, 19.0)
review_camera.data.lens = 58
point_camera(review_camera, (50.0, 0, 11.2))
bow_preview = PREVIEW.with_name("OceanFreighter-Blender-Bow.png")
scene.render.filepath = str(bow_preview)
render_review()

# Restore the saved master to a real walkable starting viewpoint.
walk_camera.location = (-24.0, -6.35, 12.18)
walk_camera.data.lens = 28
point_camera(walk_camera, (8.0, -6.0, 11.75))
scene.camera = walk_camera
for workspace in bpy.data.workspaces:
    for screen in workspace.screens:
        for area_view in screen.areas:
            if area_view.type == "VIEW_3D":
                area_view.spaces.active.region_3d.view_perspective = "CAMERA"
                area_view.spaces.active.clip_start = 0.05
                area_view.spaces.active.clip_end = 500.0

bpy.ops.file.pack_all()
bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / "CalmyOceanFreighter.blend"))

mesh_objects = [obj for obj in scene.objects if obj.type == "MESH"]
manifest = {
    "asset": "Calmy Ocean Freighter",
    "status": scene["design_status"],
    "dimensions_m": {"length_overall": LENGTH, "beam": BEAM, "hull_depth": DEPTH, "draft_reference": 7.2},
    "deck_levels_m": DECK_Z,
    "target_players": "2-12",
    "collections": {col.name: len(col.objects) for col in COL.values()},
    "mesh_objects": len(mesh_objects),
    "vertices": sum(len(obj.data.vertices) for obj in mesh_objects),
    "polygons": sum(len(obj.data.polygons) for obj in mesh_objects),
    "master_blend": str(OUTPUT / "CalmyOceanFreighter.blend"),
    "preview": str(PREVIEW),
    "cutaway_preview": str(cutaway_preview),
    "stern_preview": str(stern_preview),
    "access_preview": str(stairs_preview),
    "bow_preview": str(bow_preview),
    "walkthrough_camera": walk_camera.name,
    "walkthrough_start_m": list(walk_camera.location),
    "walkthrough_controls": "Open master, hover 3D viewport, Shift+` then WASD",
}
manifest['previews_generated'] = os.environ.get('CALMY_SKIP_RENDER') != '1'
if not manifest['previews_generated']:
    for key in ('preview','cutaway_preview','stern_preview','access_preview','bow_preview'):
        manifest[key] = None
(OUTPUT / "build_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print("CALMY_OCEAN_FREIGHTER_PASS " + json.dumps(manifest, separators=(",", ":")))
