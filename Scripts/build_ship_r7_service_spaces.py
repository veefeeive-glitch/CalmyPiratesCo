"""R7 service spaces S1: additive geometry for the parts the playable ship still lacked.

Runs headless on a COPY of the hash-pinned master; the master .blend is never saved or changed.
Everything new goes into collection 80_SERVICE_SPACES and is exported as one GLB
(SourceAssets/Generated/ShipR7ServiceSpaces/SM_R7_80_SERVICE_SPACES.glb) that the importer places
on the existing map, attached to ShipMotion_R7. Each space checks its volume against the current
model first and is skipped (reported) instead of being built inside existing geometry.

Adds: chain locker in the forepeak, a weather-deck service house (emergency generator + provision store)
forward of the superstructure, ship's office corner in the wheelhouse, forward mast with masthead and anchor
lights, stern and aft anchor lights, draught marks and the ship's name. The main switchboards already exist in
the engine room; bridge wings are not built because the exterior stairs occupy the wheelhouse sides.

blender -b <master.blend> --python Scripts/build_ship_r7_service_spaces.py -- <out_dir>
"""
import bpy, bmesh, hashlib, json, math, re, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
HASH = '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
OUT = Path(sys.argv[sys.argv.index('--') + 1]) if '--' in sys.argv else ROOT / 'SourceAssets/Generated/ShipR7ServiceSpaces'
assert hashlib.sha256(MASTER.read_bytes()).hexdigest() == HASH, 'Master changed; review before adding service spaces'
assert Path(bpy.data.filepath).resolve() == MASTER.resolve(), 'Open the R7 master read-only'
OUT.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
COL = bpy.data.collections.new('80_SERVICE_SPACES')
scene.collection.children.link(COL)

# ------------------------------------------------------------------ existing model occupancy
def world_bbox(o):
    pts = [o.matrix_world @ Vector(c) for c in o.bound_box]
    return (min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts),
            max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))

EXISTING = [(o.name, world_bbox(o)) for o in scene.objects
            if o.type in ('MESH', 'CURVE', 'FONT') and not o.hide_render
            and not any(c.hide_render for c in o.users_collection)]

def blockers(x0, y0, z0, x1, y1, z1):
    """Existing objects intruding into a volume, ignoring hull shells and ship-spanning deck/ceiling plates."""
    hits = []
    for name, (a, b, c, d, e, f) in EXISTING:
        if name == 'Hull_Complete' or d - a > 30 or e - b > 21: continue
        if f - c < .25 and (d - a > 6 or e - b > 6): continue
        if a < x1 and d > x0 and b < y1 and e > y0 and c < z1 and f > z0: hits.append(name)
    return hits

def floor_under(x0, y0, x1, y1, z):
    """A deck/floor plate spans the footprint within 12 cm below z."""
    for name, (a, b, c, d, e, f) in EXISTING:
        if f - c < .35 and a <= x0 + .05 and d >= x1 - .05 and b <= y0 + .05 and e >= y1 - .05 and z - .12 <= f <= z + .05:
            return name
    return None

# ------------------------------------------------------------------ materials (reuse master materials)
def mat(name, fallback):
    m = bpy.data.materials.get(name)
    if m: return m
    m = bpy.data.materials.new(name); m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF'); bsdf.inputs['Base Color'].default_value = fallback
    return m

def pick(pattern, fallback):
    for m in bpy.data.materials:
        if re.search(pattern, m.name, re.I): return m
    return mat('MAT_S1_' + pattern.strip('^$').replace('|', '_'), fallback)

