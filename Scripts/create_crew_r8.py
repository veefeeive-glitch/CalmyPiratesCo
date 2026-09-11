"""R8 maritime crew: modelled surfaces + baked texture, not lofted tubes.

Four things R7 got wrong, and what changed:

1. FACES were flat masks with a separate nose wedge and eye/brow quads laid on
   top. Here the head is a shaped grid (14 rows x 22 front-dense columns) whose
   vertices are displaced into a real brow ridge, recessed eye sockets, a nose
   built out of the surface, cheekbones and a jaw taper. Eyes, iris, brows,
   lips and stubble are MATERIAL ASSIGNMENTS on those existing faces - nothing
   is stuck on.
2. SILHOUETTE was a body of revolution everywhere. Every ring now uses a
   superellipse exponent, so the coat is a flat-fronted panelled section and
   the boots are box-toed, instead of barrels and cylinders.
3. NO DEPTH: flat colour on flat facets. Every part now bakes its palette
   multiplied by an ambient-occlusion pass (with the whole assembled body in
   the scene, so limbs occlude the coat) into a single PNG base-colour map.
4. SEAMS: each child part tapers hard inside its parent's bore and the visible
   break is moved onto a cuff or band.

Contract unchanged from R6B/R7: same 20 part names, same 10 bind pivots,
centimetre space, one GLB per part.

Run: blender --background --python Scripts/create_crew_r8.py
New output only; never edits R5/R6/R7.
"""
from pathlib import Path
import bpy, json, math
import numpy as np
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / 'SourceAssets/Generated/CrewR8'
assert not OUT.exists(), 'Preserve candidate before rebuilding'
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

COLORS = dict(
    ochre=(.475,.132,.019), teal=(.093,.150,.081),
    navy=(.026,.042,.088), base=(.013,.018,.029),
    cuff=(.030,.036,.044), glove=(.014,.017,.021),
    boot=(.010,.011,.014), sole=(.006,.007,.009),
    skin=(.285,.155,.096), misha=(.315,.176,.110),
    stubble=(.268,.152,.097), brow=(.038,.024,.016), socket=(.180,.092,.058),
    eye=(.019,.016,.013), sclera=(.340,.300,.258),
    lip=(.155,.072,.052), trim=(.600,.620,.570),
    lens=(.055,.095,.105), helmet=(.560,.245,.021),
    seam=(.018,.022,.028))
MAT = {}
for name, color in COLORS.items():
    m = bpy.data.materials.new('CR8_' + name); m.diffuse_color = (*color, 1); m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Roughness'].default_value = .87
    MAT[name] = m
PARTS = []; MANIFEST = []; ASSEMBLED = []


# ---------------------------------------------------------------- primitives
def mesh(name, verts, faces, mat):
    d = bpy.data.meshes.new(name); d.from_pydata([Vector(v) * .01 for v in verts], [], faces); d.update()
    o = bpy.data.objects.new(name, d); bpy.context.collection.objects.link(o)
    d.materials.append(MAT[mat]); PARTS.append(o); return o


ANG12 = [i * 30. for i in range(12)]
ANG16 = [i * 22.5 for i in range(16)]
# Front-dense columns: enough resolution across the eye to model and paint it.
ANGF = [0, 4, 9, 18, 26, 34, 42, 55, 75, 100, 130, 160, 180,
        -160, -130, -100, -75, -55, -42, -34, -26, -18, -9, -4]
#            y at the eye row, cm:  0  1.4  2.4  3.8  4.7  5.6  6.3
MIRROR = {1: 23, 2: 22, 3: 21, 4: 20, 5: 19, 6: 18, 7: 17, 8: 16, 9: 15, 10: 14, 11: 13}


def mirrored(deltas):
    """Author one side of a face; reflect it across the midline with dy negated."""
    out = dict(deltas)
    for (k, i), (dx, dy, dz) in deltas.items():
        if i in MIRROR: out[(k, MIRROR[i])] = (dx, -dy, dz)
    return out


