"""R7 maritime crew rebuilt against the R6 concept boards.

Same ten-part physical contract as R6B (identical part names and bind pivots,
centimetre space, GLB per part) so the existing UE rig and automation contract
still apply. Geometry, proportions and palette are re-authored from
SourceAssets/References/CrewR6/*.png.

Run: blender --background --python Scripts/create_crew_r7.py
New output only; never edits R5/R6.
"""
from pathlib import Path
import bpy, json, math
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / 'SourceAssets/Generated/CrewR7'
assert not OUT.exists(), 'Preserve candidate before rebuilding'
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

# Linear values sampled off the concept boards: saturated safety orange, moss
# green storm coat, amber hard hat, black gloves and boots, warm tanned skin.
COLORS = dict(
    ochre=(.475,.132,.019), teal=(.093,.150,.081),
    navy=(.026,.042,.088), base=(.013,.018,.029),
    cuff=(.030,.036,.044), glove=(.014,.017,.021),
    boot=(.010,.011,.014), sole=(.006,.007,.009),
    skin=(.335,.183,.113), misha=(.380,.214,.134),
    stubble=(.268,.152,.097), brow=(.038,.024,.016), socket=(.180,.092,.058),
    eye=(.019,.016,.013), sclera=(.215,.190,.160),
    lip=(.155,.072,.052), trim=(.600,.620,.570),
    lens=(.055,.095,.105), helmet=(.560,.245,.021),
    seam=(.018,.022,.028))
MAT = {}
for name, color in COLORS.items():
    m = bpy.data.materials.new('CR7_' + name); m.diffuse_color = (*color, 1); m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF'); p.inputs['Base Color'].default_value = (*color, 1)
    p.inputs['Roughness'].default_value = {'helmet': .55, 'glove': .78, 'boot': .74,
        'sole': .85, 'lens': .35, 'trim': .48, 'base': .90}.get(name, .87)
    p.inputs['Specular IOR Level'].default_value = .22
    MAT[name] = m
PARTS = []; MANIFEST = []; CHARS = {}


def mesh(name, verts, faces, mat):
    d = bpy.data.meshes.new(name); d.from_pydata([Vector(v) * .01 for v in verts], [], faces); d.update()
    o = bpy.data.objects.new(name, d); bpy.context.collection.objects.link(o)
    d.materials.append(MAT[mat]); PARTS.append(o); return o


def loft(name, rings, mat, fold=0, open_deg=0, n=12):
    """Ring-lofted shell. open_deg cuts a wedge out of the front for hoods/collars."""
    prof = [(math.cos(math.tau * i / n), math.sin(math.tau * i / n)) for i in range(n)]
    verts = []
    for k, (z, rx, ry, cx, cy) in enumerate(rings):
        for i, (x, y) in enumerate(prof):
            # Diagonal offset on interior rings: soft cloth, not stacked armour rings.
            dz = fold * math.sin(i * 1.7 + k * 1.9) if 0 < k < len(rings) - 1 else 0
            verts.append((cx + x * rx, cy + y * ry, z + dz))
    def cut(i):
        a = (math.degrees(math.tau * (i + .5) / n) + 180) % 360 - 180
        return abs(a) < open_deg
    faces = [] if open_deg else [tuple(range(n - 1, -1, -1))]
    for k in range(len(rings) - 1):
        for i in range(n):
            if cut(i): continue
            faces.append((k * n + i, k * n + (i + 1) % n, (k + 1) * n + (i + 1) % n, (k + 1) * n + i))
    if not open_deg: faces.append(tuple((len(rings) - 1) * n + i for i in range(n)))
    return mesh(name, verts, faces, mat)


def box(name, p, d, mat, bevel=.35):
    bpy.ops.mesh.primitive_cube_add(size=1, location=Vector(p) * .01)
    o = bpy.context.object; o.name = name; o.dimensions = Vector(d) * .01
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if bevel:
        mod = o.modifiers.new('Edge facets', 'BEVEL'); mod.width = min(bevel, min(d) * .22) * .01; mod.segments = 1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(MAT[mat]); PARTS.append(o); return o


