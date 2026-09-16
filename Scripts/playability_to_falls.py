"""Convert unguarded drops from a playability survey into the fall list the S2 guardrail builder consumes.

python Scripts/playability_to_falls.py <survey folder> [out.json] [--per-cluster 3] [--exclude-band 08_bridge_roof]
Writes a traversal-report-shaped JSON ({"results":[{"type":"edge","falls":[{"dir","from"}]}]}) so
build_ship_r7_guardrails.py can close every reachable edge the survey found, not only edges next to old route points.
Only axis directions are exported (the builder follows edges along X or Y); diagonal-only drops are reported.
"""
import sys, json, argparse, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import analyze_ship_playability as A

AXIS = {0: 'fwd', 1: 'stbd', 4: 'aft', 5: 'port'}  # survey dir bit -> traversal edge name (UE +X fwd, +Y stbd)
# A rounded edge (the stem, a corner) only drops diagonally: push both of its axis components and let the builder keep
# whichever one actually finds the deck edge.
DIAGONAL = {2: (0, 1), 3: (4, 1), 6: (4, 5), 7: (0, 5)}

ap = argparse.ArgumentParser()
ap.add_argument('folder'); ap.add_argument('out', nargs='?'); ap.add_argument('--per-cluster', type=int, default=3)
ap.add_argument('--exclude-band', action='append', default=[])
a = ap.parse_args()
folder = Path(a.folder)
cells, meta = A.load(folder)
adj, _ = A.build_graph(cells, meta, 0)
seeds = []
for t in meta['targets']:
    if t['kind'] == 'spawn':
        sx, sy, sz = t['local']
        near = [c for c in cells if c['stand'] & 1 and abs(c['x'] - sx) <= 40 and abs(c['y'] - sy) <= 40 and abs(c['z'] - sz) <= 60]
        if near: seeds.append(min(near, key=lambda c: (c['x'] - sx) ** 2 + (c['y'] - sy) ** 2)['id'])
reach = A.flood(adj, seeds)

# Group drop cells by direction and by 2 m / level buckets, keep the two ends and the middle of each run.
buckets = collections.defaultdict(list)
diagonal_only = 0
for c in cells:
    if c['id'] not in reach or not c['drop'] or A.band_of(c['z']) in a.exclude_band:
        continue
    dirs = [d for d in AXIS if c['drop'] & (1 << d)]
    if not dirs:
        diagonal_only += 1
        dirs = sorted({a for d, pair in DIAGONAL.items() if c['drop'] & (1 << d) for a in pair})
        if not dirs:
            continue
    for d in dirs:
        key = (d, round(c['z'] / 50), round(c['x'] / 200) if d in (1, 5) else round(c['x'] / 25), round(c['y'] / 25) if d in (1, 5) else round(c['y'] / 200))
        buckets[key].append(c)
results = []
for (d, _, _, _), group in buckets.items():
    along = (lambda c: c['x']) if d in (1, 5) else (lambda c: c['y'])
    group.sort(key=along)
    picks = [group[0], group[len(group) // 2], group[-1]][:a.per_cluster]
    # Start 15 cm back from the edge cell (grid columns can sit exactly on the plate edge, where the builder's floor ray
    # finds nothing and skips the fall).
    bx, by = {0: (-15, 0), 1: (0, -15), 4: (15, 0), 5: (0, 15)}[d]
    falls = [{'dir': AXIS[d], 'from': [c['x'] + bx, c['y'] + by, c['z']]} for c in {id(p): p for p in picks}.values()]
    results.append({'type': 'edge', 'point': falls[0]['from'], 'pass': False, 'falls': falls})
out = Path(a.out) if a.out else folder / 'survey-falls.json'
out.write_text(json.dumps({'source': folder.name, 'results': results}, indent=1), encoding='utf-8')
print('SURVEY_FALLS buckets=%d falls=%d diagonal_only_cells=%d -> %s' % (len(results), sum(len(r['falls']) for r in results), diagonal_only, out))