def shell(name, rings, mat, angles=ANG12, fold=0., open_deg=0., deltas=None, caps=True):
    """Ring surface. Each ring is (z, rx, ry, cx, cy[, p]); p is the superellipse
    exponent - 2 is a circle, 3+ flattens the front and back into panels.
    deltas maps (ring, column) -> (dx, dy, dz) to model features into the surface.
    Returns (object, {(ring, column): polygon index}) so faces can be painted."""
    n = len(angles); deltas = deltas or {}; verts = []
    for k, r in enumerate(rings):
        z, rx, ry, cx, cy = r[:5]; p = r[5] if len(r) > 5 else 2.
        for i, a in enumerate(angles):
            t = math.radians(a); c = math.cos(t); s = math.sin(t)
            ux = math.copysign(abs(c) ** (2. / p), c); uy = math.copysign(abs(s) ** (2. / p), s)
            fz = fold * math.sin(i * 1.7 + k * 1.9) if 0 < k < len(rings) - 1 else 0.
            dx, dy, dz = deltas.get((k, i), (0., 0., 0.))
            verts.append((cx + ux * rx + dx, cy + uy * ry + dy, z + fz + dz))

    def midangle(i):
        a0 = math.radians(angles[i]); a1 = math.radians(angles[(i + 1) % n])
        d = (a1 - a0 + math.pi) % math.tau - math.pi
        return abs((math.degrees(a0 + d / 2) + 180) % 360 - 180)

    open_ring = open_deg > 0
    faces = [] if open_ring or not caps else [tuple(range(n - 1, -1, -1))]
    fmap = {}
    for k in range(len(rings) - 1):
        for i in range(n):
            if open_ring and midangle(i) < open_deg: continue
            fmap[(k, i)] = len(faces)
            faces.append((k * n + i, k * n + (i + 1) % n, (k + 1) * n + (i + 1) % n, (k + 1) * n + i))
    if not open_ring and caps:
        faces.append(tuple((len(rings) - 1) * n + i for i in range(n)))
    # fmap indices already account for the bottom cap: it is appended to `faces`
    # before the loop runs, so len(faces) at assignment time is the real index.
    return mesh(name, verts, faces, mat), fmap


def paint(o, fmap, keys, matname):
    """Recolour existing faces. Eyes and brows are surface, never floating quads."""
    names = [m.name for m in o.data.materials]
    if MAT[matname].name not in names:
        o.data.materials.append(MAT[matname]); names.append(MAT[matname].name)
    idx = names.index(MAT[matname].name)
    for key in keys:
        if key in fmap: o.data.polygons[fmap[key]].material_index = idx


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


def assemble(name, pivot, y):
    """Join the pending parts, unwrap, and park at the bind pivot. Export happens
    later, once the whole body exists, so the AO bake sees limb-on-coat contact."""
    bpy.ops.object.select_all(action='DESELECT')
    for o in PARTS: o.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]; bpy.ops.object.join(); o = PARTS[0]; o.name = name
    bpy.context.scene.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(island_margin=.03); bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles(); tri = len(o.data.loop_triangles)
    assert tri < 1900, (name, tri)
    MANIFEST.append(dict(name=name, triangles=tri, pivot_cm=pivot,
                         palette=sorted({m.name for m in o.data.materials})))
    o.location = Vector(pivot) * .01 + Vector((0, y, 0))
    ASSEMBLED.append(o); PARTS.clear(); return o


# ------------------------------------------------------------------- the head
HEAD_ROWS = [  # z,   rx,   ry,   cx,     p
    (3.8, 4.8, 4.8, 1.4, 0, 2.6), (6.6, 6.8, 6.8, 1.8, 0, 2.6),
    (9.2, 7.8, 7.6, 1.7, 0, 2.8), (10.5, 8.0, 7.8, 1.65, 0, 2.8),
    (12.0, 8.2, 8.0, 1.5, 0, 2.9), (14.2, 8.4, 8.1, 1.2, 0, 3.0),
    (16.4, 8.55, 8.2, .85, 0, 3.0), (17.8, 8.6, 8.2, .8, 0, 3.0),
    (19.4, 8.55, 8.15, .8, 0, 3.0), (21.4, 8.45, 8.05, .75, 0, 3.0),
    (24.5, 8.25, 7.95, .15, 0, 2.9), (27.5, 7.4, 7.35, -.6, 0, 2.7),
    (29.6, 4.9, 5.1, -1.1, 0, 2.4), (30.6, 1.7, 1.8, -1.3, 0, 2.2)]
