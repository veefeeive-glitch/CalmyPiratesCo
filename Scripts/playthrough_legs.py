"""Legs for the R7 playthrough probe from a playability survey: spawn -> leak valve -> standard cargo -> delivery -> fuel.

python Scripts/playthrough_legs.py <survey folder> [out.json]
Every leg starts where the previous one ended, follows the shortest surveyed floor path and ends on a cell from which the
target is usable (range + line of sight). Delivery ends 1.2 m aft of the zone centre on the side lane, facing forward, so
the carried crate is set down inside the zone.
"""
import sys, json, math, collections
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent))
import analyze_ship_playability as A
from playability_routes import bfs_tree, path_to, simplify

folder = Path(sys.argv[1])
out = Path(sys.argv[2]) if len(sys.argv) > 2 else folder / 'playthrough-legs.json'
cells, meta = A.load(folder)
adj, _ = A.build_graph(cells, meta, 0)
targets = {t['name']: t for t in meta['targets']}
spawn = next(t for t in meta['targets'] if t['kind'] == 'spawn' and t['local'][2] < 1300)
sx, sy, sz = spawn['local']
start = min((c for c in cells if c['stand'] & 1 and abs(c['z'] - sz) < 60), key=lambda c: (c['x'] - sx) ** 2 + (c['y'] - sy) ** 2)['id']


def leg(name, src, goals, prefer=None):
    tree = bfs_tree(adj, src)
    goals = [g for g in goals if g in tree]
    assert goals, 'no reachable goal for ' + name
    if prefer:
        goal = min(goals, key=prefer)
    else:
        goal = min(goals, key=lambda g: len(path_to(tree, g)))
    ids = path_to(tree, goal)
    return {'name': name, 'points': simplify(cells, ids, 250.0)}, goal


def near_cells(p, radius, dz=60):
    return [c['id'] for c in cells if c['stand'] & 1 and abs(c['z'] - p[2]) <= dz and math.hypot(c['x'] - p[0], c['y'] - p[1]) <= radius]


legs = []
leak = next(t for t in meta['targets'] if t['kind'] == 'leak_valve')
lx, ly, lz = leak['local']
# Stand 90-140 cm in front of the valve: in repair range, not pressed into the bulkhead.
l1, at = leg('to_leak_valve', start, [int(i) for i in leak['use_cells']],
             prefer=lambda g: abs(math.hypot(cells[g]['x'] - lx, cells[g]['y'] - ly) - 115))
legs.append(l1)
cargo = [t for t in meta['targets'] if t['kind'] == 'cargo']
l2, at2 = leg('to_cargo', at, sorted({int(i) for t in cargo for i in t['use_cells']}),
              prefer=None)
legs.append(l2)
delivery = next(t for t in meta['targets'] if t['kind'] == 'delivery')
dx, dy, dz = delivery['local']
aim = (dx - 120, dy - 40, dz - 72)
l3, at3 = leg('to_delivery', at2, near_cells(aim, 60, 90), prefer=lambda g: math.hypot(cells[g]['x'] - aim[0], cells[g]['y'] - aim[1]))
legs.append(l3)
fuel = next(t for t in meta['targets'] if t['kind'] == 'fuel_pump')
fx, fy, fz = fuel['local']
l4, _ = leg('to_fuel_station', at3, [int(i) for i in fuel['use_cells']], prefer=lambda g: math.hypot(cells[g]['x'] - fx, cells[g]['y'] - fy))
legs.append(l4)
out.write_text(json.dumps({'source': folder.name, 'legs': legs,
                           'targets': {'leak_valve': leak['local'], 'delivery': delivery['local'], 'fuel_pump': fuel['local']}}, indent=1), encoding='utf-8')
print('PLAYTHROUGH_LEGS %s -> %s' % ([(l['name'], len(l['points'])) for l in legs], out))
