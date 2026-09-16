"""R7 guardrails S2: close the unguarded edges found by the in-game traversal audit (UCalmyTraversalProbe).

blender -b SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend --python Scripts/build_ship_r7_guardrails.py -- <report.json>[,<report.json>...] [out_dir] [--doors doors.json]

Opens the hash-pinned R7 master read-only. For every recorded fall (start point + push direction, UE cm ship-local)
it ray-casts the real deck to find the edge the capsule went over, follows that edge sideways while there is floor on
one side and a drop on the other, and places a standard guardrail (posts, 1.05 m top rail, knee rail, toe board) on it.
Edges that already have a barrier, stair tops (gradual floor) and door openings (a wall face across the edge line)
are skipped. Exports one GLB (SM_R7_81_GUARDRAILS) + guardrails_manifest.json. The master is never saved.
"""
import bpy, bmesh, hashlib, json, math, sys
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
MASTER = ROOT / 'SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend'
HASH = '8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93'
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
# Hand-checked short runs for corners the edge follower cannot reach (Scripts/playability_patch_rails.py builds the list
# from the clusters a survey still reports). They pass the same floor and door-swing checks as everything else.
PATCHES = []
if '--patch' in ARGS:
    i = ARGS.index('--patch'); PATCHES = json.load(open(ARGS[i + 1], encoding='utf-8'))['runs']; del ARGS[i:i + 2]
DOORS = []
if '--doors' in ARGS:
    i = ARGS.index('--doors'); DOORS = json.load(open(ARGS[i + 1], encoding='utf-8')); del ARGS[i:i + 2]
assert ARGS, 'usage: -- <report.json>[,<report.json>...] [out_dir] [--doors doors.json]'
# Falls from several audits are merged: after a partial install the newest report only lists the edges still open.
REPORTS = [json.load(open(p, encoding='utf-8')) for p in ARGS[0].split(',')]
REPORT = {'results': [r for rep in REPORTS for r in rep['results']]}
OUT = Path(ARGS[1]) if len(ARGS) > 1 else ROOT / 'SourceAssets/Generated/ShipR7Guardrails'
# Door hinges (UE cm) -> Blender m. A rail inside a leaf's swing stops the heavy door half open across the passage.
# Door leaves swept over their real arc (UE frame): hinge, yaw, swing sign, open angle and the door's own leaf width
# (R7 doorways are 1.27 m jamb to jamb; assuming a stock 1.12 m leaf put rails inside the closed leaf).
LEAF_W, LEAF_CLEAR, FASCIA = 112.0, 16.0, .18
LEAVES = [(d['x'], d['y'], d['z'], d['yaw'], d['sign'], d['open_angle'], d.get('width', LEAF_W)) for d in DOORS]
assert Path(bpy.data.filepath).resolve() == MASTER.resolve(), 'Open the R7 master read-only'
assert hashlib.sha256(MASTER.read_bytes()).hexdigest() == HASH, 'R7 master hash changed'
OUT.mkdir(parents=True, exist_ok=True)

scene = bpy.context.scene
deps = bpy.context.evaluated_depsgraph_get()
COL = bpy.data.collections.new('81_GUARDRAILS'); scene.collection.children.link(COL)

# UE push directions -> Blender (Y reflected on import).
DIRS = {'fwd': Vector((1, 0, 0)), 'aft': Vector((-1, 0, 0)), 'stbd': Vector((0, -1, 0)), 'port': Vector((0, 1, 0))}
RAIL_H, KNEE_H, TOE_H, POST_STEP, INSET = 1.05, .55, .1, 1.2, .05

def floor_z(p, near_z, reach=.35):
    """Top of the walkable surface under p within +-reach of near_z, or None."""
    ok, loc, normal, *_ = scene.ray_cast(deps, Vector((p.x, p.y, near_z + .6)), Vector((0, 0, -1)), distance=.6 + reach)
    if ok and normal.z > .7 and abs(loc.z - near_z) <= reach:
        return loc.z
    return None