HEAD_DELTAS = mirrored({
    # Nose ridge on columns 0-2 (y 0..2.4 cm), falling away by column 3.
    (4, 0): (.50, 0, 0), (4, 1): (.35, 0, 0),
    (5, 0): (1.70, 0, 0), (5, 1): (1.45, -.20, 0), (5, 2): (.35, -.35, 0),
    (6, 0): (1.05, 0, 0), (6, 1): (.70, 0, 0),
    (7, 0): (.80, 0, 0), (7, 1): (.50, 0, 0),
    (8, 0): (.60, 0, 0), (8, 1): (.45, 0, 0),
    # Sockets recessed on columns 3-4, brow ridge proud above them.
    (6, 3): (-.45, 0, 0), (6, 4): (-.40, 0, 0),
    (7, 3): (-.50, 0, 0), (7, 4): (-.45, 0, 0),
    (8, 3): (-.15, 0, 0), (8, 4): (-.20, 0, 0),
    (9, 0): (.35, 0, 0), (9, 1): (.50, 0, 0), (9, 2): (.65, 0, 0),
    (9, 3): (.60, 0, -.15), (9, 4): (.45, 0, -.25), (9, 5): (.20, 0, -.20),
    # cheekbone, jaw taper, flat temple
    (5, 6): (.30, 0, 0), (5, 7): (.20, 0, 0), (6, 6): (.25, 0, 0),
    (1, 4): (0, -.25, 0), (1, 5): (-.25, -.45, 0), (1, 6): (-.45, -.50, 0),
    (10, 6): (-.25, 0, 0), (10, 7): (-.20, 0, 0)})
# Face keys, chosen by where the column lands in cm rather than by angle.
EYE = [(6, 1), (6, 2), (6, 3), (6, 20), (6, 21), (6, 22)]          # y 1.4..4.7
BROW = [(8, 1), (8, 2), (8, 3), (8, 4), (8, 19), (8, 20), (8, 21), (8, 22)]
LID = [(7, 1), (7, 2), (7, 3), (7, 20), (7, 21), (7, 22)]
MOUTH = [(2, 0), (2, 1), (2, 22), (2, 23)]                          # 4.8 cm wide


def build_head(variant, coat, skin):
    shell('Neck column', [(-6.5, 5.4, 5.8, 0, 0, 2.4), (2.5, 5.8, 6.1, .3, 0, 2.5)], skin)
    shell('Base layer roll', [(-6.5, 6.3, 6.7, 0, 0, 2.4), (-1.5, 6.1, 6.5, .1, 0, 2.4)], 'base')
    face, fm = shell('Facial anatomy', HEAD_ROWS, skin, angles=ANGF, deltas=HEAD_DELTAS)
    # Eyes, brows, lips and stubble are painted into the texture, not assigned
    # to polygons: quad-sized features read as Minecraft blocks at this density.
    shell('Hair', [(19.0, 8.72, 8.30, .70, 0, 3.0), (22.5, 8.66, 8.22, .40, 0, 3.0),
                   (25.4, 8.45, 8.12, -.20, 0, 2.9), (27.8, 7.60, 7.55, -.70, 0, 2.7),
                   (29.7, 5.15, 5.35, -1.10, 0, 2.4), (30.7, 1.95, 2.05, -1.35, 0, 2.2)],
          'brow', angles=ANG16, open_deg=96)
    for s in (-1, 1):
        shell('Ear', [(12.5, 1.2, .9, -1.4, s * 7.9), (16, 1.7, 1.15, -1.4, s * 8.1),
                      (19, 1.1, 1.0, -1.4, s * 7.9)], skin)
    if variant == 'Misha':
        shell('Hard hat', [(22.0, 11.2, 10.4, 1.7, 0, 2.6), (23.3, 11.0, 10.2, 1.7, 0, 2.6),
                           (23.8, 9.9, 9.6, .4, 0, 2.7), (26.5, 10.0, 9.7, .2, 0, 2.7),
                           (29.2, 9.3, 9.1, -.1, 0, 2.6), (31.4, 7.5, 7.3, -.5, 0, 2.4),
                           (33.2, 4.3, 4.2, -.8, 0, 2.2), (34.0, 1.3, 1.3, -1.0, 0, 2.1)],
              'helmet', angles=ANG16)
        for s in (-1, 1):
            shell('Ear defender cup', [(9.9, 2.6, 1.6, .2, s * 9.5, 3.0),
                                       (11.2, 3.4, 2.05, .2, s * 9.5, 3.2),
                                       (16.8, 3.5, 2.10, .2, s * 9.5, 3.2),
                                       (18.1, 2.6, 1.6, .2, s * 9.5, 3.0)], 'cuff', angles=ANG12)
            shell('Cup pad', [(10.7, 2.7, 1.0, .2, s * 8.2, 3.0),
                              (17.3, 2.8, 1.0, .2, s * 8.2, 3.0)], 'seam', angles=ANG12)
            bar('Headband arm', (-1.2, s * 9.2, 18.0), (-2.6, s * 7.8, 25.6), 1.4, .9, 'seam')
    else:
        hood, _ = shell('Storm hood', [(-2.0, 8.9, 9.1, -1.2, 0, 2.7), (6, 10.2, 10.2, -1.4, 0, 2.8),
                                       (14, 10.7, 10.5, -1.7, 0, 2.8), (21, 10.8, 10.5, -2.0, 0, 2.8),
                                       (26.5, 9.7, 9.5, -2.3, 0, 2.6), (30.6, 6.8, 6.7, -2.7, 0, 2.3)],
                        coat, angles=ANG16, fold=.3, open_deg=52)
        thicken(hood, .6)
        shell('Hood crown', [(29.8, 7.3, 7.2, -2.6, 0, 2.4), (32.8, 4.1, 4.1, -3.0, 0, 2.2),
                             (34.1, 1.2, 1.2, -3.3, 0, 2.1)], coat, angles=ANG16)
        shell('Face mask', [(1.6, 5.2, 5.1, 1.7, 0, 2.5), (5.5, 7.7, 7.6, 1.8, 0, 2.7),
                            (10.5, 8.9, 8.6, 2.35, 0, 2.9), (14.0, 8.9, 8.6, 2.95, 0, 2.9),
                            (16.8, 8.8, 8.5, 1.7, 0, 2.9)], 'base', angles=ANG16, fold=.2)
        for s in (-1, 1):
            bar('Hood drawcord', (7.6, s * 5.6, 1.0), (8.2, s * 5.0, -3.8), .45, .45, 'seam')
            box('Cord toggle', (8.3, s * 4.9, -4.6), (1.0, 1.0, 1.4), 'cuff', .25)


