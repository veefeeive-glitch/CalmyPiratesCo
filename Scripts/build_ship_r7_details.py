"""R7 details S3: deck equipment from the MPP reference (Docs/Current/SHIP_REFERENCE_DETAILING.md, items 1, 6, 13, 14).

Runs headless on the hash-pinned master opened read-only; the master .blend is never saved. Everything new goes into
collection 82_DETAILS and is exported as one GLB (SourceAssets/Generated/ShipR7Details/SM_R7_82_DETAILS.glb) that
Scripts/install_ship_r7_details.py places on OceanFreighterR7 attached to ShipMotion_R7.

Adds:
- bunker stations SB/PS on the outboard face of the hold 1 coaming (manifold, valves, blind flanges, gauge, hose rack,
  SOPEP locker, drip tray, signs) - clear of the side lane the traversal routes use at |Y| 8.6 m;
- four mushroom vent heads for holds 1 and 2 on the cross deck between the hatches;
- bridge roof: S-band radar on its own pedestal (array above head height), VSAT and Inmarsat-C domes, GMDSS whips,
  magnetic compass binnacle, two liferaft canisters per side in cradles;
- radar mast: searchlights on the lower platform, ship's whistle, anemometer;
- hatch coamings of holds 1 and 2 (item 11): coaming stays every 3 m and quick-acting cleats every 1.5 m on the
  outboard faces, kept inside |Y| 7.8 m and clear of hold entrances, signs, vents and the bunker stations;
- accommodation ladders SB/PS (item 15) stowed on the crew-deck wall under the lifeboats, above head height;
- service rooms (items 2, 9): CO2 room on the engine-room tween flat (bottle bank, manifold, distribution valves,
  release cabinet outside), paint store on the bow tween and deck store on the bow lower level, each lit
  (lights listed in the manifest and spawned by the installer) and clear of the routes and door swings.

Every placed group checks its volume against the master, the S1 service spaces and the S2 guardrails, and needs a
deck under it; a blocked group is skipped and reported instead of being built inside existing geometry.

blender -b <master.blend> --python Scripts/build_ship_r7_details.py -- [out_dir]
"""
import bpy, bmesh, hashlib, json, math, re, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
HASH = '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
OUT = Path(ARGS[0]) if ARGS else ROOT / 'SourceAssets/Generated/ShipR7Details'
assert hashlib.sha256(MASTER.read_bytes()).hexdigest() == HASH, 'Master changed; review before adding details'
assert Path(bpy.data.filepath).resolve() == MASTER.resolve(), 'Open the R7 master read-only'
OUT.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
COL = bpy.data.collections.new('82_DETAILS')
scene.collection.children.link(COL)

# ------------------------------------------------------------------ occupancy: master + S1 service spaces + S2 guardrails
def world_bbox(o):
    pts = [o.matrix_world @ Vector(c) for c in o.bound_box]
    return (min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts),
            max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts))

EXISTING = [(o.name, world_bbox(o)) for o in scene.objects
            if o.type in ('MESH', 'CURVE', 'FONT') and not o.hide_render
            and not any(c.hide_render for c in o.users_collection)]
# Hold entrances cut through the coaming side (R3_HoldWeatherBridge1/2 are the deck-level bridges in the opening):
# the whole opening up to above the coaming top is a passage, not a wall to hang stays or cleats on.
for name, (a, b, c, d, e, f) in list(EXISTING):
    if name.startswith('R3_HoldWeatherBridge'):
        EXISTING.append(('HoldEntrance_' + name[-1], (a - .1, b - .3, c, d + .1, e + .3, 12.6)))
S1 = json.loads((ROOT / 'SourceAssets/Generated/ShipR7ServiceSpaces/service_spaces_manifest.json').read_text(encoding='utf8'))
for item in S1['built']:
    EXISTING.append(('S1_' + item['space'], tuple(item['volume'])))
S2 = json.loads((ROOT / 'SourceAssets/Generated/ShipR7Guardrails/guardrails_manifest.json').read_text(encoding='utf8'))
for seg in S2['segments']:
    for a, b in seg['door_safe_spans'] + seg.get('fascia_spans', []):
        EXISTING.append(('S2_rail_%s_%.2f' % (seg['dir'], seg['z']), (min(a[0], b[0]) - .12, min(a[1], b[1]) - .12, seg['z'],
                                                                     max(a[0], b[0]) + .12, max(a[1], b[1]) + .12, seg['z'] + 1.1)))

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

REPORT = {'built': [], 'skipped': []}
def space(name, volume, needs_floor_z=None):
    hits = blockers(*volume)
    floor = floor_under(volume[0], volume[1], volume[3], volume[4], needs_floor_z) if needs_floor_z is not None else 'n/a'
    if hits or floor is None:
        REPORT['skipped'].append({'group': name, 'volume': [round(v, 2) for v in volume], 'blockers': hits[:12], 'floor': floor})
        print('S3_SKIP', name, 'blockers=', hits[:8], 'floor=', floor)
        return False
    REPORT['built'].append({'group': name, 'volume': [round(v, 2) for v in volume], 'floor': floor})
    EXISTING.append(('S3_' + name, tuple(volume)))  # later groups must keep clear of earlier ones
    print('S3_BUILD', name, 'floor=', floor)
    return True