def blocked(p, z, direction, length):
    """A wall/rail across the path at knee or chest height."""
    for h in (.45, .95):
        ok, *_ = scene.ray_cast(deps, Vector((p.x, p.y, z + h)), direction, distance=length)
        if ok:
            return True
    return False

def find_edge(start, direction, z):
    """Walk from the start point until the floor under the capsule disappears; return the last floor point."""
    last = None
    for i in range(0, 80):
        q = start + direction * (i * .05)
        if blocked(q - direction * .05, z, direction, .1):
            return None  # something already stops the player
        fz = floor_z(q, z)
        if fz is None:
            return last
        last = Vector((q.x, q.y, fz))
    return None

def follow(edge, direction, z):
    """Extend the edge sideways while floor stays on the near side and a drop on the far side."""
    side = Vector((-direction.y, direction.x, 0))
    ends = []
    for sgn in (1, -1):
        reach, misses = 0., 0
        s = .1
        while s < 14 and misses < 3:
            e = edge + side * (sgn * s)
            near = floor_z(e - direction * .15, z)
            far = floor_z(e + direction * .2, z, reach=.6)
            wall = blocked(e - direction * .3, z, direction, .35) or blocked(e, z, side * sgn, .12)
            if near is not None and far is None and not wall:
                reach, misses = s, 0
            else:
                misses += 1
            s += .1
        ends.append(edge + side * (sgn * reach))
    return ends[1], ends[0]

def mat(name, rgba):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = rgba
    bsdf.inputs['Roughness'].default_value = .6
    return m

RAIL = next((m for m in bpy.data.materials if 'rail' in m.name.lower()), None) or mat('MAT_S2_SafetyRail', (.85, .62, .08, 1))

def box(name, center, size, yaw):
    me = bpy.data.meshes.new('S2_' + name); bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new('S2_' + name, me); o.data.materials.append(RAIL); COL.objects.link(o)
    o.location = center; o.scale = size; o.rotation_euler = (0, 0, yaw)
    return o

def clear_of_doors(p, z):
    """True when the rail point (Blender m) keeps LEAF_CLEAR cm from every door leaf at every angle it can reach."""
    ux, uy = p.x * 100, -p.y * 100
    for hx, hy, hz, yaw, sign, open_angle, width in LEAVES:
        if abs(z * 100 - hz) > 100 or math.hypot(ux - hx, uy - hy) > width + LEAF_CLEAR + 10:
            continue
        a = 0.
        while a <= open_angle + .01:
            t = math.radians(yaw + a * sign)
            ex, ey = -math.cos(t), -math.sin(t)  # leaf runs along local -X from the hinge
            along = max(0., min(width, (ux - hx) * ex + (uy - hy) * ey))
            if math.hypot(ux - (hx + ex * along), uy - (hy + ey * along)) < LEAF_CLEAR:
                return False
            a += 5.
    return True

def door_safe_spans(a, b, z):
    """Split a->b into the pieces that stay outside every door swing on this deck."""
    run = b - a; n = max(1, int(run.length / .05))
    spans, cur = [], None
    for i in range(n + 1):
        p = a + run * (i / n)
        if clear_of_doors(p, z):
            cur = [p, p] if cur is None else [cur[0], p]
        elif cur is not None:
            spans.append(cur); cur = None
    if cur is not None: spans.append(cur)
    return spans

RUNS = []