# ------------------------------------------------------------------- the body
for variant, y in [('Deckhand', -.59), ('Misha', .59)]:
    coat = 'teal' if variant == 'Deckhand' else 'ochre'
    skin = 'skin' if variant == 'Deckhand' else 'misha'
    def done(part, pivot): assemble('SM_' + variant + '_' + part, pivot, y)

    # Panelled coat: p>3 flattens front and back, so this is a garment section,
    # not a barrel. Slight left/right asymmetry kills the mannequin symmetry.
    shell('Storm coat shell', [
        (0, 13.2, 19.4, 0, 0, 3.4), (4, 13.7, 20.1, 0, .3, 3.4),
        (14, 12.9, 18.7, .4, 0, 3.3), (24, 13.9, 20.6, .2, -.3, 3.2),
        (33, 14.3, 22.7, 0, 0, 3.1), (41, 13.8, 24.4, 0, 0, 2.9),
        (46, 12.4, 24.0, -.3, 0, 2.7), (49, 9.8, 20.0, -.6, 0, 2.5),
        (51, 7.4, 13.0, -.8, 0, 2.3), (52.5, 6.4, 9.6, -.8, 0, 2.2)],
        coat, angles=ANG16, fold=.9)
    bar('Centre zip', (13.9, 0, 3), (14.5, 0, 45), .9, .5, 'seam')
    bar('Zip storm flap', (14.4, -1.5, 4), (14.9, -1.5, 45), 1.0, .5, coat)
    for s in (-1, 1):
        bar('Slanted hand pocket', (13.1, s * 7.6, 5.5), (13.8, s * 12.2, 16.5), 1.2, .8, 'seam')
    bar('Chest zip pocket', (14.2, 5.2, 26.5), (14.3, 5.2, 33), .7, .6, 'seam')
    collar, _ = shell('Standing collar', [(51.2, 7.2, 9.8, -.8, 0, 2.4), (54.6, 7.9, 10.3, -1.5, .3, 2.4),
                                          (58.4, 7.1, 9.3, -1.9, .5, 2.3)], coat, angles=ANG16, open_deg=58)
    thicken(collar, .6)
    shell('Base layer neck', [(48, 6.2, 7.2, 0, 0, 2.3), (57.6, 6.4, 7.4, -.6, 0, 2.3)], 'base', fold=.25)
    if variant == 'Misha':
        shell('Stowed hood roll', [(43, 5.6, 9.6, -9.0, .6, 2.5), (48, 7.6, 12.0, -10.6, .9, 2.6),
                                   (53, 8.0, 12.4, -11.4, .6, 2.6), (56.5, 6.2, 9.6, -10.6, .2, 2.4),
                                   (58.5, 3.0, 5.2, -9.6, 0, 2.2)], coat, angles=ANG16, fold=.9)
    box('Chest camera', (14.6, 6.8, 38.5), (2.6, 4.8, 4.8), 'cuff', .5)
    box('Camera lens', (15.9, 6.8, 39.0), (.8, 2.1, 2.1), 'lens', .25)
    box('Belt radio', (14.4, -8.6, 36.5), (2.5, 4.2, 8.2), 'cuff', .45)
    box('Radio clip', (12.9, -8.6, 33.2), (1.2, 3.0, 3.6), 'seam', .2)
    bar('Flexible antenna', (14.4, -9.8, 40.4), (14.0, -9.8, 45.6), .5, .5, 'cuff')
    for z in (34.6, 36.4, 38.2): box('Radio grille', (15.7, -8.6, z), (.25, 2.2, .35), 'trim', 0)
    done('Torso', (0, 0, -6))

    build_head(variant, coat, skin)
    done('Head', (0, 0, 51))

    for side, s in [('L', -1), ('R', 1)]:
        shell('Raglan shoulder', [(-28, 6.3, 6.5, 0, 0, 2.5), (-22, 6.8, 6.9, .3, 0, 2.5),
                                  (-16, 7.3, 7.4, .1, -s * .8, 2.4), (-11, 7.6, 7.8, 0, -s * 2.0, 2.4),
                                  (-7.5, 6.6, 7.0, 0, -s * 3.4, 2.3), (-5.5, 4.0, 4.4, 0, -s * 4.6, 2.2),
                                  (-4.5, 1.4, 1.6, 0, -s * 5.2, 2.1)], coat, angles=ANG12, fold=.4)
        shell('Sleeve reflective ring', [(-19.4, 7.42, 7.52, .18, -s * .35, 2.4),
                                         (-15.9, 7.72, 7.82, .12, -s * .55, 2.4)], 'trim')
        done('UpperArm' + side, (0, s * 27, 44))

        # Sleeve top tapers well inside the upper arm's bore: the elbow break
        # now falls on the cuff, not on a visible telescoping step.
        shell('Sleeve', [(-22.5, 4.6, 5.0, 0, 0, 2.4), (-18, 5.2, 5.4, .3, 0, 2.4),
                         (-11, 5.9, 6.0, -.1, .15, 2.3), (-4.5, 5.9, 6.0, 0, 0, 2.3),
                         (0, 5.0, 5.1, 0, 0, 2.2), (2.6, 3.9, 4.0, 0, 0, 2.1)], coat, fold=.5)
        shell('Elastic cuff', [(-24.6, 4.75, 5.15, .1, 0, 2.3), (-21.8, 4.95, 5.35, .05, 0, 2.3)], 'cuff')
        shell('Glove gauntlet', [(-27.6, 4.55, 4.10, .35, 0, 2.6), (-24.2, 4.80, 4.35, .15, 0, 2.5)], 'glove')
        shell('Cuff stripe upper', [(-26.9, 4.86, 4.41, .32, 0, 2.6), (-26.35, 4.89, 4.44, .31, 0, 2.6)], 'trim')
        shell('Cuff stripe lower', [(-25.7, 4.96, 4.51, .26, 0, 2.6), (-25.15, 4.98, 4.53, .25, 0, 2.6)], 'trim')
        shell('Glove hand', [(-27.2, 4.7, 3.9, .4, 0, 2.8), (-29.8, 5.3, 4.2, .8, 0, 3.0),
                             (-33.0, 5.2, 4.1, 1.0, 0, 3.0), (-35.4, 4.2, 3.3, 1.1, 0, 2.8),
                             (-36.6, 2.0, 1.6, 1.1, 0, 2.4)], 'glove', fold=.18)
        for g in (-1.9, 0, 1.9):
            bar('Finger groove', (5.85, g, -30.8), (5.2, g, -35.6), .34, .32, 'seam')
        shell('Thumb', [(-28.6, 1.5, 1.5, 4.5, -s * 1.9), (-30.6, 1.7, 1.6, 5.0, -s * 2.5),
                        (-32.5, 1.3, 1.3, 4.4, -s * 2.7)], 'glove')
        done('Forearm' + side, (0, s * 29, 17))

        shell('Work trousers', [(-39, 7.8, 8.0, .1, 0, 2.6), (-31, 8.1, 8.3, .7, 0, 2.6),
                                (-19, 9.0, 9.1, .1, 0, 2.5), (-7, 10.2, 10.4, -.3, 0, 2.4),
                                (2, 10.6, 10.8, 0, 0, 2.3)], 'navy', fold=1.0)
        box('Cargo pocket', (-.5, s * 8.9, -14), (8.2, 1.1, 11.5), 'navy', .35)
        box('Pocket flap', (-.5, s * 9.4, -8.6), (8.4, .6, 1.4), 'seam', .12)
        done('Thigh' + side, (0, s * 12, -6))

        # Trouser top tucks inside the thigh bore instead of flaring past it.
        shell('Trouser lower leg', [(-30, 6.9, 7.3, .2, 0, 2.5), (-24, 7.4, 7.8, -.2, 0, 2.5),
                                    (-13, 7.7, 8.0, .3, 0, 2.4), (-5, 7.6, 7.8, 0, 0, 2.4),
                                    (2, 6.7, 6.9, 0, 0, 2.3)], 'navy', fold=.7)
        shell('Calf reflective band', [(-14.8, 7.83, 8.13, .24, 0, 2.4), (-12.8, 7.88, 8.18, .25, 0, 2.4)], 'trim')
        shell('Ankle reflective band', [(-24.4, 7.48, 7.88, -.14, 0, 2.5), (-22.4, 7.58, 7.98, -.10, 0, 2.5)], 'trim')
        bar('Outseam stripe', (1.4, s * 7.85, -23.0), (1.4, s * 8.05, -6.0), .34, 2.2, 'trim')
        shell('Boot shaft', [(-38.5, 7.7, 7.0, .9, 0, 2.8), (-33, 7.3, 6.9, .4, 0, 2.8),
                             (-29.2, 7.0, 6.9, 0, 0, 2.6)], 'boot')
        # Box-toed work boot: a squared section, not a rounded duck bill.
        shell('Safety toe', [(-41.8, 13.4, 7.3, 5.4, 0, 3.4), (-38.6, 13.6, 7.5, 5.2, 0, 3.4),
                             (-35.2, 11.8, 7.2, 4.8, 0, 3.1), (-31, 6.2, 6.7, .8, 0, 2.8)], 'boot')
        shell('Boot outsole', [(-43.6, 14.0, 7.7, 5.5, 0, 3.6), (-41.2, 14.2, 7.8, 5.5, 0, 3.6)], 'sole')
        bar('Toe cap seam', (12.5, -6.6, -37.4), (12.5, 6.6, -37.4), .7, .5, 'seam')
        for z in (-31.5, -33.5): bar('Lace bar', (7.4, -2.4, z), (7.4, 2.4, z), .6, .5, 'seam')
        box('Boot pull loop', (-6.9, 0, -28.6), (1.0, 3.0, 3.6), 'cuff', .2)
        done('Shin' + side, (0, s * 12, -44))

