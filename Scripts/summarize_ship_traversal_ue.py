"""Summarise Saved/Verification/ShipTraversal-UE-*/traversal-report.json (UCalmyTraversalProbe output).

Groups walk failures by the blocking actor/component and by ship area, and clusters unguarded-edge falls by deck and
position, so fixes target real places instead of individual failed attempts. Coordinates are ship-local UE cm at rest.

usage: python Scripts/summarize_ship_traversal_ue.py [report.json] [--json out.json]
"""
import json, sys, glob, os, collections

DECKS = [(150, 'tank top'), (400, 'lower'), (650, 'tween'), (950, 'weather'), (1200, 'boat'), (1480, 'crew'),
         (1750, 'officers'), (2050, 'bridge'), (99999, 'bridge roof')]

def deck(z):
    for limit, name in DECKS:
        if z < limit: return name
    return 'bridge roof'

def area(x):
    if x < -3100: return 'superstructure (aft)'
    if x < -2000: return 'engine casing / hold 2 aft'
    if x > 4600: return 'forepeak'
    if x > 0: return 'hold 1 / fore deck'
    return 'midships / hold 2'

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    path = args[0] if args else sorted(glob.glob('Saved/Verification/ShipTraversal-UE-*/traversal-report.json'))[-1]
    out = sys.argv[sys.argv.index('--json') + 1] if '--json' in sys.argv else None
    r = json.load(open(path, encoding='utf-8'))
    walks = [x for x in r['results'] if x['type'] == 'walk']
    edges = [x for x in r['results'] if x['type'] == 'edge']
    fails = [w for w in walks if not w['pass']]
    print(f"report: {path}")
    print(f"walks: {r['walks_passed']}/{r['walks_total']}   edge points with falls: {r['edge_points_with_falls']}/{r['edge_points']}")

    by_reason = collections.Counter(w['reason'] for w in fails)
    print('\nwalk failure reasons:', dict(by_reason))
    by_hit = collections.defaultdict(list)
    for w in fails:
        comp = w['hit'].split('/')[-1] if w['hit'] not in ('', 'none') else 'none (no blocker in front)'
        by_hit[comp].append(w)
    print('\nwalk failures by blocking component:')
    for comp, items in sorted(by_hit.items(), key=lambda kv: -len(kv[1])):
        places = collections.Counter((deck(w['local'][2]), area(w['local'][0])) for w in items)
        print(f"  {len(items):3d}  {comp}   " + ', '.join(f"{d}/{a} x{n}" for (d, a), n in places.most_common(4)))

    # Cluster stuck points within 1.5 m on the same deck: one physical defect often fails many routes.
    clusters = []
    for w in fails:
        p = w['local']
        for c in clusters:
            q = c['center']
            if abs(p[2] - q[2]) < 150 and ((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2) ** .5 < 150:
                c['walks'].append(w['name']); c['hits'].add(w['hit']); break
        else:
            clusters.append({'center': p, 'walks': [w['name']], 'hits': {w['hit']}})
    clusters.sort(key=lambda c: -len(c['walks']))
    print(f"\nstuck/fall locations ({len(clusters)} clusters):")
    for c in clusters[:40]:
        x, y, z = c['center']
        print(f"  {len(c['walks']):3d} walks at ({x:.0f},{y:.0f},{z:.0f}) {deck(z)} / {area(x)}  hit={sorted(c['hits'])[:2]}  e.g. {c['walks'][0]}")

    falls = []
    for e in edges:
        for f in e.get('falls', []):
            falls.append(f)
    fall_clusters = []
    for f in falls:
        p = f['from']
        for c in fall_clusters:
            q = c['center']
            if abs(p[2] - q[2]) < 150 and ((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2) ** .5 < 250:
                c['falls'].append(f); break
        else:
            fall_clusters.append({'center': p, 'falls': [f]})
    fall_clusters.sort(key=lambda c: -len(c['falls']))
    print(f"\nunguarded edges ({len(falls)} falls in {len(fall_clusters)} places):")
    for c in fall_clusters[:40]:
        x, y, z = c['center']
        dirs = collections.Counter(f['dir'] for f in c['falls'])
        drop = max(f['drop_cm'] for f in c['falls'])
        print(f"  {len(c['falls']):3d} at ({x:.0f},{y:.0f},{z:.0f}) {deck(z)} / {area(x)}  dirs={dict(dirs)}  max_drop={drop:.0f}cm")

    if out:
        json.dump({'report': path, 'walk_clusters': [{**c, 'hits': sorted(c['hits'])} for c in clusters],
                   'edge_clusters': fall_clusters}, open(out, 'w', encoding='utf-8'), indent=2)
        print('\nwrote', out)

if __name__ == '__main__':
    main()