# ------------------------------------------------------------------ materials (reuse master materials where they exist)
def pick(pattern, fallback):
    for m in bpy.data.materials:
        if re.search(pattern, m.name, re.I): return m
    m = bpy.data.materials.new('MAT_S3_' + re.sub(r'\W', '_', pattern)); m.use_nodes = True
    m.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value = fallback
    return m

M = {
    'steel': pick(r'^MAT_Steel$|Steel', (.45, .47, .5, 1)),
    'dark': pick(r'^MAT_Dark$|Dark', (.06, .07, .08, 1)),
    'white': pick(r'^MAT_White$|White|Paint_White', (.86, .87, .86, 1)),
    'orange': pick(r'Orange', (.9, .38, .08, 1)),
    'red': pick(r'^MAT_Red$|Red', (.7, .06, .05, 1)),
    'wood': pick(r'Wood', (.45, .3, .18, 1)),
}
lamp = bpy.data.materials.new('MAT_S3_lamp_white'); lamp.use_nodes = True
bsdf = lamp.node_tree.nodes.get('Principled BSDF'); bsdf.inputs['Base Color'].default_value = (1, 1, .9, 1)
bsdf.inputs['Emission Color'].default_value = (1, 1, .9, 1); bsdf.inputs['Emission Strength'].default_value = 6.0
M['lamp_white'] = lamp
for key, color in {'mark_white': (.95, .95, .92, 1), 'sign_dark': (.04, .06, .08, 1), 'hose': (.03, .03, .03, 1),
                   'pipe_brown': (.28, .16, .08, 1), 'lens': (.85, .9, .95, 1)}.items():
    m = bpy.data.materials.new('MAT_S3_' + key); m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF'); bsdf.inputs['Base Color'].default_value = color
    bsdf.inputs['Roughness'].default_value = .2 if key == 'lens' else .75
    bsdf.inputs['Metallic'].default_value = .6 if key == 'lens' else 0.0
    M[key] = m

# ------------------------------------------------------------------ primitives
def link_object(name, me, material, loc=(0, 0, 0), rot=(0, 0, 0), scale=(1, 1, 1)):
    o = bpy.data.objects.new('S3_' + name, me); o.data.materials.append(M[material]); COL.objects.link(o)
    o.location = loc; o.rotation_euler = rot; o.scale = scale
    return o

def box(name, loc, dims, m='steel', rot=(0, 0, 0)):
    c = [(-1, -1, -1), (-1, -1, 1), (-1, 1, -1), (-1, 1, 1), (1, -1, -1), (1, -1, 1), (1, 1, -1), (1, 1, 1)]
    v = [tuple(p[i] * dims[i] * .5 for i in range(3)) for p in c]
    f = [(0, 2, 6, 4), (1, 5, 7, 3), (0, 4, 5, 1), (2, 3, 7, 6), (0, 1, 3, 2), (4, 6, 7, 5)]
    me = bpy.data.meshes.new('S3_' + name); me.from_pydata(v, [], f); me.update()
    return link_object(name, me, m, loc, rot)

def cyl(name, loc, r, h, m='steel', rot=(0, 0, 0), n=18):
    v = [(r * math.cos(i * math.tau / n), r * math.sin(i * math.tau / n), z) for z in (-h / 2, h / 2) for i in range(n)]
    f = [(i, (i + 1) % n, (i + 1) % n + n, i + n) for i in range(n)] + [tuple(reversed(range(n))), tuple(range(n, 2 * n))]
    me = bpy.data.meshes.new('S3_' + name); me.from_pydata(v, [], f); me.update()
    return link_object(name, me, m, loc, rot)

def rod(name, a, b, r=.025, m='steel', n=12):
    a, b = Vector(a), Vector(b); d = b - a
    o = cyl(name, (a + b) * .5, r, d.length, m, n=n); o.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return o

def dome(name, loc, r, m='white', squash=1.0):
    """Upper half of a UV sphere, open at the base (always sits on a plate or skirt)."""
    bm = bmesh.new(); bmesh.ops.create_uvsphere(bm, u_segments=24, v_segments=12, radius=r)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -1e-4], context='VERTS')
    me = bpy.data.meshes.new('S3_' + name); bm.to_mesh(me); bm.free()
    return link_object(name, me, m, loc, (0, 0, 0), (1, 1, squash))

def text(name, body, loc, size, m='mark_white', rot=(math.pi / 2, 0, 0), extrude=.004):
    cu = bpy.data.curves.new('S3_' + name, 'FONT'); cu.body = body; cu.size = size; cu.align_x = 'CENTER'; cu.extrude = extrude
    o = bpy.data.objects.new('S3_' + name, cu); o.data.materials.append(M[m]); o.location = loc; o.rotation_euler = rot
    COL.objects.link(o); return o

def side_sign(name, body, x, y_face, s, z, size, width):
    """Dark plate + white letters on a wall whose outward normal is s*Y (readable from that side, as S1 signs are)."""
    box(name + '_Plate', (x, y_face + s * .012, z), (width, .02, size * 1.9), 'sign_dark')
    text(name, body, (x, y_face + s * .026, z - size * .38), size, 'mark_white', (math.pi / 2, 0, math.pi if s > 0 else 0))

ALONG_X = (0, math.pi / 2, 0)   # cylinder axis -> X
ALONG_Y = (math.pi / 2, 0, 0)   # cylinder axis -> Y
Z_WD, Z_RF = 10.58, 22.0        # weather deck, bridge roof