totals = {v: sum(p['triangles'] for p in MANIFEST if p['name'].startswith('SM_' + v + '_'))
          for v in ('Deckhand', 'Misha')}
assert all(1000 < n < 6500 for n in totals.values()), totals

# --------------------------------------------------- bake palette x occlusion
scene = bpy.context.scene
scene.render.engine = 'CYCLES'; scene.cycles.device = 'CPU'
scene.world = bpy.data.worlds.new('Neutral studio'); scene.world.color = (.16, .16, .16)
scene.view_settings.view_transform = 'AgX'
bake = scene.render.bake; bake.margin = 12; bake.use_clear = True
rng = np.random.default_rng(7)


def grain(res, cells=9, amp=.022):
    """Very soft large-scale cloth variation. Bilinear, so it never blocks up."""
    g = rng.random((cells + 1, cells + 1)).astype(np.float32)
    t = np.linspace(0, cells, res, dtype=np.float32)
    i0 = np.floor(t).astype(int); i1 = np.minimum(i0 + 1, cells); f = (t - i0).astype(np.float32)
    row0 = g[np.ix_(i0, i0)] * (1 - f)[None, :] + g[np.ix_(i0, i1)] * f[None, :]
    row1 = g[np.ix_(i1, i0)] * (1 - f)[None, :] + g[np.ix_(i1, i1)] * f[None, :]
    out = row0 * (1 - f)[:, None] + row1 * f[:, None]
    out = (out - out.mean()) / (out.std() + 1e-6)
    return np.clip(1. + out * amp, .94, 1.06).reshape(-1)


