"""Camera shots for UCalmyPhotoTourProbe from a playability report: the worst unguarded drops, rail/detail-blocked
spots, every non-door target and the hold/engine stair heads, each seen from reachable floor at eye height.

python Scripts/playability_shots.py <survey folder> [out.json] [--drops 30] [--blocked 20]
"""
import sys, json, math, argparse
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import analyze_ship_playability as A

ap = argparse.ArgumentParser()
ap.add_argument('folder'); ap.add_argument('out', nargs='?'); ap.add_argument('--drops', type=int, default=30); ap.add_argument('--blocked', type=int, default=20)
ap.add_argument('--doors', action='store_true', help='also frame every door face, closed')
a = ap.parse_args()
folder = Path(a.folder)
cells, meta = A.load(folder)
report = json.load(open(folder / 'playability-report.json', encoding='utf-8'))
reach_ids = None
adj, _ = A.build_graph(cells, meta, 0)
seeds = []
for t in meta['targets']:
    if t['kind'] == 'spawn':
        sx, sy, sz = t['local']
        near = [c for c in cells if c['stand'] & 1 and abs(c['x'] - sx) <= 40 and abs(c['y'] - sy) <= 40 and abs(c['z'] - sz) <= 60]
        if near: seeds.append(min(near, key=lambda c: (c['x'] - sx) ** 2 + (c['y'] - sy) ** 2)['id'])
reach = A.flood(adj, seeds)
reach_cells = [cells[i] for i in reach]


def viewpoint(target, min_d=220, max_d=520):
    """A reachable floor cell 2.2-5.2 m from the target on the same level, preferring the most open view."""
    tx, ty, tz = target
    best = None
    for c in reach_cells:
        if abs(c['z'] - tz) > 120:
            continue
        d = math.hypot(c['x'] - tx, c['y'] - ty)
        if min_d <= d <= max_d:
            score = abs(d - 350)
            if best is None or score < best[0]:
                best = (score, c)
    if best is None:
        return None
    c = best[1]
    return [c['x'], c['y'], c['z'] + 165]


shots = []
def add(name, target, fov=70):
    eye = viewpoint(target)
    if eye:
        shots.append({'name': name, 'eye': eye, 'target': [target[0], target[1], target[2] + 60], 'fov': fov, 'doors': 'open'})
    else:
        print('NO_VIEW', name, target)

for i, c in enumerate(report['unguarded_drop_clusters'][:a.drops]):
    add('drop_%02d_%d_%d_%d' % (i + 1, *c['center']), c['center'])
for i, c in enumerate(report['guardrail_blocked_clusters'][:a.blocked]):
    add('railblock_%02d_%d_%d_%d' % (i + 1, *c['center']), c['center'])
for i, c in enumerate(report['detail_blocked_clusters'][:a.blocked]):
    add('detailblock_%02d_%d_%d_%d' % (i + 1, *c['center']), c['center'])
use_by_name = {t['name']: [int(i) for i in t['use_cells']] for t in meta['targets']}
for t in report['targets']:
    if t['kind'] in ('door', 'spawn'):
        continue
    x, y, z = t['local']
    floor = z - (0 if t['kind'] == 'delivery' else 90)
    # Stand where the player actually uses the target (the survey checked range and line of sight from these cells),
    # as far back as that allows, so the frame never shows the far side of a bulkhead.
    use = [cells[i] for i in use_by_name.get(t['name'], []) if i in reach]
    if use and t['kind'] != 'delivery':
        c = max(use, key=lambda c: math.hypot(c['x'] - x, c['y'] - y))
        shots.append({'name': 'target_%s_%s' % (t['kind'], t['name']), 'eye': [c['x'], c['y'], c['z'] + 165],
                      'target': [x, y, z], 'fov': 70, 'doors': 'open'})
    else:
        add('target_%s_%s' % (t['kind'], t['name']), [x, y, floor], 75)
if a.doors:
    # Each door face from reachable floor straight out along the door normal (same room, so the frame is in view),
    # 1.5-3.2 m back, eye 1.6 m: shows leaf, jambs, head and sill against the bulkhead.
    sides = {t['name']: t for t in meta['targets'] if t['kind'] == 'door'}
    for name, t in sorted(sides.items()):
        did, face = name.rsplit('_', 1)
        other = sides.get(did + '_' + ('b' if face == 'a' else 'a'))
        if not other:
            continue
        px, py, pz = t['local']; ox, oy, _ = other['local']
        nx, ny = px - ox, py - oy; n = math.hypot(nx, ny) or 1; nx, ny = nx / n, ny / n
        floor = pz - 88
        best = None
        for c in reach_cells:
            if abs(c['z'] - floor) > 40:
                continue
            along = (c['x'] - px) * nx + (c['y'] - py) * ny
            off = abs((c['x'] - px) * -ny + (c['y'] - py) * nx)
            if 70 <= along <= 240 and off <= 30:
                score = -along + off * 2
                if best is None or score < best[0]:
                    best = (score, c)
        if best:
            c = best[1]
            mid = ((px + ox) / 2, (py + oy) / 2)
            shots.append({'name': '%s_%s_closed' % (did, face), 'eye': [c['x'], c['y'], c['z'] + 160],
                          'target': [mid[0], mid[1], floor + 105], 'fov': 65, 'doors': 'closed'})
        else:
            print('NO_DOOR_VIEW', name)
out = Path(a.out) if a.out else folder / 'review-shots.json'
out.write_text(json.dumps({'shots': shots}, indent=1), encoding='utf-8')
print('SHOTS %d -> %s' % (len(shots), out))