M = {
    'steel': pick(r'^MAT_Steel$|Steel', (.45, .47, .5, 1)),
    'dark': pick(r'^MAT_Dark$|Dark', (.06, .07, .08, 1)),
    'white': pick(r'^MAT_White$|White|Paint_White', (.86, .87, .86, 1)),
    'blue': pick(r'^MAT_Blue$|Blue', (.12, .3, .45, 1)),
    'orange': pick(r'Orange', (.9, .38, .08, 1)),
    'red': pick(r'^MAT_Red$|Red', (.7, .06, .05, 1)),
    'floor': pick(r'Floor|Deck', (.22, .23, .24, 1)),
    'wood': pick(r'Wood', (.45, .3, .18, 1)),
    'screen': pick(r'^MAT_Screen$|Screen', (.01, .15, .2, 1)),
    'hull': pick(r'^MAT_Hull$|Hull', (.1, .12, .14, 1)),
    'glass': pick(r'Glass', (.5, .7, .8, .3)),
}
for key, color in {'lamp_green': (0.05, 1, .2, 1), 'lamp_red': (1, .05, .03, 1), 'lamp_white': (1, 1, .9, 1), 'mark_white': (.95, .95, .92, 1),
                   'chain': (.09, .085, .08, 1), 'sign_dark': (.04, .06, .08, 1)}.items():
    m = bpy.data.materials.new('MAT_S1_' + key); m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF'); bsdf.inputs['Base Color'].default_value = color
    if key.startswith('lamp'):
        bsdf.inputs['Emission Color'].default_value = color; bsdf.inputs['Emission Strength'].default_value = 6.0
    else:
        bsdf.inputs['Metallic'].default_value = .35 if key == 'chain' else 0.0
        bsdf.inputs['Roughness'].default_value = .78
    M[key] = m

# ------------------------------------------------------------------ primitives
def mesh_object(name, verts, faces, material):
    me = bpy.data.meshes.new('S1_' + name); me.from_pydata(verts, [], faces); me.update()
    o = bpy.data.objects.new('S1_' + name, me); o.data.materials.append(material); COL.objects.link(o)
    return o

def box(name, loc, dims, m='steel', rot=(0, 0, 0)):
    c = [(-1, -1, -1), (-1, -1, 1), (-1, 1, -1), (-1, 1, 1), (1, -1, -1), (1, -1, 1), (1, 1, -1), (1, 1, 1)]
    v = [tuple(p[i] * dims[i] * .5 for i in range(3)) for p in c]
    f = [(0, 2, 6, 4), (1, 5, 7, 3), (0, 4, 5, 1), (2, 3, 7, 6), (0, 1, 3, 2), (4, 6, 7, 5)]
    o = mesh_object(name, v, f, M[m]); o.location = loc; o.rotation_euler = rot
    return o

def cyl(name, loc, r, h, m='steel', rot=(0, 0, 0), n=16):
    v = [(r * math.cos(i * math.tau / n), r * math.sin(i * math.tau / n), z) for z in (-h / 2, h / 2) for i in range(n)]
    f = [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)] + [tuple(reversed(range(n))), tuple(range(n, 2 * n))]
    o = mesh_object(name, v, f, M[m]); o.location = loc; o.rotation_euler = rot
    return o

def link(name, loc, rot, length=.3, width=.19, wire=.045, m='chain', n=16, k=6):
    """Stud-link chain ring: an oval torus in the local XY plane, long axis X."""
    straight = max(0.0, (length - width) / 2); rad = (width - wire) / 2
    verts, faces = [], []
    for i in range(n):
        t = i * math.tau / n; cx, cy = rad * math.cos(t), rad * math.sin(t)
        cx += straight if math.cos(t) >= 0 else -straight
        nx, ny = math.cos(t), math.sin(t)
        for j in range(k):
            u = j * math.tau / k; verts.append((cx + nx * wire / 2 * math.cos(u), cy + ny * wire / 2 * math.cos(u), wire / 2 * math.sin(u)))
    for i in range(n):
        for j in range(k):
            a, b = i * k + j, i * k + (j + 1) % k; c, d = ((i + 1) % n) * k + (j + 1) % k, ((i + 1) % n) * k + j
            faces.append((a, d, c, b))
    o = mesh_object(name, verts, faces, M[m]); o.location = loc; o.rotation_euler = rot
    return o

def rod(name, a, b, r=.025, m='steel', n=10):
    a, b = Vector(a), Vector(b); d = b - a
    o = cyl(name, (a + b) * .5, r, d.length, m, n=n); o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return o

def text(name, body, loc, size, m='white', rot=(math.pi / 2, 0, 0), align='CENTER', extrude=.004):
    cu = bpy.data.curves.new('S1_' + name, 'FONT'); cu.body = body; cu.size = size; cu.align_x = align; cu.extrude = extrude
    o = bpy.data.objects.new('S1_' + name, cu); o.data.materials.append(M[m]); o.location = loc; o.rotation_euler = rot
    COL.objects.link(o); return o