def bake_position(o, res):
    """Bake world position into a float image so texels can be addressed in 3D."""
    saved = list(o.data.materials)
    pm = bpy.data.materials.new('POS_' + o.name); pm.use_nodes = True
    nt = pm.node_tree; nt.nodes.clear()
    geo = nt.nodes.new('ShaderNodeNewGeometry')
    off = nt.nodes.new('ShaderNodeVectorMath'); off.operation = 'ADD'
    off.inputs[1].default_value = (.5, .5, .5)
    emi = nt.nodes.new('ShaderNodeEmission'); out = nt.nodes.new('ShaderNodeOutputMaterial')
    nt.links.new(off.inputs[0], geo.outputs['Position'])
    nt.links.new(emi.inputs['Color'], off.outputs[0])
    nt.links.new(out.inputs['Surface'], emi.outputs['Emission'])
    img = bpy.data.images.new(o.name + '_P', res, res, float_buffer=True)
    tex = nt.nodes.new('ShaderNodeTexImage'); tex.image = img; nt.nodes.active = tex
    o.data.materials.clear(); o.data.materials.append(pm)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o
    scene.cycles.samples = 1
    bpy.ops.object.bake(type='EMIT')
    px = np.empty(res * res * 4, np.float32); img.pixels.foreach_get(px)
    o.data.materials.clear()
    for m in saved: o.data.materials.append(m)
    bpy.data.images.remove(img); bpy.data.materials.remove(pm)
    world = np.array([o.location.x, o.location.y, o.location.z], np.float32)
    return (px.reshape(-1, 4)[:, :3] - .5 - world) * 100.