# ------------------------------------------------------------------ 1. Bunker stations on the hold 1 coaming (item 1)
# The coaming side runs X -20..10 with its outboard face at |Y| 7.53. The side lane of the traversal routes is at
# |Y| 8.6 (capsule edge 8.26), so nothing here reaches beyond |Y| 8.15. The hold 1 stair sign (X -15.0..-13.6) stays clear.
BX0, BX1 = -19.3, -16.1
for side, s in (('SB', 1), ('PS', -1)):
    def Y(d): return s * (7.53 + d)
    t = 'Bunker' + side
    if not space('bunker_station_' + side, (BX0, min(Y(.02), Y(.62)), Z_WD + .02, BX1, max(Y(.02), Y(.62)), Z_WD + 1.75), Z_WD):
        continue
    box(t + '_TrayBase', ((BX0 + BX1) / 2, Y(.3), Z_WD + .015), (BX1 - BX0, .56, .03), 'dark')
    for k, (cx, cy, sx, sy) in enumerate((((BX0 + BX1) / 2, Y(.035), BX1 - BX0, .03), ((BX0 + BX1) / 2, Y(.575), BX1 - BX0, .03),
                                          (BX0 + .015, Y(.3), .03, .54), (BX1 - .015, Y(.3), .03, .54))):
        box(f'{t}_TrayLip{k}', (cx, cy, Z_WD + .07), (sx, sy, .08), 'orange')
    zp = Z_WD + 1.0
    rod(t + '_Header', (-18.95, Y(.22), zp), (-16.65, Y(.22), zp), .075, 'pipe_brown')
    cyl(t + '_HeaderCap', (-16.63, Y(.22), zp), .095, .05, 'pipe_brown', ALONG_X)
    cyl(t + '_Elbow', (-18.95, Y(.22), zp), .095, .19, 'pipe_brown')
    rod(t + '_DownPipe', (-18.95, Y(.22), zp), (-18.95, Y(.22), Z_WD + .03), .075, 'pipe_brown')
    cyl(t + '_DeckPenetration', (-18.95, Y(.22), Z_WD + .05), .14, .06, 'dark')
    for k, x in enumerate((-18.55, -17.65)):
        box(f'{t}_ValveBody{k}', (x, Y(.22), zp), (.24, .22, .22), 'red')
        rod(f'{t}_ValveStem{k}', (x, Y(.22), zp + .11), (x, Y(.22), zp + .42), .02)
        cyl(f'{t}_Handwheel{k}', (x, Y(.22), zp + .43), .16, .03, 'red')
        rod(f'{t}_Spoke{k}', (x - .15, Y(.22), zp + .43), (x + .15, Y(.22), zp + .43), .012, 'red')
        xb = x + .3
        rod(f'{t}_Branch{k}', (xb, Y(.22), zp), (xb, Y(.52), zp), .065, 'pipe_brown')
        cyl(f'{t}_Flange{k}', (xb, Y(.53), zp), .15, .04, 'steel', ALONG_Y)
        cyl(f'{t}_BlindCap{k}', (xb, Y(.57), zp), .1, .06, 'red', ALONG_Y)
    for k, x in enumerate((-18.9, -16.9)):
        box(f'{t}_Bracket{k}', (x, Y(.11), zp - .12), (.06, .22, .06), 'dark')
        box(f'{t}_BracketPlate{k}', (x, Y(.012), zp - .05), (.14, .02, .3), 'dark')
    rod(t + '_GaugeStem', (-17.1, Y(.22), zp), (-17.1, Y(.22), zp + .22), .015)
    cyl(t + '_Gauge', (-17.1, Y(.25), zp + .28), .065, .04, 'dark', ALONG_Y)
    cyl(t + '_GaugeDial', (-17.1, Y(.275), zp + .28), .055, .006, 'mark_white', ALONG_Y)
    box(t + '_HoseRack', (-16.45, Y(.06), zp + .1), (.5, .12, .5), 'dark')
    cyl(t + '_HoseCoil', (-16.45, Y(.23), zp + .1), .27, .2, 'hose', ALONG_Y)
    cyl(t + '_HoseCoilHole', (-16.45, Y(.34), zp + .1), .12, .02, 'dark', ALONG_Y)
    box(t + '_SopepLocker', (-16.88, Y(.21), Z_WD + .31), (.42, .3, .56), 'red')
    side_sign(t + '_Sign', 'BUNKER STATION', -18.0, Y(0), s, Z_WD + .63, .1, 1.5)
    side_sign(t + '_Grade', 'MGO  /  HFO', -18.0, Y(0), s, Z_WD + .4, .07, .8)
    side_sign(t + '_Sopep', 'SOPEP', -16.88, Y(.36), s, Z_WD + .46, .06, .34)

# ------------------------------------------------------------------ 2. Hold vent heads between the hatches (item 6)
# Cross deck X 10..14 (coaming ends at 10.18 / 13.82), crane pedestal |Y| <= 0.92, fire stations at |Y| 7.3.
for hold, x in (('1', 10.62), ('2', 13.38)):
    for side, s in (('SB', 1), ('PS', -1)):
        y = s * 4.2; t = f'HoldVent{hold}{side}'
        if not space(t, (x - .42, y - .42, Z_WD + .02, x + .42, y + .42, Z_WD + 1.5), Z_WD):
            continue
        cyl(t + '_Flange', (x, y, Z_WD + .025), .3, .05, 'dark')
        cyl(t + '_Trunk', (x, y, Z_WD + .5), .2, .9, 'white')
        cyl(t + '_Skirt', (x, y, Z_WD + 1.02), .38, .14, 'white')
        dome(t + '_Cap', (x, y, Z_WD + 1.09), .36, 'white', .45)
        rod(t + '_Spindle', (x, y, Z_WD + 1.2), (x, y, Z_WD + 1.36), .02)
        cyl(t + '_Handwheel', (x, y, Z_WD + 1.37), .13, .025, 'red')
        cyl(t + '_Band', (x, y, Z_WD + .78), .205, .08, 'dark')
        side_sign(t + '_Label', 'No.' + hold + ' HOLD', x, y + s * .2, s, Z_WD + .56, .055, .34)

