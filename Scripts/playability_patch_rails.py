"""REJECTED 2026-09-16 (round L): the generated corner runs closed 1 of 15 clusters, broke 4 historical walks (287/332)
and blocked the hold 1 stair carry path (playthrough dropped the crate). Kept to explain the attempt; do not use.

Turn the drop clusters a survey still reports into explicit short rail runs for build_ship_r7_guardrails.py --patch.

python Scripts/playability_patch_rails.py <survey folder> [out.json] [--margin 40] [--max-cells 24]
The edge follower cannot rail a corner it cannot walk along, so these leftovers (a few cells each) are handed to the
builder as geometry. Direction comes from the cluster's own shape: a run along its longer axis, on the side the fall
happens, extended by a margin at both ends. The builder still refuses any run without deck under it or inside a door swing.
"""
import sys, json, argparse
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument('folder'); ap.add_argument('out', nargs='?')
ap.add_argument('--margin', type=float, default=40.0, help='cm added at both ends')
ap.add_argument('--max-cells', type=int, default=24, help='skip clusters larger than this: they are a job for the follower')
a = ap.parse_args()
folder = Path(a.folder)
report = json.load(open(folder / 'playability-report.json', encoding='utf-8'))
runs = []
for c in report['unguarded_drop_clusters']:
    if c['cells'] > a.max_cells:
        continue
    x0, x1 = c['x_range']; y0, y1 = c['y_range']; cx, cy, cz = c['center']
    along_x = (x1 - x0) >= (y1 - y0)
    if along_x:
        ax, bx = x0 - a.margin, x1 + a.margin
        ay = by = cy
        direction = 'stbd' if cy < 0 else 'port'   # push direction the fall used: outboard of the cluster
    else:
        ay, by = y0 - a.margin, y1 + a.margin
        ax = bx = cx
        direction = 'fwd' if cx > 0 else 'aft'
    runs.append({'z': round(cz / 100, 3), 'dir': direction,
                 'a': [round(ax / 100, 3), round(-ay / 100, 3)], 'b': [round(bx / 100, 3), round(-by / 100, 3)],
                 'from_cluster': {'center': c['center'], 'cells': c['cells']}})
out = Path(a.out) if a.out else folder / 'patch-rails.json'
out.write_text(json.dumps({'source': folder.name, 'runs': runs}, indent=1), encoding='utf-8')
print('PATCH_RAILS runs=%d (of %d clusters) -> %s' % (len(runs), len(report['unguarded_drop_clusters']), out))