FACE_INK = dict(brow=(.085,.062,.046), iris=(.070,.060,.055), sclera=(.720,.675,.625),
                lip=(.375,.225,.190), stubble=(.360,.268,.212), shadow=(.415,.288,.228),
                rim=(.455,.320,.255))


def face_features(rgb, pos, skinname, stubbled):
    """Draw the face analytically in centimetre space, with soft edges, so the
    result does not depend on where the polygons happen to fall."""
    X, Y, Z = pos[:, 0], pos[:, 1], pos[:, 2]
    front = np.clip((X - 2.5) / 2.5, 0, 1)
    # Only ever paint on bare skin - never the mask, helmet, hair or ear cups.
    # Image.pixels hands back the byte buffer's own encoding, so match the skin
    # reference in both scene-linear and sRGB and take whichever hits.
    lin = np.array(COLORS[skinname], np.float32)
    enc = np.where(lin <= .0031308, lin * 12.92,
                   1.055 * np.power(np.clip(lin, 1e-6, None), 1 / 2.4) - .055).astype(np.float32)
    on_skin = np.maximum(
        np.clip(1. - np.linalg.norm(rgb - lin[None, :], axis=1) / .06, 0, 1),
        np.clip(1. - np.linalg.norm(rgb - enc[None, :], axis=1) / .06, 0, 1))
    gate = front * on_skin

    def blob2d(cy, cz, ry, rz, soft=.30, rot=0.):
        dy = np.abs(Y) - cy; dz = Z - cz
        if rot:
            c, sn = math.cos(rot), math.sin(rot)
            dy, dz = dy * c + dz * sn, dz * c - dy * sn
        d = np.sqrt((dy / ry) ** 2 + (dz / rz) ** 2)
        return np.clip((1. - d) / soft, 0, 1) * gate

    def mix(dst, mask, key, k=1.):
        m = (mask * k)[:, None]
        return dst * (1. - m) + np.array(FACE_INK[key], np.float32)[None, :] * m

    socket = blob2d(3.15, 17.90, 2.55, 1.60, .95)   # orbit shadow
    sclera = blob2d(3.05, 17.25, 1.40, .42)         # light opening ...
    iris = blob2d(3.05, 17.25, .44, .36)            # ... with a dark iris inside it
    brow = blob2d(3.35, 18.80, 2.55, .52, .44, -.16)
    lip = blob2d(0.00, 10.20, 2.30, .44)
    # A nose reads from the shadow down each side, not from nostril dots.
    noseside = blob2d(1.75, 15.20, .52, 2.40, .55)
    undernose = blob2d(0.00, 13.10, 1.45, .45, .55)
    # Soft rim shading so the cheeks and temples turn away instead of staying flat.
    rim = np.clip((np.abs(Y) - 4.9) / 2.1, 0, 1) * gate
    jaw = np.clip((7.8 - Z) / 3.0, 0, 1) * np.clip((Z - 2.2) / 2.0, 0, 1) * gate
    rgb = mix(rgb, rim, 'rim', .40)
    rgb = mix(rgb, jaw, 'rim', .30)
    if stubbled:
        st = np.clip((14.2 - Z) / 3.2, 0, 1) * np.clip((Z - 2.4) / 2.2, 0, 1) * gate
        rgb = mix(rgb, st, 'stubble', .68)
    rgb = mix(rgb, socket, 'shadow', .48)
    rgb = mix(rgb, noseside, 'shadow', .50)
    rgb = mix(rgb, undernose, 'shadow', .60)
    rgb = mix(rgb, lip, 'lip', .70)
    rgb = mix(rgb, sclera, 'sclera', .74)
    rgb = mix(rgb, iris, 'iris', .92)
    rgb = mix(rgb, brow, 'brow', .74)
    return rgb