# Enclosed room: four walls of thickness t with door openings (list of (wall, centre, width)),
# ceiling optional. wall in {'aft','fwd','port','stbd'}; centre along the wall (x for port/stbd, y for aft/fwd).
LIGHTS = []
def room(tag, x0, x1, y0, y1, z, h=2.35, t=.08, doors=(), ceiling=True, label=None, label_face='aft'):
    door_h = 2.0
    def wall(name, axis, fixed, a, b, openings):
        cuts = sorted([(c - w / 2, c + w / 2) for c, w in openings])
        spans, cur = [], a
        for c0, c1 in cuts:
            if c0 > cur: spans.append((cur, c0))
            cur = max(cur, c1)
        if cur < b: spans.append((cur, b))
        for i, (s0, s1) in enumerate(spans):
            if s1 - s0 < .02: continue
            if axis == 'x': box(f'{tag}_{name}{i}', ((s0 + s1) / 2, fixed, z + h / 2), (s1 - s0, t, h), 'white')
            else: box(f'{tag}_{name}{i}', (fixed, (s0 + s1) / 2, z + h / 2), (t, s1 - s0, h), 'white')
        for c, w in openings:  # lintel above each door
            if axis == 'x': box(f'{tag}_{name}Lintel{c:.2f}', (c, fixed, z + door_h + (h - door_h) / 2), (w, t, h - door_h), 'white')
            else: box(f'{tag}_{name}Lintel{c:.2f}', (fixed, c, z + door_h + (h - door_h) / 2), (t, w, h - door_h), 'white')
            if axis == 'x':
                for s in (-1, 1): box(f'{tag}_{name}Jamb{c:.2f}{s}', (c + s * w / 2, fixed, z + door_h / 2), (.06, t + .04, door_h), 'dark')
            else:
                for s in (-1, 1): box(f'{tag}_{name}Jamb{c:.2f}{s}', (fixed, c + s * w / 2, z + door_h / 2), (t + .04, .06, door_h), 'dark')
    op = {k: [(c, w) for wk, c, w in doors if wk == k] for k in ('aft', 'fwd', 'port', 'stbd')}
    wall('Aft', 'y', x0, y0, y1, op['aft']); wall('Fwd', 'y', x1, y0, y1, op['fwd'])
    wall('Port', 'x', y0, x0, x1, op['port']); wall('Stbd', 'x', y1, x0, x1, op['stbd'])
    if ceiling: box(f'{tag}_Ceiling', ((x0 + x1) / 2, (y0 + y1) / 2, z + h + .03), (x1 - x0, y1 - y0, .06), 'white')
    box(f'{tag}_Lamp', ((x0 + x1) / 2, (y0 + y1) / 2, z + h - .04), (.9, .18, .05), 'lamp_white')
    LIGHTS.append({'name': tag, 'blender_m': [round((x0 + x1) / 2, 3), round((y0 + y1) / 2, 3), round(z + h - .25, 3)]})
    if label:
        face = {'aft': (x0 - .05, (y0 + y1) / 2, z + 2.2, (math.pi / 2, 0, -math.pi / 2)),
                'fwd': (x1 + .05, (y0 + y1) / 2, z + 2.2, (math.pi / 2, 0, math.pi / 2)),
                'port': ((x0 + x1) / 2, y0 - .05, z + 2.2, (math.pi / 2, 0, 0)),
                'stbd': ((x0 + x1) / 2, y1 + .05, z + 2.2, (math.pi / 2, 0, math.pi))}[label_face]
        box(f'{tag}_SignPlate', face[:3], (.02, max(.9, len(label) * .09), .2) if label_face in ('aft', 'fwd') else (max(.9, len(label) * .09), .02, .2), 'sign_dark')
        off = {'aft': (-.02, 0, 0), 'fwd': (.02, 0, 0), 'port': (0, -.02, 0), 'stbd': (0, .02, 0)}[label_face]
        text(f'{tag}_Sign', label, (face[0] + off[0], face[1] + off[1], face[2] - .05), .11, 'mark_white', face[3])