def fill_joints():
    """Close the gaps left between collinear rail runs (a player fell through the joints between them).

    Runs stop where `follow` loses the edge or where a door swing cuts them, so two runs on the same line can end 20-80 cm
    apart. Only such joints are filled: the gap must be short, lie on the same line and level, keep deck under it and stay
    out of every door's swing. Free ends are never extended - doing that railed off open deck and broke walking routes.
    """
    groups = {}
    for z, direction, a, b in RUNS:
        along_x = abs((b - a).x) >= abs((b - a).y)
        key = (round(z, 2), direction, along_x, round(a.y if along_x else a.x, 2))
        groups.setdefault(key, []).append((a, b))
    filled = 0.
    for (z, direction, along_x, _), runs in groups.items():
        pick = (lambda v: v.x) if along_x else (lambda v: v.y)
        ordered = sorted(((min(pick(a), pick(b)), max(pick(a), pick(b)), a, b) for a, b in runs))
        for (lo0, hi0, a0, b0), (lo1, hi1, a1, b1) in zip(ordered, ordered[1:]):
            gap = lo1 - hi0
            if not (.05 < gap <= .9):
                continue
            end0 = a0 if abs(pick(a0) - hi0) < 1e-6 else b0
            start1 = a1 if abs(pick(a1) - lo1) < 1e-6 else b1
            steps = max(2, int(gap / .05))
            if any(floor_z(end0 + (start1 - end0) * (k / steps), z) is None or not clear_of_doors(end0 + (start1 - end0) * (k / steps), z)
                   for k in range(steps + 1)):
                continue
            filled += rail('RailJoint_%d' % len(RUNS), end0, start1, z, DIRS[direction])
    return filled

def rail(name, a, b, z, direction):
    """Posts + top rail + knee rail + toe board along a->b, inset onto the deck side of the edge."""
    inset = -direction * INSET
    a, b = a + inset, b + inset
    run = (b - a); length = run.length
    if length < .4:
        return 0
    yaw = math.atan2(run.y, run.x); mid = (a + b) * .5
    box(name + '_Top', Vector((mid.x, mid.y, z + RAIL_H)), (length + .05, .05, .05), yaw)
    box(name + '_Knee', Vector((mid.x, mid.y, z + KNEE_H)), (length, .03, .03), yaw)
    box(name + '_Toe', Vector((mid.x, mid.y, z + TOE_H * .5)), (length, .015, TOE_H), yaw)
    posts = max(2, int(math.ceil(length / POST_STEP)) + 1)
    for i in range(posts):
        p = a + run * (i / (posts - 1))
        box(f'{name}_Post{i}', Vector((p.x, p.y, z + RAIL_H * .5)), (.05, .05, RAIL_H), yaw)
    return length

segments, skipped = [], []
for item in REPORT['results']:
    if item.get('type') != 'edge':
        continue
    for fall in item.get('falls', []):
        fx, fy, fz = fall['from']
        start = Vector((fx / 100, -fy / 100, 0)); z = fz / 100
        direction = DIRS[fall['dir']]
        fz_real = floor_z(start, z)
        if fz_real is None:
            skipped.append({'fall': fall, 'why': 'no floor at start'}); continue
        edge = find_edge(start, direction, fz_real)
        if edge is None:
            skipped.append({'fall': fall, 'why': 'edge not found or already guarded'}); continue
        a, b = follow(edge, direction, fz_real)
        key = (round(edge.z, 1), round(min(a.x, b.x), 0), round(min(a.y, b.y), 0), round(max(a.x, b.x), 0), round(max(a.y, b.y), 0), fall['dir'])
        if any(s['key'] == list(key) for s in segments):
            continue
        segments.append({'key': list(key), 'from_fall': fall, 'a': [round(v, 3) for v in a], 'b': [round(v, 3) for v in b],
                         'z': round(fz_real, 3), 'dir': fall['dir']})