# ------------------------------------------------------------------ 3. Bridge roof kit (items 13, 14)
# Roof slab X -49.5..-31.5, |Y| <= 6.6, S2 rails at |Y| 6.55-6.65. The only roof routes are at X >= -35 (tower stairs),
# so everything stays aft of X -40. Anything a player could walk under is above 2.2 m.
for side, s in (('SB', 1), ('PS', -1)):
    yc = s * 5.85; t = 'Liferaft' + side
    if not space(t, (-49.1, min(s * 5.45, s * 6.25), Z_RF + .02, -45.7, max(s * 5.45, s * 6.25), Z_RF + .95), Z_RF):
        continue
    for k, dy in enumerate((-.22, .22)):
        rod(f'{t}_Rail{k}', (-49.0, yc + dy, Z_RF + .06), (-45.8, yc + dy, Z_RF + .06), .03)
        for kx, x in enumerate((-48.9, -47.4, -45.9)):
            box(f'{t}_Foot{k}{kx}', (x, yc + dy, Z_RF + .025), (.1, .1, .05), 'dark')
    for k, xc in enumerate((-48.2, -46.6)):
        for dx in (-.4, .4):
            box(f'{t}_Saddle{k}{dx:+.1f}', (xc + dx, yc, Z_RF + .15), (.08, .56, .14), 'steel')
        cyl(f'{t}_Canister{k}', (xc, yc, Z_RF + .5), .3, 1.2, 'white', ALONG_X)
        for dx in (-.35, .35):
            cyl(f'{t}_Band{k}{dx:+.2f}', (xc + dx, yc, Z_RF + .5), .305, .05, 'orange', ALONG_X)
        for dx in (-.15, .15):
            cyl(f'{t}_Strap{k}{dx:+.2f}', (xc + dx, yc, Z_RF + .5), .31, .03, 'dark', ALONG_X)
        box(f'{t}_Hydrostatic{k}', (xc, yc - s * .36, Z_RF + .2), (.1, .08, .14), 'red')
        rod(f'{t}_Painter{k}', (xc, yc - s * .36, Z_RF + .13), (xc + .3, yc - s * .36, Z_RF + .04), .008, 'hose')

if space('sband_radar', (-48.1, -2.1, Z_RF + .02, -47.1, 2.1, Z_RF + 2.7), Z_RF):
    box('SBand_Base', (-47.6, 0, Z_RF + .05), (.7, .7, .1), 'dark')
    box('SBand_Column', (-47.6, 0, Z_RF + .95), (.4, .4, 1.7), 'white')
    for k in range(6):
        rod(f'SBand_Rung{k}', (-47.39, -.15, Z_RF + .35 + k * .28), (-47.39, .15, Z_RF + .35 + k * .28), .012)
    box('SBand_Gearbox', (-47.6, 0, Z_RF + 1.93), (.56, .42, .26), 'white')
    rod('SBand_Drive', (-47.6, 0, Z_RF + 2.06), (-47.6, 0, Z_RF + 2.22), .07, 'dark')
    box('SBand_Array', (-47.6, 0, Z_RF + 2.38), (.22, 3.9, .3), 'dark')
    box('SBand_ArrayLip', (-47.48, 0, Z_RF + 2.3), (.04, 3.8, .1), 'steel')

for side, s, body_r, pole_h, name in (('PS', -1, .55, 1.3, 'VSAT'), ('SB', 1, .15, .9, 'InmarsatC')):
    x, y = -44.8, s * 3.2
    if not space(name, (x - .65, y - .65, Z_RF + .02, x + .65, y + .65, Z_RF + pole_h + body_r + .3), Z_RF):
        continue
    box(name + '_Base', (x, y, Z_RF + .04), (.4, .4, .08), 'dark')
    rod(name + '_Pole', (x, y, Z_RF + .08), (x, y, Z_RF + pole_h), .09 if body_r > .3 else .04)
    cyl(name + '_Ring', (x, y, Z_RF + pole_h + .03), body_r * 1.02, .06, 'white')
    dome(name + '_Dome', (x, y, Z_RF + pole_h + .06), body_r, 'white', 1.15)

for side, s in (('SB', 1), ('PS', -1)):
    x, y = -43.2, s * 5.6; t = 'GMDSSWhip' + side
    if space(t, (x - .12, y - .12, Z_RF + .02, x + .12, y + .12, Z_RF + 3.8), Z_RF):
        cyl(t + '_Base', (x, y, Z_RF + .14), .06, .28, 'dark')
        rod(t + '_Whip', (x, y, Z_RF + .28), (x, y, Z_RF + 3.7), .018, 'white')