def bake_part(o, res, cloth=True):
    imgs = []
    for tag in ('C', 'A'):
        img = bpy.data.images.new(o.name + '_' + tag, res, res, alpha=False); imgs.append(img)
    nodes = []
    for m in o.data.materials:
        n = m.node_tree.nodes.new('ShaderNodeTexImage'); m.node_tree.nodes.active = n
        nodes.append(n)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o
    for n in nodes: n.image = imgs[0]
    scene.cycles.samples = 1
    bake.use_pass_direct = False; bake.use_pass_indirect = False; bake.use_pass_color = True
    bpy.ops.object.bake(type='DIFFUSE')
    for n in nodes: n.image = imgs[1]
    scene.cycles.samples = 64
    bpy.ops.object.bake(type='AO')
    px = np.empty(res * res * 4, np.float32); imgs[0].pixels.foreach_get(px)
    ao = np.empty(res * res * 4, np.float32); imgs[1].pixels.foreach_get(ao)
    px = px.reshape(-1, 4); shade = ao.reshape(-1, 4)[:, 0]
    if 'Head' in o.name:
        px[:, :3] = face_features(px[:, :3], bake_position(o, res),
                                  'misha' if 'Misha' in o.name else 'skin', 'Misha' in o.name)
    shade = .30 + .70 * np.power(np.clip(shade, 0, 1), 1.35)   # crevices readable, not black
    if cloth: shade = shade * grain(res)
    px[:, 0] *= shade; px[:, 1] *= shade; px[:, 2] *= shade; px[:, 3] = 1.
    out = bpy.data.images.new('T_' + o.name, res, res, alpha=False)
    out.pixels.foreach_set(px.reshape(-1))
    out.filepath_raw = str(OUT / ('T_' + o.name + '.png')); out.file_format = 'PNG'; out.save()
    for m, n in zip(o.data.materials, nodes): m.node_tree.nodes.remove(n)
    for img in imgs: bpy.data.images.remove(img)
    mat = bpy.data.materials.new(o.name + '_M'); mat.use_nodes = True
    bsdf = mat.node_tree.nodes['Principled BSDF']
    tex = mat.node_tree.nodes.new('ShaderNodeTexImage'); tex.image = out
    mat.node_tree.links.new(bsdf.inputs['Base Color'], tex.outputs['Color'])
    bsdf.inputs['Roughness'].default_value = .86
    o.data.materials.clear(); o.data.materials.append(mat)


for o in ASSEMBLED:
    res = 1024 if ('Torso' in o.name or 'Head' in o.name) else 512
    bake_part(o, res, cloth='Head' not in o.name)
    print('CREW_R8_BAKED', o.name, res)

for entry in MANIFEST:
    o = bpy.data.objects[entry['name']]
    keep = o.location.copy(); o.location = (0, 0, 0)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.export_scene.gltf(filepath=str(OUT / (entry['name'] + '.glb')), export_format='GLB',
                              use_selection=True, export_apply=True)
    o.location = keep
    entry['texture'] = 'T_' + entry['name'] + '.png'

(OUT / 'mesh_manifest.json').write_text(json.dumps(dict(
    units='metres; bind pivots in cm',
    style='modelled low-poly maritime workwear, baked palette x ambient occlusion',
    reference='SourceAssets/References/CrewR6/CONCEPT-not-gameplay.png (+DECKHAND)',
    triangles=totals, parts=MANIFEST), indent=2))

# ------------------------------------------------------------------- previews
box('Studio floor', (0, 0, -90), (1000, 1000, 2), 'navy', 0); PARTS.clear()
for loc, power, size in [((3, -4, 5), 550, 4), ((-2, 3, 4), 650, 3), ((3, 3, 2), 240, 3)]:
    bpy.ops.object.light_add(type='AREA', location=loc); o = bpy.context.object
    o.data.energy = power; o.data.shape = 'DISK'; o.data.size = size
    o.rotation_euler = (Vector((0, 0, .2)) - o.location).to_track_quat('-Z', 'Y').to_euler()
bpy.ops.object.camera_add(location=(4, -2, 1.35)); cam = bpy.context.object; scene.camera = cam
cam.data.type = 'ORTHO'
scene.cycles.samples = 48
scene.render.resolution_x = 1500; scene.render.resolution_y = 1100; scene.render.resolution_percentage = 100
def render(name, loc, target, scale):
    cam.location = loc
    cam.rotation_euler = (Vector(target) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    cam.data.ortho_scale = scale; scene.render.filepath = str(OUT / name)
    bpy.ops.render.render(write_still=True)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / 'Crew_R8.blend'))
render('R8_Lineup.png', (4, -1.7, 1.0), (0, 0, -.02), 2.9)
render('R8_Faces.png', (3, -.8, .8), (0, 0, .64), 1.65)
render('R8_Back.png', (-4, 1.5, 1.0), (0, 0, 0), 2.9)
render('R8_Misha_ThreeQuarter.png', (2.6, 1.1, .95), (0, .59, .18), 1.5)
print('CREW_R8_AUTHORED', totals)