def blob(name, p, d, mat, seg=8, ring=4):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=seg, ring_count=ring, location=Vector(p) * .01)
    o = bpy.context.object; o.name = name; o.dimensions = Vector(d) * .01
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    o.data.materials.append(MAT[mat]); PARTS.append(o); return o


def bar(name, a, b, width, depth, mat):
    a = Vector(a); b = Vector(b); o = box(name, (a + b) / 2, (depth, width, (b - a).length), mat, .1)
    o.rotation_euler = (b - a).to_track_quat('Z', 'Y').to_euler(); return o


def thicken(o, t):
    mod = o.modifiers.new('Cloth thickness', 'SOLIDIFY'); mod.thickness = t * .01
    bpy.context.view_layer.objects.active = o; bpy.ops.object.modifier_apply(modifier=mod.name); return o


def finish(name, pivot, y):
    bpy.ops.object.select_all(action='DESELECT')
    for o in PARTS: o.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]; bpy.ops.object.join(); o = PARTS[0]; o.name = name
    bpy.context.scene.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(island_margin=.02); bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles(); tri = len(o.data.loop_triangles)
    assert tri < 1900, (name, tri)
    bpy.ops.export_scene.gltf(filepath=str(OUT / (name + '.glb')), export_format='GLB',
                              use_selection=True, export_apply=True)
    MANIFEST.append(dict(name=name, triangles=tri, pivot_cm=pivot, materials=[m.name for m in o.data.materials]))
    o.location = Vector(pivot) * .01 + Vector((0, y, 0)); PARTS.clear(); return o