if space('compass_binnacle', (-41.0, -.4, Z_RF + .02, -40.2, .4, Z_RF + 1.6), Z_RF):
    cyl('Binnacle_Foot', (-40.6, 0, Z_RF + .04), .3, .08, 'dark')
    cyl('Binnacle_Column', (-40.6, 0, Z_RF + .6), .2, 1.05, 'white')
    cyl('Binnacle_Head', (-40.6, 0, Z_RF + 1.2), .28, .22, 'white')
    dome('Binnacle_Hood', (-40.6, 0, Z_RF + 1.31), .27, 'lens', .7)
    for s in (-1, 1):
        dome(f'Binnacle_Corrector{s}', (-40.6, s * .36, Z_RF + 1.12), .1, 'steel')

# ------------------------------------------------------------------ 4. Radar mast attachments (no space check, like S1 nav lights)
# Lower mast platform X -39.08..-37.92, |Y| <= 1.2, top Z 24.31; mast pole |Y| <= 0.24; yard braces at X -38.5.
for side, s in (('SB', 1), ('PS', -1)):
    y = s * .78; t = 'Searchlight' + side
    rod(t + '_Stand', (-38.2, y, 24.31), (-38.2, y, 24.55), .03, 'dark')
    box(t + '_Yoke', (-38.2, y, 24.58), (.08, .36, .04), 'dark')
    cyl(t + '_Body', (-38.12, y, 24.72), .15, .34, 'dark', ALONG_X)
    cyl(t + '_Lens', (-37.94, y, 24.72), .13, .02, 'lens', ALONG_X)
rod('Whistle_Bracket', (-38.26, 0, 25.5), (-38.05, 0, 25.5), .04, 'dark')
rod('Whistle_Horn', (-38.05, 0, 25.5), (-37.7, 0, 25.5), .06)
cyl('Whistle_Bell', (-37.66, 0, 25.5), .15, .1, 'steel', ALONG_X)
rod('Anemometer_Pole', (-38.5, 1.75, 27.22), (-38.5, 1.75, 27.78), .015)
for k in range(3):
    a = k * math.tau / 3; tip = (-38.5 + .16 * math.cos(a), 1.75 + .16 * math.sin(a), 27.8)
    rod(f'Anemometer_Arm{k}', (-38.5, 1.75, 27.8), tip, .006)
    dome(f'Anemometer_Cup{k}', tip, .04, 'dark')

# ------------------------------------------------------------------ 5. Hatch coaming stays and cleats (item 11)
# Coaming sides: hold 1 X -20..10, hold 2 X 14..42, outboard face |Y| 7.53, top Z 11.80; hatch panels |Y| <= 6.95,
# top Z 11.95. Everything outboard stays inside |Y| 7.8 (the side lane capsule edge is 8.26).
Z_CT = 11.8

def prism_yz(name, pts, x, thickness, m):
    """Triangle in the Y-Z plane at X, extruded thickness along X (a bracket web)."""
    v = [(x - thickness / 2, y, z) for y, z in pts] + [(x + thickness / 2, y, z) for y, z in pts]
    f = [(0, 1, 2), (3, 5, 4), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)]
    me = bpy.data.meshes.new('S3_' + name); me.from_pydata(v, [], f); me.update()
    return link_object(name, me, m)

for hold, xa, xb in (('1', -20.0, 10.0), ('2', 14.0, 42.0)):
    for side, s in (('SB', 1), ('PS', -1)):
        face = s * 7.53
        def span(d0, d1): return min(face + s * d0, face + s * d1), max(face + s * d0, face + s * d1)
        x = xa + 1.5
        while x < xb - .4:
            t = f'Coaming{hold}{side}_Stay{x:+.1f}'
            y0, y1 = span(.005, .27)
            if space(t, (x - .07, y0, Z_WD + .02, x + .07, y1, Z_CT - .05)):
                prism_yz(t + '_Web', [(face, Z_WD + .03), (face + s * .25, Z_WD + .03), (face, Z_CT - .1)], x, .025, 'steel')
                box(t + '_Toe', (x, face + s * .13, Z_WD + .045), (.12, .26, .03), 'steel')
                box(t + '_Flat', (x, face + s * .035, (Z_WD + Z_CT) / 2), (.1, .07, Z_CT - Z_WD - .15), 'steel')
            x += 3.0
        x = xa + .75
        while x < xb - .3:
            t = f'Coaming{hold}{side}_Cleat{x:+.2f}'
            y0, y1 = span(.005, .13)
            if space(t, (x - .07, y0, Z_CT - .2, x + .07, y1, Z_CT - .005)):
                box(t + '_Lug', (x, face + s * .05, Z_CT - .09), (.1, .1, .14), 'dark')
                rod(t + '_Hook', (x, face + s * .05, Z_CT - .03), (x, s * 6.99, 11.975), .016, 'dark')
                cyl(t + '_Nut', (x, s * 6.99, 11.985), .032, .035, 'dark')
            x += 1.5