built = 0.
for i, seg in enumerate(segments):
    spans = door_safe_spans(Vector(seg['a']), Vector(seg['b']), seg['z'])
    seg['door_safe_spans'] = [[[round(v, 3) for v in s0], [round(v, 3) for v in s1]] for s0, s1 in spans]
    for j, (s0, s1) in enumerate(spans):
        built += rail(f'Rail{i}_{j}', s0, s1, seg['z'], DIRS[seg['dir']])
        RUNS.append((seg['z'], seg['dir'], s0.copy(), s1.copy()))
    # Where a leaf parks along the edge (90 degree doors), carry the rail on brackets just over the drop instead.
    d = DIRS[seg['dir']]; a0, b0 = Vector(seg['a']), Vector(seg['b'])
    shifted = door_safe_spans(a0 + d * FASCIA, b0 + d * FASCIA, seg['z'])
    kept = [s for s in shifted if all(((s[0] + s[1]) * .5 - d * FASCIA - (k[0] + k[1]) * .5).length > .05 for k in spans)]
    fascia = []
    for s0, s1 in kept:
        # Only the parts that the deck-edge rail had to leave out.
        run = s1 - s0; n = max(1, int(run.length / .05)); cur = None
        for k in range(n + 1):
            q = s0 + run * (k / n); back = q - d * FASCIA
            if not clear_of_doors(back, seg['z']):
                cur = [q, q] if cur is None else [cur[0], q]
            elif cur is not None:
                fascia.append(cur); cur = None
        if cur is not None: fascia.append(cur)
    seg['fascia_spans'] = [[[round(v, 3) for v in f0], [round(v, 3) for v in f1]] for f0, f1 in fascia]
    for j, (f0, f1) in enumerate(fascia):
        # Extend 10 cm each way so the fascia run overlaps the deck-edge runs (no gap at the jog).
        ext = (f1 - f0).normalized() * .1 if (f1 - f0).length > 0 else Vector((0, 0, 0))
        built += rail(f'Rail{i}_F{j}', f0 - ext + d * INSET, f1 + ext + d * INSET, seg['z'], DIRS[seg['dir']])
        for k, q in enumerate((f0 - ext, f1 + ext)):
            box(f'Rail{i}_F{j}_Bracket{k}', Vector((q.x - d.x * FASCIA * .5, q.y - d.y * FASCIA * .5, seg['z'] - .03)),
                (FASCIA + .06, .05, .05), math.atan2(d.y, d.x))

built += fill_joints()

# Corner patches: same checks, explicit geometry.
patched = 0
for k, run in enumerate(PATCHES):
    a = Vector((run['a'][0], run['a'][1], 0)); b = Vector((run['b'][0], run['b'][1], 0)); z = run['z']
    steps = max(2, int((b - a).length / .05))
    if any(floor_z(a + (b - a) * (t / steps), z) is None or not clear_of_doors(a + (b - a) * (t / steps), z) for t in range(steps + 1)):
        skipped.append({'fall': run, 'why': 'patch blocked by a door swing or has no floor'})
        continue
    length = rail('RailPatch%d' % k, a, b, z, DIRS[run['dir']])
    built += length
    patched += 1 if length else 0

objs = list(COL.objects)
assert objs, 'no guardrail segments were built'
# Re-evaluate after creating the rails: the depsgraph taken for ray casts predates them, and exporting from it left
# every object at an identity matrix (all rails collapsed into a unit cube at the origin).
bpy.context.view_layer.update()
deps = bpy.context.evaluated_depsgraph_get()
copies = []
for o in objs:
    data = bpy.data.meshes.new_from_object(o.evaluated_get(deps), depsgraph=deps)
    c = bpy.data.objects.new('export_' + o.name, data); scene.collection.objects.link(c); c.matrix_world = o.matrix_world.copy(); copies.append(c)
bpy.ops.object.select_all(action='DESELECT')
for c in copies: c.select_set(True)
bpy.context.view_layer.objects.active = copies[0]; bpy.ops.object.join(); joined = copies[0]
joined.name = 'SM_R7_81_GUARDRAILS'
scene.cursor.location = (0, 0, 0); bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
glb = OUT / 'SM_R7_81_GUARDRAILS.glb'
joined.data.calc_loop_triangles()
span = [max(v.co[i] for v in joined.data.vertices) - min(v.co[i] for v in joined.data.vertices) for i in range(3)]
assert max(span) > 5.0, 'guardrail geometry collapsed: span %s' % span
bpy.ops.export_scene.gltf(filepath=str(glb), export_format='GLB', use_selection=True, export_apply=True)
manifest = {'master_sha256': HASH, 'report': str(ARGS[0]), 'segments': segments, 'skipped': skipped,
            'rail_length_m': round(built, 2), 'glb': str(glb), 'built': segments, 'lights': []}
(OUT / 'guardrails_manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf8')
print('S2_GUARDRAILS_PASS segments=%d skipped=%d rail_m=%.1f glb=%s' % (len(segments), len(skipped), built, glb))