for variant, y in [('Deckhand', -.59), ('Misha', .59)]:
    coat = 'teal' if variant == 'Deckhand' else 'ochre'
    skin = 'skin' if variant == 'Deckhand' else 'misha'
    objects = []
    def done(part, pivot): objects.append(finish('SM_' + variant + '_' + part, pivot, y))

    # ---- Torso: sloped raglan shoulders, nipped waist, hem at the hip. -------
    loft('Storm coat shell', [(0, 13.2, 19.4, 0, 0), (4, 13.7, 20.1, 0, 0), (14, 12.9, 18.7, .4, 0),
        (24, 13.9, 20.6, .2, 0), (33, 14.3, 22.7, 0, 0), (41, 13.8, 24.4, 0, 0),
        (46, 12.4, 24.0, -.3, 0), (49, 9.8, 20.0, -.6, 0), (51, 7.4, 13.0, -.8, 0),
        (52.5, 6.4, 9.6, -.8, 0)], coat, .9)
    bar('Centre zip', (13.9, 0, 3), (14.5, 0, 45), .9, .5, 'seam')
    bar('Zip storm flap', (14.4, -1.5, 4), (14.9, -1.5, 45), 1.0, .5, coat)
    for s in (-1, 1):
        # Flush welt slits, no strapped-on rectangles.
        bar('Slanted hand pocket', (13.1, s * 7.6, 5.5), (13.8, s * 12.2, 16.5), 1.2, .8, 'seam')
    bar('Chest zip pocket', (14.2, 5.2, 26.5), (14.3, 5.2, 33), .7, .6, 'seam')
    # Standing collar with a black base layer showing in the open throat.
    thicken(loft('Standing collar', [(51.2, 7.2, 9.8, -.8, 0), (54.6, 7.9, 10.3, -1.5, 0),
        (58.4, 7.1, 9.3, -1.9, 0)], coat, open_deg=58), .6)
    loft('Base layer neck', [(48, 6.2, 7.2, 0, 0), (57.6, 6.4, 7.4, -.6, 0)], 'base', .25)
    if variant == 'Misha':
        # The concept shows the hood bunched behind the collar, not stowed away.
        loft('Stowed hood roll', [(43, 5.6, 9.6, -9.0, 0), (48, 7.6, 12.0, -10.6, 0),
            (53, 8.0, 12.4, -11.4, 0), (56.5, 6.2, 9.6, -10.6, 0),
            (58.5, 3.0, 5.2, -9.6, 0)], coat, .9)
    box('Chest camera', (14.6, 6.8, 38.5), (2.6, 4.8, 4.8), 'cuff', .5)
    box('Camera lens', (15.9, 6.8, 39.0), (.8, 2.1, 2.1), 'lens', .25)
    box('Belt radio', (14.4, -8.6, 36.5), (2.5, 4.2, 8.2), 'cuff', .45)
    box('Radio clip', (12.9, -8.6, 33.2), (1.2, 3.0, 3.6), 'seam', .2)
    bar('Flexible antenna', (14.4, -9.8, 40.4), (14.0, -9.8, 45.6), .5, .5, 'cuff')
    for z in (34.6, 36.4, 38.2): box('Radio grille', (15.7, -8.6, z), (.25, 2.2, .35), 'trim', 0)
    done('Torso', (0, 0, -6))

    # ---- Head: smaller adult skull, brow ridge over deep-set eyes. -----------
    loft('Neck column', [(-6.5, 5.4, 5.8, 0, 0), (2, 5.7, 6.0, .3, 0)], skin)
    loft('Base layer roll', [(-6.5, 6.3, 6.7, 0, 0), (-1.5, 6.1, 6.5, .1, 0)], 'base')
    face = loft('Facial anatomy', [(1.5, 4.8, 4.7, 1.6, 0), (4.0, 6.5, 6.4, 1.9, 0),
        (7.5, 7.8, 7.6, 1.7, 0), (11.5, 8.4, 8.1, 1.2, 0), (15.5, 8.6, 8.2, .7, 0),
        (19.0, 8.4, 7.9, .9, 0), (22.5, 8.2, 7.9, 0, 0), (26.0, 7.2, 7.2, -.7, 0),
        (28.8, 4.6, 4.8, -1.1, 0), (30.2, 1.5, 1.6, -1.3, 0)], skin)
    # Cropped hair shell; the skull is never bare under a hat or hood.
    loft('Hair', [(17.8, 8.75, 8.32, .80, 0), (21.0, 8.60, 8.22, .55, 0),
        (24.4, 8.20, 7.85, -.35, 0), (27.0, 7.05, 7.05, -.80, 0), (29.2, 4.85, 5.05, -1.15, 0),
        (30.6, 1.85, 1.95, -1.35, 0)], 'brow', open_deg=82)
    for s in (-1, 1):
        cy = s * 3.6
        # Heavy upper lid and a small muted iris: adult squint, not doll eyes.
        mesh('Eye socket', [(8.60, cy - 1.9, 16.3), (8.76, cy - 1.4, 16.8), (8.80, cy + 1.4, 16.8),
            (8.64, cy + 1.9, 16.3), (8.72, cy, 18.2)], [(0, 1, 4, 3), (1, 2, 4), (2, 3, 4)], 'socket')
        mesh('Eye almond', [(8.72, cy - 1.45, 17.05), (8.80, cy, 17.55), (8.72, cy + 1.45, 17.05),
            (8.76, cy, 16.75)], [(0, 1, 2, 3)], 'sclera')
        box('Iris', (8.86, cy, 17.15), (.16, .95, .85), 'eye', .06)
        mesh('Upper lid', [(8.70, cy - 1.9, 17.5), (8.86, cy - 1.4, 18.15), (8.86, cy + 1.4, 18.15),
            (8.70, cy + 1.9, 17.5)], [(0, 1, 2, 3)], skin)
        # Thick angled brow sitting proud of the ridge.
        mesh('Brow', [(9.32, cy - 2.7, 19.0), (9.44, cy - 2.2, 20.3), (9.32, cy + 2.3, 20.1),
            (9.16, cy + 2.6, 18.7)], [(0, 1, 2, 3)], 'brow')
        loft('Ear', [(12.5, 1.2, .9, -1.4, s * 7.9), (16, 1.7, 1.15, -1.4, s * 8.1),
            (19, 1.1, 1.0, -1.4, s * 7.9)], skin)
    mesh('Nose', [(9.0, -.7, 19.6), (9.0, .7, 19.6), (9.7, -.95, 15.8), (9.7, .95, 15.8),
        (11.1, -1.15, 13.0), (11.1, 1.15, 13.0), (9.5, -2.35, 11.9), (9.5, 2.35, 11.9),
        (10.3, 0, 11.7)],
        [(0, 1, 3, 2), (2, 3, 5, 4), (4, 5, 8), (4, 8, 6), (5, 7, 8), (0, 2, 4), (1, 5, 3),
         (2, 6, 8, 4), (3, 5, 8, 7)], skin)
    bar('Mouth line', (9.15, -2.3, 8.4), (9.15, 2.3, 8.4), .34, .3, 'lip')
    mesh('Lower lip', [(9.05, -2.1, 8.3), (9.25, 0, 7.6), (9.05, 2.1, 8.3)], [(0, 1, 2)], 'lip')
    if variant == 'Misha':
        # Stubble is pigment on the existing jaw faces, never a floating plate.
        face.data.materials.append(MAT['stubble'])
        for f in face.data.polygons:
            c = sum((face.data.vertices[i].co for i in f.vertices), Vector()) / len(f.vertices)
            if c.z < .086 and c.x > .030: f.material_index = 1
        # Deep hard hat: continuous brim, shell down to the brow, front peak.
        loft('Hard hat', [(19.2, 11.3, 10.5, 1.6, 0), (20.5, 11.1, 10.3, 1.6, 0),
            (21.0, 9.7, 9.4, .4, 0), (24.2, 9.8, 9.5, .2, 0), (27.4, 9.2, 9.0, -.1, 0),
            (29.6, 7.4, 7.2, -.5, 0), (31.3, 4.2, 4.1, -.8, 0), (32.1, 1.3, 1.3, -1.0, 0)], 'helmet')
        for s in (-1, 1):
            blob('Ear defender cup', (.2, s * 9.9, 14.4), (7.4, 4.4, 9.4), 'cuff')
            blob('Cup pad', (.2, s * 8.4, 14.4), (6.0, 2.0, 7.8), 'seam', 8, 3)
            bar('Headband arm', (-1.2, s * 9.5, 18.6), (-2.6, s * 7.8, 25.0), 1.4, .9, 'seam')
    else:
        # Storm hood up, tight opening, finished edge, drawcord toggles.
        thicken(loft('Storm hood', [(-2.0, 8.9, 9.1, -1.2, 0), (6, 10.2, 10.2, -1.4, 0),
            (14, 10.7, 10.5, -1.7, 0), (21, 10.8, 10.5, -2.0, 0), (26.5, 9.7, 9.5, -2.3, 0),
            (30.6, 6.8, 6.7, -2.7, 0)], coat, .3, open_deg=54), .6)
        loft('Hood crown', [(29.8, 7.3, 7.2, -2.6, 0), (32.8, 4.1, 4.1, -3.0, 0),
            (34.1, 1.2, 1.2, -3.3, 0)], coat)
        loft('Face mask', [(1.6, 5.2, 5.1, 1.7, 0), (5.5, 7.7, 7.6, 1.8, 0),
            (10.5, 8.9, 8.6, 2.35, 0), (14.0, 8.9, 8.6, 2.95, 0),
            (16.8, 8.8, 8.5, 1.7, 0)], 'base', .2)
        for s in (-1, 1):
            bar('Hood drawcord', (7.6, s * 5.6, 1.0), (8.2, s * 5.0, -3.8), .45, .45, 'seam')
            box('Cord toggle', (8.3, s * 4.9, -4.6), (1.0, 1.0, 1.4), 'cuff', .25)
    done('Head', (0, 0, 51))

    for side, s in [('L', -1), ('R', 1)]:
        # ---- Upper arm: rounded raglan cap, full reflective ring. -----------
        loft('Raglan shoulder', [(-28, 6.0, 6.2, 0, 0), (-22, 6.6, 6.7, .3, 0),
            (-16, 7.2, 7.3, .1, -s * .8), (-11, 7.6, 7.8, 0, -s * 2.0),
            (-7.5, 6.6, 7.0, 0, -s * 3.4), (-5.5, 4.0, 4.4, 0, -s * 4.6),
            (-4.5, 1.4, 1.6, 0, -s * 5.2)], coat, .4)
        loft('Sleeve reflective ring', [(-19.4, 7.30, 7.40, .18, -s * .35),
            (-15.9, 7.62, 7.72, .12, -s * .55)], 'trim')
        done('UpperArm' + side, (0, s * 27, 44))

        # ---- Forearm: sleeve, dark cuff, black mitten glove. ----------------
        loft('Sleeve', [(-22.5, 4.6, 5.0, 0, 0), (-18, 5.2, 5.4, .3, 0), (-11, 5.9, 6.0, -.1, .15),
            (-4.5, 5.9, 6.0, 0, 0), (0, 5.2, 5.3, 0, 0), (2.6, 4.2, 4.3, 0, 0)], coat, .5)
        loft('Elastic cuff', [(-24.6, 4.75, 5.15, .1, 0), (-21.8, 4.95, 5.35, .05, 0)], 'cuff')
        loft('Glove gauntlet', [(-27.6, 4.55, 4.10, .35, 0), (-24.2, 4.80, 4.35, .15, 0)], 'glove')
        loft('Cuff stripe upper', [(-26.9, 4.86, 4.41, .32, 0), (-26.35, 4.89, 4.44, .31, 0)], 'trim')
        loft('Cuff stripe lower', [(-25.7, 4.96, 4.51, .26, 0), (-25.15, 4.98, 4.53, .25, 0)], 'trim')
        # One fused mitten mass with shallow grooves: no segmented robot digits.
        loft('Glove hand', [(-27.2, 4.7, 3.9, .4, 0), (-29.8, 5.3, 4.2, .8, 0),
            (-33.0, 5.2, 4.1, 1.0, 0), (-35.4, 4.2, 3.3, 1.1, 0),
            (-36.6, 2.0, 1.6, 1.1, 0)], 'glove', .18)
        for g in (-1.9, 0, 1.9):
            bar('Finger groove', (5.85, g, -30.8), (5.2, g, -35.6), .34, .32, 'seam')
        loft('Thumb', [(-28.6, 1.5, 1.5, 4.5, -s * 1.9), (-30.6, 1.7, 1.6, 5.0, -s * 2.5),
            (-32.5, 1.3, 1.3, 4.4, -s * 2.7)], 'glove')
        done('Forearm' + side, (0, s * 29, 17))

        # ---- Legs: work trousers with hi-vis bands, black safety boots. -----
        loft('Work trousers', [(-39, 7.4, 7.6, .1, 0), (-31, 7.9, 8.1, .7, 0), (-19, 8.9, 9.0, .1, 0),
            (-7, 10.2, 10.4, -.3, 0), (2, 10.6, 10.8, 0, 0)], 'navy', 1.0)
        box('Cargo pocket', (-.5, s * 8.9, -14), (8.2, 1.1, 11.5), 'navy', .35)
        box('Pocket flap', (-.5, s * 9.4, -8.6), (8.4, .6, 1.4), 'seam', .12)
        done('Thigh' + side, (0, s * 12, -6))

        loft('Trouser lower leg', [(-30, 6.9, 7.3, .2, 0), (-24, 7.4, 7.8, -.2, 0),
            (-13, 7.7, 8.0, .3, 0), (-5, 7.9, 8.1, 0, 0), (2, 7.9, 8.1, 0, 0)], 'navy', .7)
        loft('Calf reflective band', [(-14.8, 7.83, 8.13, .24, 0), (-12.8, 7.88, 8.18, .25, 0)], 'trim')
        loft('Ankle reflective band', [(-24.4, 7.48, 7.88, -.14, 0), (-22.4, 7.58, 7.98, -.10, 0)], 'trim')
        bar('Outseam stripe', (1.4, s * 7.85, -23.0), (1.4, s * 8.05, -6.0), .34, 2.2, 'trim')
        loft('Boot shaft', [(-38.5, 7.7, 7.0, .9, 0), (-33, 7.3, 6.9, .4, 0),
            (-29.2, 7.0, 6.9, 0, 0)], 'boot')
        loft('Safety toe', [(-41.8, 13.4, 7.3, 5.4, 0), (-38.6, 13.6, 7.5, 5.2, 0),
            (-35.2, 11.8, 7.2, 4.8, 0), (-31, 6.2, 6.7, .8, 0)], 'boot')
        loft('Boot outsole', [(-43.6, 14.0, 7.7, 5.5, 0), (-41.2, 14.2, 7.8, 5.5, 0)], 'sole')
        bar('Toe cap seam', (12.5, -6.6, -37.4), (12.5, 6.6, -37.4), .7, .5, 'seam')
        for z in (-31.5, -33.5): bar('Lace bar', (7.4, -2.4, z), (7.4, 2.4, z), .6, .5, 'seam')
        box('Boot pull loop', (-6.9, 0, -28.6), (1.0, 3.0, 3.6), 'cuff', .2)
        done('Shin' + side, (0, s * 12, -44))
    CHARS[variant] = objects