# ------------------------------------------------------------------ 6. Stowed accommodation ladders (item 15)
# Crew-deck outer wall |Y| 7.53..7.67 above the boat-deck wall; lifeboat keels from Z 14.62, davit feet from 14.97;
# the side walkway below is open to Z 12.34 (capsule top), so the ladder hangs at Z 13.3..14.05.
for side, s in (('SB', 1), ('PS', -1)):
    w = s * 7.67; x0, x1 = -54.3, -46.4; t = 'AccomLadder' + side
    if not space(t, (-55.0, min(w + s * .01, w + s * .45), 13.28, -45.5, max(w + s * .01, w + s * .45), 14.1)):
        continue
    yl = w + s * .26
    for k, z in enumerate((13.42, 13.92)):
        box(f'{t}_Stringer{k}', ((x0 + x1) / 2, yl, z), (x1 - x0, .08, .14), 'steel')
    n = int((x1 - x0) / .3)
    for i in range(n + 1):
        xi = x0 + i * (x1 - x0) / n
        rod(f'{t}_Tread{i}', (xi, yl, 13.49), (xi, yl, 13.85), .022)
    rod(t + '_Handrail', (x0, yl + s * .1, 14.0), (x1, yl + s * .1, 14.0), .02, 'white')
    for i in range(7):
        xi = x0 + .4 + i * (x1 - x0 - .8) / 6
        rod(f'{t}_Stanchion{i}', (xi, yl + s * .04, 13.99), (xi + .45, yl + s * .1, 14.0), .015, 'white')
    box(t + '_Platform', (x1 + .45, yl, 13.67), (.8, .1, .62), 'steel')
    for k, x in enumerate((x0 + .5, (x0 + x1) / 2, x1 - .5)):
        box(f'{t}_Bracket{k}', (x, w + s * .13, 13.33), (.1, .26, .06), 'dark')
        box(f'{t}_Clamp{k}', (x, w + s * .13, 14.03), (.1, .26, .05), 'dark')
    box(t + '_WinchBase', (-54.72, w + s * .17, 13.55), (.45, .3, .35), 'dark')
    cyl(t + '_WinchDrum', (-54.72, w + s * .36, 13.62), .1, .32, 'steel', ALONG_X)

# ------------------------------------------------------------------ 7. Service rooms (items 2, 9)
# Enclosed room like S1: walls with door openings (lintel, jambs), ceiling, lamp + manifest light, sign above the door.
# wall in {'aft','fwd','port','stbd'} = x0 / x1 / y0 / y1; door centre is along the wall.
LIGHTS = []
def room(tag, x0, x1, y0, y1, z, h=2.35, t=.08, doors=(), label=None, label_face='aft'):
    door_h = 2.0
    def wall(name, axis, fixed, a, b, openings):
        cuts = sorted([(c - w / 2, c + w / 2) for c, w in openings]); spans, cur = [], a
        for c0, c1 in cuts:
            if c0 > cur: spans.append((cur, c0))
            cur = max(cur, c1)
        if cur < b: spans.append((cur, b))
        for i, (s0, s1) in enumerate(spans):
            if s1 - s0 < .02: continue
            if axis == 'x': box(f'{tag}_{name}{i}', ((s0 + s1) / 2, fixed, z + h / 2), (s1 - s0, t, h), 'white')
            else: box(f'{tag}_{name}{i}', (fixed, (s0 + s1) / 2, z + h / 2), (t, s1 - s0, h), 'white')
        for c, w in openings:
            if axis == 'x':
                box(f'{tag}_{name}Lintel{c:.2f}', (c, fixed, z + door_h + (h - door_h) / 2), (w, t, h - door_h), 'white')
                for s in (-1, 1): box(f'{tag}_{name}Jamb{c:.2f}{s}', (c + s * w / 2, fixed, z + door_h / 2), (.06, t + .04, door_h), 'dark')
            else:
                box(f'{tag}_{name}Lintel{c:.2f}', (fixed, c, z + door_h + (h - door_h) / 2), (t, w, h - door_h), 'white')
                for s in (-1, 1): box(f'{tag}_{name}Jamb{c:.2f}{s}', (fixed, c + s * w / 2, z + door_h / 2), (t + .04, .06, door_h), 'dark')
    op = {k: [(c, w) for wk, c, w in doors if wk == k] for k in ('aft', 'fwd', 'port', 'stbd')}
    wall('Aft', 'y', x0, y0, y1, op['aft']); wall('Fwd', 'y', x1, y0, y1, op['fwd'])
    wall('Port', 'x', y0, x0, x1, op['port']); wall('Stbd', 'x', y1, x0, x1, op['stbd'])
    box(f'{tag}_Ceiling', ((x0 + x1) / 2, (y0 + y1) / 2, z + h + .03), (x1 - x0, y1 - y0, .06), 'white')
    box(f'{tag}_Lamp', ((x0 + x1) / 2, (y0 + y1) / 2, z + h - .04), (.9, .18, .05), 'lamp_white')
    LIGHTS.append({'name': tag, 'blender_m': [round((x0 + x1) / 2, 3), round((y0 + y1) / 2, 3), round(z + h - .25, 3)]})
    if label:
        face = {'aft': (x0 - .05, (y0 + y1) / 2, (math.pi / 2, 0, -math.pi / 2)), 'fwd': (x1 + .05, (y0 + y1) / 2, (math.pi / 2, 0, math.pi / 2)),
                'port': ((x0 + x1) / 2, y0 - .05, (math.pi / 2, 0, 0)), 'stbd': ((x0 + x1) / 2, y1 + .05, (math.pi / 2, 0, math.pi))}[label_face]
        plate = (.02, max(.9, len(label) * .09), .2) if label_face in ('aft', 'fwd') else (max(.9, len(label) * .09), .02, .2)
        box(f'{tag}_SignPlate', (face[0], face[1], z + 2.18), plate, 'sign_dark')
        off = {'aft': (-.02, 0), 'fwd': (.02, 0), 'port': (0, -.02), 'stbd': (0, .02)}[label_face]
        text(f'{tag}_Sign', label, (face[0] + off[0], face[1] + off[1], z + 2.13), .11, 'mark_white', face[2])