REPORT = {'built': [], 'skipped': []}
def space(name, volume, needs_floor_z=None):
    hits = blockers(*volume)
    floor = floor_under(volume[0], volume[1], volume[3], volume[4], needs_floor_z) if needs_floor_z is not None else 'n/a'
    if hits or floor is None:
        REPORT['skipped'].append({'space': name, 'volume': [round(v, 2) for v in volume], 'blockers': hits[:12], 'floor': floor})
        print('S1_SKIP', name, 'blockers=', hits[:8], 'floor=', floor)
        return False
    REPORT['built'].append({'space': name, 'volume': [round(v, 2) for v in volume], 'floor': floor})
    print('S1_BUILD', name, 'floor=', floor)
    return True

# ------------------------------------------------------------------ 1. Chain locker (forepeak tween, behind the X52 bulkhead door)
# Starts 1.65 m forward of the bulkhead: watertight door 29 (pivot X 52.13) swings 180 deg into the forepeak.
Z_TWEEN = 7.33
if space('chain_locker', (53.8, -3.6, Z_TWEEN, 57.8, 3.6, Z_TWEEN + 2.6), Z_TWEEN):
    room('ChainLocker', 53.8, 57.8, -3.6, 3.6, Z_TWEEN, h=2.6, doors=[('aft', 0.0, 1.2)], label='CHAIN LOCKER', label_face='aft')
    for s, y in (('P', -1.7), ('S', 1.7)):
        cyl(f'ChainLocker_Well{s}', (56.1, y, Z_TWEEN + .45), .95, .9, 'dark')
        for k in range(26):  # heaped stud-link chain spiralling in the well
            a = k * .55; r = .18 + .026 * k
            link(f'ChainLocker_Link{s}{k}', (56.1 + r * math.cos(a), y + r * math.sin(a), Z_TWEEN + .93 + .05 * (k % 2)), ((k % 2) * math.pi / 2, 0, a + math.pi / 2))
        rod(f'ChainLocker_SpurlingPipe{s}', (56.1, y, Z_TWEEN + 2.6), (56.1, y, Z_TWEEN + 1.4), .16, 'chain')
        for k in range(4): link(f'ChainLocker_HangingLink{s}{k}', (56.1, y, Z_TWEEN + 1.3 - k * .2), (0, math.pi / 2, k * math.pi / 2))
        cyl(f'ChainLocker_PipeFlange{s}', (56.1, y, Z_TWEEN + 2.5), .24, .06, 'chain')
    box('ChainLocker_BitterEndPlate', (57.7, 0, Z_TWEEN + 1.4), (.06, .6, .5), 'orange')
    for i in range(6): rod(f'ChainLocker_Rung{i}', (57.6, -.3, Z_TWEEN + .3 + i * .35), (57.6, .3, Z_TWEEN + .3 + i * .35), .018)
    box('ChainLocker_DrainGrating', (55.7, 0, Z_TWEEN + .02), (4.0, 7.0, .03), 'dark')

