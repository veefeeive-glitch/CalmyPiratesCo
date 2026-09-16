"""Turn a playability survey into walker routes: shortest floor path from the first weather-deck player start to every
usable gameplay target (and from each cargo to the delivery zone), for UCalmyTraversalProbe (-CalmyTraversalRoutes=).

python Scripts/playability_routes.py <survey folder> [out.json]
A route that the survey says exists but a real CharacterMovement walker cannot finish is a survey error or a snag.
"""
import sys, json, math, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import analyze_ship_playability as A


def bfs_tree(adj, start):
    parent = {start: None}; q = collections.deque([start])
    while q:
        n = q.popleft()
        for m in adj.get(n, ()):
            if m not in parent:
                parent[m] = n; q.append(m)
    return parent


def path_to(parent, goal):
    out = []
    while goal is not None:
        out.append(goal); goal = parent[goal]
    return out[::-1]


def simplify(cells, ids, max_gap=300.0):
    """Keep turns, height changes and a point at least every 3 m so the walker follows the surveyed corridor."""
    pts = [(cells[i]['x'], cells[i]['y'], cells[i]['z']) for i in ids]
    keep = [pts[0]]
    for k in range(1, len(pts) - 1):
        a, b, c = keep[-1], pts[k], pts[k + 1]
        d1 = (b[0] - a[0], b[1] - a[1]); d2 = (c[0] - b[0], c[1] - b[1])
        turn = d1 != (0, 0) and d2 != (0, 0) and abs(math.atan2(d1[1], d1[0]) - math.atan2(d2[1], d2[0])) > .35
        climb = abs(c[2] - a[2]) > 40 and abs(b[2] - a[2]) > 40
        far = math.hypot(b[0] - a[0], b[1] - a[1]) > max_gap
        if turn or climb or far:
            keep.append(b)
    keep.append(pts[-1])
    return [[round(p[0]), round(p[1]), round(p[2])] for p in keep]


def main(folder, out):
    folder = Path(folder)
    cells, meta = A.load(folder)
    adj, _ = A.build_graph(cells, meta, 0)
    targets = meta['targets']
    starts = [t for t in targets if t['kind'] == 'spawn' and t['local'][2] < 1300]
    sx, sy, sz = starts[0]['local']
    start = min((c for c in cells if c['stand'] & 1 and abs(c['z'] - sz) < 60), key=lambda c: (c['x'] - sx) ** 2 + (c['y'] - sy) ** 2)['id']
    tree = bfs_tree(adj, start)
    routes = []
    def add(name, src_tree, goal_cells):
        goals = [int(i) for i in goal_cells if int(i) in src_tree]
        if not goals:
            print('ROUTE_MISSING', name); return None
        best = min(goals, key=lambda g: len(path_to(src_tree, g)))
        ids = path_to(src_tree, best)
        routes.append({'name': name, 'points': simplify(cells, ids)})
        return best
    for t in targets:
        if t['kind'] in ('spawn', 'door'):
            continue
        add('spawn_to_%s_%s' % (t['kind'], t['name']), tree, t['use_cells'])
    delivery = [t for t in targets if t['kind'] == 'delivery']
    for t in targets:
        if t['kind'] != 'cargo' or not delivery:
            continue
        reachable = [int(i) for i in t['use_cells'] if int(i) in tree]
        if not reachable:
            continue
        pick = min(reachable, key=lambda g: len(path_to(tree, g)))
        add('%s_to_delivery' % t['name'], bfs_tree(adj, pick), delivery[0]['use_cells'])
    Path(out).write_text(json.dumps({'source': folder.name, 'routes': routes}, indent=1), encoding='utf-8')
    print('ROUTES %d -> %s' % (len(routes), out))
    for r in routes:
        print('  %-60s points=%d' % (r['name'][:60], len(r['points'])))


if __name__ == '__main__':
    f = sys.argv[1]
    main(f, sys.argv[2] if len(sys.argv) > 2 else str(Path(f) / 'walker-routes.json'))