Z_TW, Z_LW = 7.33, 4.58   # tween and lower flats

# CO2 room on the engine-room tween flat, starboard of the centre walk (routes >= 0.75 m away). It starts at X -33.2 so
# bulkhead door 15 (hinge X -34.87, leaf 1.27 m, 180 deg) swings clear; the engine-access stair foot at |Y| 5.1 stays open.
CX0, CX1, CY0, CY1 = -33.2, -28.6, 1.3, 4.3
if space('co2_room', (CX0, CY0, Z_TW + .05, CX1, CY1, Z_TW + 2.45), Z_TW):
    room('CO2Room', CX0, CX1, CY0, CY1, Z_TW, doors=[('port', -30.9, 1.0)], label='CO2 ROOM', label_face='port')
    bottles = []
    for row, y in enumerate((3.95, 3.63)):
        for i in range(13):
            x = CX0 + .3 + i * .3 + (.15 if row else 0)
            if x > CX1 - .9: continue
            t = f'CO2Room_Bottle{row}_{i}'
            cyl(t, (x, y, Z_TW + .85), .135, 1.6, 'red')
            dome(t + '_Shoulder', (x, y, Z_TW + 1.65), .135, 'red', .6)
            rod(t + '_Valve', (x, y, Z_TW + 1.72), (x, y, Z_TW + 1.84), .03)
            rod(t + '_Hose', (x, y, Z_TW + 1.84), (x, 3.79, Z_TW + 2.0), .012, 'dark')
            bottles.append(x)
    rod('CO2Room_Manifold', (CX0 + .2, 3.79, Z_TW + 2.02), (CX1 - .15, 3.79, Z_TW + 2.02), .05)
    for k, z in enumerate((Z_TW + .45, Z_TW + 1.3)):
        box(f'CO2Room_RackBar{k}', ((CX0 + CX1 - .6) / 2, 3.44, z), (CX1 - CX0 - 1.0, .04, .05), 'dark')
        for kx, x in enumerate((CX0 + .08, CX1 - .78)):
            box(f'CO2Room_RackPost{k}{kx}', (x, 3.8, z), (.05, .75, .05), 'dark')
    for k in range(4):   # distribution valves and lines up through the deckhead, one per protected space
        y = 1.75 + k * .45
        rod(f'CO2Room_Line{k}', (CX1 - .18, y, Z_TW + .95), (CX1 - .18, y, Z_TW + 2.38), .035)
        box(f'CO2Room_DistValve{k}', (CX1 - .18, y, Z_TW + 1.15), (.14, .16, .16), 'red')
        cyl(f'CO2Room_DistWheel{k}', (CX1 - .3, y, Z_TW + 1.15), .09, .02, 'dark', ALONG_X)
    rod('CO2Room_Feeder', (CX1 - .15, 3.79, Z_TW + 2.02), (CX1 - .18, 1.75, Z_TW + 2.02), .045)
    for k, x in enumerate((CX1 - .75, CX1 - .5)):   # pilot bottles by the door
        cyl(f'CO2Room_Pilot{k}', (x, 1.62, Z_TW + .5), .09, .9, 'red')
    box('CO2Room_ReleaseCabinet', (-30.05, CY0 - .12, Z_TW + 1.3), (.45, .16, .6), 'red')
    box('CO2Room_ReleaseGlass', (-30.05, CY0 - .205, Z_TW + 1.35), (.34, .01, .36), 'lens')
    box('CO2Room_AlarmHorn', (-31.6, CY0 - .1, Z_TW + 2.12), (.16, .12, .16), 'red')
    side_sign('CO2Room_Warning', 'DANGER  CO2', -32.0, CY0 - .04, -1, Z_TW + 1.45, .07, .7)

# Paint store and deck store in the bow, centred between the two stairs (|Y| >= 2.3); doors face the collision bulkhead
# (door 28/29 hinge X 52.13 swings into the forepeak, 1.3 m clear of the stores).
SX0, SX1 = 44.6, 49.8
def x_sign(name, body, x_face, y, z, size, width):
    """Sign on a forward-facing (+X) wall, readable from forward."""
    box(name + '_Plate', (x_face + .012, y, z), (.02, width, size * 1.9), 'sign_dark')
    text(name, body, (x_face + .026, y, z - size * .38), size, 'mark_white', (math.pi / 2, 0, math.pi / 2))