# ------------------------------------------------------------------ 2. Weather-deck service house forward of the superstructure
# Clear of the engine-access stairs (Y +-5.8), the transverse walk (X -25.3) and the crane pedestal (X >= -22.9);
# a 1.4 m passage stays open along the superstructure front (X -31).
Z_WD = 10.58
HX0, HX1, HY = -29.6, -26.4, 3.6
if space('weather_service_house', (HX0, -HY, Z_WD + .05, HX1, HY, Z_WD + 2.6), Z_WD):
    room('EmergencyGen', HX0, HX1, -HY, -.05, Z_WD, h=2.5, doors=[('fwd', -1.9, 1.0)], label='EMERGENCY GENERATOR', label_face='fwd')
    room('ProvisionStore', HX0, HX1, .05, HY, Z_WD, h=2.5, doors=[('fwd', 1.9, 1.0)], label='PROVISION STORE', label_face='fwd')
    box('ServiceHouse_Roof', ((HX0 + HX1) / 2, 0, Z_WD + 2.58), (HX1 - HX0 + .3, 2 * HY + .3, .1), 'white')
    for s_, y in (('P', -2.6), ('S', 2.6)):
        cyl(f'ServiceHouse_Vent{s_}', (-28.0, y, Z_WD + 2.95), .22, .7, 'white')
        box(f'ServiceHouse_VentCowl{s_}', (-27.75, y, Z_WD + 3.3), (.5, .44, .25), 'white')
    # Emergency generator: diesel set on a skid, radiator, day tank, local panel, exhaust through the roof.
    box('EmergencyGen_Skid', (-28.2, -1.9, Z_WD + .12), (2.2, 1.1, .24), 'dark')
    box('EmergencyGen_Engine', (-28.6, -1.9, Z_WD + .75), (1.3, .8, 1.0), 'orange')
    box('EmergencyGen_Alternator', (-27.55, -1.9, Z_WD + .65), (.7, .75, .8), 'blue')
    box('EmergencyGen_Radiator', (-29.3, -1.9, Z_WD + .9), (.25, .95, 1.3), 'dark')
    rod('EmergencyGen_Exhaust', (-28.8, -1.9, Z_WD + 1.25), (-28.8, -1.9, Z_WD + 3.2), .09, 'steel')
    box('EmergencyGen_DayTank', (-29.1, -3.1, Z_WD + 1.4), (.8, .5, .7), 'red')
    box('EmergencyGen_Panel', (-26.75, -3.2, Z_WD + 1.0), (.3, .6, 1.4), 'blue')
    box('EmergencyGen_PanelScreen', (-26.58, -3.2, Z_WD + 1.4), (.02, .35, .22), 'screen')
    # Provision store: freezer cabinets on one wall, shelving with sacks and tins on the other.
    for i in range(2): box(f'Provision_Freezer{i}', (-29.05, .9 + i * 1.2, Z_WD + .9), (.7, 1.1, 1.8), 'white')
    for k in range(3): box(f'Provision_Shelf{k}', (-27.6, 3.25, Z_WD + .45 + k * .6), (1.9, .5, .05), 'wood')
    for i in range(6): box(f'Provision_Sack{i}', (-28.2 + (i % 3) * .6, 3.25, Z_WD + .6 + (i // 3) * .6), (.45, .35, .25), 'wood')
    for i in range(6): cyl(f'Provision_Tin{i}', (-27.0 + (i % 3) * .22, 3.25, Z_WD + 1.9), .08, .16, 'steel')
    box('Provision_Thermometer', (-26.5, .6, Z_WD + 1.5), (.02, .12, .25), 'screen')

# ------------------------------------------------------------------ 4. Ship's office corner, aft port side of the wheelhouse
Z_BR = 18.92
if space('ship_office', (-49.2, -6.3, Z_BR + .05, -45.6, -3.4, Z_BR + 2.2), Z_BR):
    box('Office_Partition', (-45.6, -4.85, Z_BR + .7), (.06, 2.9, 1.4), 'wood')
    box('Office_Desk', (-48.2, -5.7, Z_BR + .74), (1.6, .75, .05), 'wood')
    for dx in (-.75, .75): box(f'Office_DeskLeg{dx}', (-48.2 + dx, -5.7, Z_BR + .37), (.05, .6, .74), 'dark')
    box('Office_Monitor', (-48.2, -6.0, Z_BR + 1.02), (.55, .04, .34), 'screen')
    box('Office_FilingCabinet', (-46.3, -5.9, Z_BR + .65), (.5, .6, 1.3), 'steel')
    box('Office_Safe', (-49.0, -4.0, Z_BR + .4), (.6, .6, .8), 'dark')
    box('Office_NoticeBoard', (-49.38, -4.8, Z_BR + 1.5), (.03, 1.4, .8), 'wood')
    box('Office_SignPlate', (-49.365, -4.8, Z_BR + 2.1), (.02, 1.45, .24), 'sign_dark')
    text('Office_Sign', "SHIP'S OFFICE", (-49.35, -4.8, Z_BR + 2.05), .12, 'mark_white', (math.pi / 2, 0, math.pi / 2))
    box('Office_Chair', (-48.2, -5.1, Z_BR + .45), (.45, .45, .08), 'blue')

# Bridge wings are not built: the R3 exterior stairs already occupy both wheelhouse sides at the bridge deck.

# ------------------------------------------------------------------ 6. Navigation lights (masthead, stern, anchor lights)
def navlight(name, loc, m):
    cyl(name + '_Lens', loc, .09, .2, m); cyl(name + '_Cap', (loc[0], loc[1], loc[2] + .13), .11, .05, 'dark')
    cyl(name + '_Base', (loc[0], loc[1], loc[2] - .13), .1, .06, 'dark')
navlight('NavLight_MastheadAft', (-38.5, 0, 31.0), 'lamp_white')
if space('foremast', (47.5, -.4, 12.0, 48.3, .4, 20.0), None):
    rod('Foremast_Pole', (47.9, 0, 12.0), (47.9, 0, 19.6), .12, 'white')
    rod('Foremast_Yard', (47.9, -1.0, 17.8), (47.9, 1.0, 17.8), .05, 'white')
    navlight('NavLight_MastheadFwd', (47.9, 0, 19.8), 'lamp_white')
    navlight('NavLight_AnchorFwd', (47.9, 0, 16.2), 'lamp_white')
navlight('NavLight_Stern', (-60.6, 0, 11.9), 'lamp_white')
navlight('NavLight_AnchorAft', (-55.2, 0, 22.3), 'lamp_white')

# ------------------------------------------------------------------ 7. Draught marks and ship name on the hull
def hull_half(x):
    stations = [(-61.0, 6.4), (-57.0, 7.7), (-50.0, 9.3), (-38.0, 10.0), (-20.0, 10.2), (0.0, 10.2), (22.0, 10.15), (42.0, 9.7), (52.0, 8.2), (58.0, 4.4), (61.0, .45)]
    for (x0, b0), (x1, b1) in zip(stations, stations[1:]):
        if x0 <= x <= x1: return b0 + (b1 - b0) * (x - x0) / (x1 - x0)
    return stations[0][1]
for side, sign in (('P', -1), ('S', 1)):
    for x, tag in ((52.5, 'Fwd'), (-2.0, 'Mid'), (-57.5, 'Aft')):
        y = sign * (hull_half(x) + .03)
        for d in range(4, 10):
            text(f'Draught{tag}{side}{d}', str(d), (x, y, d * 1.0 - .5), .32, 'mark_white', (math.pi / 2, 0, 0 if sign < 0 else math.pi))
    x = 30.0; y = sign * (hull_half(x) + .04)
    text(f'HullName{side}', 'CALMY NIGHTWATCH', (x, y, 9.2), .75, 'mark_white', (math.pi / 2, 0, 0 if sign < 0 else math.pi))
text('HullNameStern', 'CALMY NIGHTWATCH', (-61.05, 0, 8.6), .7, 'mark_white', (math.pi / 2, 0, -math.pi / 2))
text('HullPortOfRegistry', 'KRONSHTADT', (-61.05, 0, 7.6), .45, 'mark_white', (math.pi / 2, 0, -math.pi / 2))

# ------------------------------------------------------------------ export only the new collection
objs = list(COL.objects)
assert objs, 'nothing built'
deps = bpy.context.evaluated_depsgraph_get()
copies = []
for o in objs:
    data = bpy.data.meshes.new_from_object(o.evaluated_get(deps), depsgraph=deps)
    c = bpy.data.objects.new('export_' + o.name, data); scene.collection.objects.link(c); c.matrix_world = o.matrix_world.copy(); copies.append(c)
bpy.ops.object.select_all(action='DESELECT')
for c in copies: c.select_set(True)
bpy.context.view_layer.objects.active = copies[0]; bpy.ops.object.join(); joined = copies[0]
joined.name = 'SM_R7_80_SERVICE_SPACES'
scene.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
joined.data.calc_loop_triangles()
glb = OUT / 'SM_R7_80_SERVICE_SPACES.glb'
bpy.ops.export_scene.gltf(filepath=str(glb), export_format='GLB', use_selection=True, export_apply=True)
REPORT.update({'lights': LIGHTS, 'master_sha256': HASH, 'objects': len(objs), 'triangles': len(joined.data.loop_triangles), 'glb': str(glb),
               'units': 'GLB metres -> UE centimetres; Blender Y reflected by importer (same as R7 chunks)'})
(OUT / 'service_spaces_manifest.json').write_text(json.dumps(REPORT, indent=2), encoding='utf8')
print('S1_EXPORT_PASS objects=%d triangles=%d built=%d skipped=%d' % (len(objs), len(joined.data.loop_triangles), len(REPORT['built']), len(REPORT['skipped'])))