totals = {v: sum(p['triangles'] for p in MANIFEST if p['name'].startswith('SM_' + v + '_')) for v in CHARS}
assert all(1000 < n < 6500 for n in totals.values()), totals
(OUT / 'mesh_manifest.json').write_text(json.dumps(dict(
    units='metres; bind pivots in cm', style='concept-matched faceted low-poly maritime workwear',
    reference='SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png (+DECKHAND)',
    triangles=totals, parts=MANIFEST), indent=2))

scene = bpy.context.scene; scene.render.engine = 'CYCLES'; scene.cycles.samples = 32
scene.world = bpy.data.worlds.new('Neutral studio'); scene.world.color = (.16, .16, .16)
scene.view_settings.view_transform = 'AgX'
box('Studio floor', (0, 0, -90), (1000, 1000, 2), 'navy', 0); PARTS.clear()
for loc, power, size in [((3, -4, 5), 550, 4), ((-2, 3, 4), 650, 3), ((3, 3, 2), 240, 3)]:
    bpy.ops.object.light_add(type='AREA', location=loc); o = bpy.context.object
    o.data.energy = power; o.data.shape = 'DISK'; o.data.size = size
    o.rotation_euler = (Vector((0, 0, .2)) - o.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.camera_add(location=(4, -2, 1.35)); cam = bpy.context.object; scene.camera = cam
cam.data.type = 'ORTHO'
scene.render.resolution_x = 1500; scene.render.resolution_y = 1100; scene.render.resolution_percentage = 100
def render(name, loc, target, scale):
    cam.location = loc
    cam.rotation_euler = (Vector(target) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = scale; scene.render.filepath = str(OUT / name)
    bpy.ops.render.render(write_still=True)
cam.data.ortho_scale = 3.1
cam.rotation_euler = (Vector((0, 0, 0)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Crew_R7.blend'))
render('R7_Lineup.png', (4, -1.7, 1.0), (0, 0, -.02), 2.9)
render('R7_Faces.png', (3, -.8, .8), (0, 0, .64), 1.65)
render('R7_Back.png', (-4, 1.5, 1.0), (0, 0, 0), 2.9)
render('R7_Misha_ThreeQuarter.png', (2.6, 1.1, .95), (0, .59, .18), 1.5)
print('CREW_R7_AUTHORED', totals)