if space('paint_store', (SX0, -1.4, Z_TW + .05, SX1, 1.4, Z_TW + 2.45), Z_TW):
    room('PaintStore', SX0, SX1, -1.4, 1.4, Z_TW, doors=[('fwd', 0.0, 1.0)], label='PAINT STORE', label_face='fwd')
    colours = ('red', 'white', 'orange', 'dark', 'white', 'red')
    for s in (-1, 1):
        y = s * 1.08
        for k, x in enumerate((SX0 + .3, (SX0 + SX1) / 2, SX1 - .35)):
            rod(f'PaintStore_Upright{s}{k}', (x, y - s * .22, Z_TW), (x, y - s * .22, Z_TW + 1.85), .02, 'dark')
        for k, z in enumerate((Z_TW + .3, Z_TW + .9, Z_TW + 1.5)):
            box(f'PaintStore_Shelf{s}{k}', ((SX0 + SX1) / 2, y, z), (SX1 - SX0 - .5, .5, .04), 'steel')
            for i in range(14):
                x = SX0 + .45 + i * .31
                if (i + k) % 5 == 4: continue
                cyl(f'PaintStore_Tin{s}{k}{i}', (x, y + (.08 if i % 2 else -.08), z + .13), .1, .22, colours[(i + k + (s > 0)) % 6])
    for k, y in enumerate((-.45, .45)):
        cyl(f'PaintStore_Drum{k}', (SX0 + .42, y, Z_TW + .47), .29, .88, 'dark')
        cyl(f'PaintStore_DrumBand{k}', (SX0 + .42, y, Z_TW + .6), .295, .04, 'orange')
    box('PaintStore_SpillTray', (SX0 + .42, 0, Z_TW + .02), (.75, 1.6, .04), 'orange')
    cyl('PaintStore_Extinguisher', (SX0 + 1.1, -.62, Z_TW + .3), .09, .55, 'red')
    rod('PaintStore_VentDuct', (SX0 + 1.2, 0, Z_TW + 2.2), (SX0 + 1.2, 0, Z_TW + 2.38), .16)
    x_sign('PaintStore_NoSmoking', 'NO SMOKING', SX1 + .04, -.95, Z_TW + 1.5, .07, .8)

if space('deck_store', (SX0, -1.4, Z_LW + .05, SX1, 1.4, Z_LW + 2.45), Z_LW):
    room('DeckStore', SX0, SX1, -1.4, 1.4, Z_LW, doors=[('fwd', 0.0, 1.0)], label='DECK STORE', label_face='fwd')
    for i, (x, y) in enumerate(((45.55, -.76), (46.55, -.76), (47.55, -.76), (45.55, .76), (46.55, .76))):
        box(f'DeckStore_Pallet{i}', (x, y, Z_LW + .06), (.9, .9, .12), 'wood')
        cyl(f'DeckStore_Coil{i}', (x, y, Z_LW + .35), .4, .45, 'orange')
        cyl(f'DeckStore_CoilEye{i}', (x, y, Z_LW + .36), .14, .46, 'dark')
    cyl('DeckStore_WireReel', (48.6, -.8, Z_LW + .52), .42, .55, 'steel', ALONG_Y)
    for s in (-1, 1):
        cyl(f'DeckStore_ReelFlange{s}', (48.6, -.8 + s * .3, Z_LW + .52), .5, .04, 'dark', ALONG_Y)
    box('DeckStore_ReelStand', (48.6, -.8, Z_LW + .05), (.9, .8, .1), 'dark')
    for k, z in enumerate((Z_LW + .5, Z_LW + 1.1, Z_LW + 1.7)):
        box(f'DeckStore_AftShelf{k}', (SX0 + .22, 0, z), (.36, 1.8, .04), 'steel')
        for i in range(4):
            box(f'DeckStore_Bin{k}{i}', (SX0 + .22, -.66 + i * .44, z + .1), (.3, .36, .16), 'orange' if (i + k) % 2 else 'dark')
    for i in range(5):
        cyl(f'DeckStore_Fender{i}', (47.4 + i * .1, .8, Z_LW + .3 + i * .02), .22, .6 - .04 * i, 'dark', ALONG_X)
    x_sign('DeckStore_Notice', 'MOORING ROPES', SX1 + .04, -.95, Z_LW + 1.5, .06, .8)

# ------------------------------------------------------------------ export only the new collection
objs = list(COL.objects)
assert objs, 'nothing built'
# Re-evaluate first: a depsgraph older than the new objects exports them at identity (the S2 unit-cube collapse).
bpy.context.view_layer.update()
deps = bpy.context.evaluated_depsgraph_get()
copies = []
for o in objs:
    data = bpy.data.meshes.new_from_object(o.evaluated_get(deps), depsgraph=deps)
    c = bpy.data.objects.new('export_' + o.name, data); scene.collection.objects.link(c); c.matrix_world = o.matrix_world.copy(); copies.append(c)
bpy.context.view_layer.update()
bpy.ops.object.select_all(action='DESELECT')
for c in copies: c.select_set(True)
bpy.context.view_layer.objects.active = copies[0]; bpy.ops.object.join(); joined = copies[0]
joined.name = 'SM_R7_82_DETAILS'
scene.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
lo, hi = world_bbox(joined)[:3], world_bbox(joined)[3:]
assert hi[0] - lo[0] > 50 and hi[2] - lo[2] > 10, 'exported details collapsed: %s %s' % (lo, hi)
joined.data.calc_loop_triangles()
glb = OUT / 'SM_R7_82_DETAILS.glb'
bpy.ops.export_scene.gltf(filepath=str(glb), export_format='GLB', use_selection=True, export_apply=True)
REPORT.update({'lights': LIGHTS, 'master_sha256': HASH, 'objects': len(objs),'triangles': len(joined.data.loop_triangles), 'glb': str(glb),
               'bounds_m': [[round(v, 2) for v in lo], [round(v, 2) for v in hi]],
               'units': 'GLB metres -> UE centimetres; Blender Y reflected by importer (same as R7 chunks)'})
(OUT / 'details_manifest.json').write_text(json.dumps(REPORT, indent=2), encoding='utf8')
print('S3_EXPORT_PASS objects=%d triangles=%d built=%d skipped=%d' % (len(objs), len(joined.data.loop_triangles), len(REPORT['built']), len(REPORT['skipped'])))
