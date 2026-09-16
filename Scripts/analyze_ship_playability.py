"""Analyze a UCalmyPlayabilityProbe survey: reachability from the player starts, islands, dead-end stairs, unguarded drops,
links closed only by guardrails/details, per-target usability, and top-down deck maps.

python Scripts/analyze_ship_playability.py Saved/Verification/ShipPlayability-<stamp>
Writes playability-report.json, playability-summary.md and maps/<band>.png next to the survey.
"""
import csv, json, sys, collections
from pathlib import Path
from PIL import Image, ImageDraw

DIRS = [(1, 0), (0, 1), (1, 1), (-1, 1), (-1, 0), (0, -1), (-1, -1), (1, -1)]
# Floor bands (ship-local Z, cm) matching R7 levels; a cell belongs to the band that contains its floor.
BANDS = [('01_tank_top', 100, 330), ('02_lower_4.58', 330, 600), ('03_tween_7.33', 600, 900), ('04_weather', 900, 1230),
         ('05_boat_deck', 1230, 1480), ('06_crew_deck', 1480, 1760), ('07_officers_deck', 1760, 2040), ('08_bridge_roof', 2040, 2900)]
SCALE = 2  # px per cell


def load(folder):
    cells = []
    with open(folder / 'cells.csv', newline='') as f:
        for r in csv.DictReader(f):
            cells.append({k: (float(v) if k in ('x', 'y', 'z') else int(v)) for k, v in r.items()})
    meta = json.load(open(folder / 'targets.json', encoding='utf-8'))
    return cells, meta


def build_graph(cells, meta, variant):
    """Adjacency for one variant: 0 full, 1 without guardrails, 2 without details."""
    stand_bit = (1, 2, 4)[variant]
    by_col = collections.defaultdict(list)
    for c in cells:
        by_col[(c['ix'], c['iy'])].append(c['id'])
    adj = collections.defaultdict(set)
    for c in cells:
        if not c['stand'] & stand_bit:
            continue
        mask = c['pass%d' % variant]
        for d in range(4):
            if not mask & (1 << d):
                continue
            dx, dy = DIRS[d]
            best, bdz = None, 51
            for o in by_col.get((c['ix'] + dx, c['iy'] + dy), []):
                dz = abs(cells[o]['z'] - c['z'])
                if dz <= bdz:
                    best, bdz = o, dz
            if best is not None and cells[best]['stand'] & stand_bit:
                adj[c['id']].add(best); adj[best].add(c['id'])
    return adj, by_col


def flood(adj, seeds):
    seen = set(seeds); queue = collections.deque(seeds)
    while queue:
        n = queue.popleft()
        for m in adj.get(n, ()):
            if m not in seen:
                seen.add(m); queue.append(m)
    return seen


def components(cells, adj, allowed):
    comp = {}
    out = []
    for c in cells:
        i = c['id']
        if i in comp or i not in allowed:
            continue
        members = flood(adj, [i])
        for m in members:
            comp[m] = len(out)
        out.append(members)
    return comp, out


def band_of(z):
    for name, lo, hi in BANDS:
        if lo <= z < hi:
            return name
    return BANDS[-1][0]


def bbox(cells, ids):
    xs = [cells[i]['x'] for i in ids]; ys = [cells[i]['y'] for i in ids]; zs = [cells[i]['z'] for i in ids]
    return [round(min(xs)), round(max(xs)), round(min(ys)), round(max(ys)), round(min(zs)), round(max(zs))]


def main(folder):
    folder = Path(folder)
    cells, meta = load(folder)
    grid = meta['grid']
    adj, by_col = build_graph(cells, meta, 0)
    adj_norails, _ = build_graph(cells, meta, 1)
    adj_nodetails, _ = build_graph(cells, meta, 2)
    standable = {c['id'] for c in cells if c['stand'] & 1}
    targets = meta['targets']
    # Seed from the standable floor cell under each player start (target local = capsule foot).
    spawn_cells = []
    for t in targets:
        if t['kind'] != 'spawn':
            continue
        sx, sy, sz = t['local']
        near = [c for c in cells if c['stand'] & 1 and abs(c['x'] - sx) <= 40 and abs(c['y'] - sy) <= 40 and -60 <= c['z'] - sz <= 60]
        if near:
            best = min(near, key=lambda c: (c['x'] - sx) ** 2 + (c['y'] - sy) ** 2 + (c['z'] - sz) ** 2)
            spawn_cells.append(best['id']); t['use_cells'] = [best['id']]
    reach = flood(adj, spawn_cells)
    reach_norails = flood(adj_norails, spawn_cells)
    reach_nodetails = flood(adj_nodetails, spawn_cells)
    comp, comps = components(cells, adj, standable)

    # Islands: walkable regions a player can never get to (with every door open).
    islands = []
    for members in comps:
        if members & reach or len(members) < 16:  # < 1 m2 is noise (tops of furniture, sills)
            continue
        islands.append({'cells': len(members), 'area_m2': round(len(members) * (grid['step'] / 100) ** 2, 1),
                        'band': band_of(sum(cells[i]['z'] for i in members) / len(members)), 'bbox_xxyyzz': bbox(cells, members)})
    islands.sort(key=lambda r: -r['cells'])

    # Unguarded drops from reachable floor.
    drops = []
    for c in cells:
        if c['id'] in reach and c['drop']:
            drops.append(c)
    drop_clusters = cluster_points([(c['x'], c['y'], c['z']) for c in drops], 150)

    # Links closed by guardrails / details between floor that is otherwise walkable.
    blocked_rails = [c for c in cells if (c['pass1'] & ~c['pass0']) & 0xF or (c['stand'] & 2 and not c['stand'] & 1)]
    blocked_details = [c for c in cells if (c['pass2'] & ~c['pass0']) & 0xF or (c['stand'] & 4 and not c['stand'] & 1)]
    only_without_rails = reach_norails - reach
    only_without_details = reach_nodetails - reach

    rows = []
    for t in targets:
        use = [int(i) for i in t['use_cells']]
        ok = [i for i in use if i in reach]
        rows.append({'kind': t['kind'], 'name': t['name'], 'local': t['local'], 'use_cells': len(use), 'reachable_use_cells': len(ok),
                     'usable': bool(ok), 'band': band_of(t['local'][2] - (88 if t['kind'] in ('door', 'crew_helper', 'leak_valve', 'fuel_pump', 'cargo', 'bridge_terminal') else 0))})
    doors = collections.defaultdict(dict)
    for r in rows:
        if r['kind'] == 'door':
            did, side = r['name'].rsplit('_', 1)
            doors[did][side] = r['usable']
    door_issues = {d: s for d, s in doors.items() if not all(s.values())}

    report = {
        'survey': folder.name, 'cells': len(cells), 'standable': len(standable), 'reachable': len(reach),
        'reachable_m2': round(len(reach) * (grid['step'] / 100) ** 2, 1),
        'reachable_by_band_m2': {b: round(sum(1 for i in reach if band_of(cells[i]['z']) == b) * (grid['step'] / 100) ** 2, 1) for b, _, _ in BANDS},
        'islands': islands, 'unguarded_drop_cells': len(drops), 'unguarded_drop_clusters': drop_clusters,
        'cells_reachable_only_without_guardrails': len(only_without_rails), 'cells_reachable_only_without_details': len(only_without_details),
        'guardrail_blocked_links': len(blocked_rails), 'detail_blocked_links': len(blocked_details),
        'guardrail_blocked_clusters': cluster_points([(c['x'], c['y'], c['z']) for c in blocked_rails], 150),
        'detail_blocked_clusters': cluster_points([(c['x'], c['y'], c['z']) for c in blocked_details], 150),
        'targets': rows, 'doors_not_usable_both_sides': door_issues,
    }
    (folder / 'playability-report.json').write_text(json.dumps(report, indent=1), encoding='utf-8')
    render_maps(folder, cells, grid, reach, drops, blocked_rails, blocked_details, rows)
    write_summary(folder, report)
    unusable = [r for r in rows if not r['usable'] and r['kind'] != 'door']
    print('PLAYABILITY reachable_m2=%.0f islands=%d drop_clusters=%d rail_blocked_clusters=%d detail_blocked_clusters=%d unusable_targets=%d doors_one_side=%d' % (
        report['reachable_m2'], len(islands), len(drop_clusters), len(report['guardrail_blocked_clusters']), len(report['detail_blocked_clusters']),
        len(unusable), len(door_issues)))
    for r in rows:
        if r['kind'] != 'door':
            print('  TARGET %-16s %-34s usable=%s use_cells=%d reachable=%d at %s' % (r['kind'], r['name'][:34], r['usable'], r['use_cells'], r['reachable_use_cells'], r['local']))


def cluster_points(points, radius):
    clusters = []
    for p in points:
        for c in clusters:
            if abs(c['z'] - p[2]) < 60 and (c['x'] - p[0]) ** 2 + (c['y'] - p[1]) ** 2 < radius ** 2 * 4:
                c['n'] += 1; c['xs'].append(p[0]); c['ys'].append(p[1])
                break
        else:
            clusters.append({'x': p[0], 'y': p[1], 'z': p[2], 'n': 1, 'xs': [p[0]], 'ys': [p[1]]})
    out = [{'center': [round(sum(c['xs']) / c['n']), round(sum(c['ys']) / c['n']), round(c['z'])], 'cells': c['n'],
            'x_range': [round(min(c['xs'])), round(max(c['xs']))], 'y_range': [round(min(c['ys'])), round(max(c['ys']))]} for c in clusters]
    return sorted(out, key=lambda r: -r['cells'])


def render_maps(folder, cells, grid, reach, drops, blocked_rails, blocked_details, rows):
    maps = folder / 'maps'; maps.mkdir(exist_ok=True)
    nx, ny = grid['nx'], grid['ny']
    drop_ids = {c['id'] for c in drops}
    rail_ids = {c['id'] for c in blocked_rails}
    detail_ids = {c['id'] for c in blocked_details}
    colors = {'kind': {'spawn': (80, 170, 255), 'cargo': (255, 200, 40), 'delivery': (255, 140, 0), 'leak_valve': (0, 230, 255),
                       'fuel_pump': (120, 255, 120), 'crew_helper': (255, 120, 255), 'bridge_terminal': (255, 255, 255)}}
    for name, lo, hi in BANDS:
        img = Image.new('RGB', (nx * SCALE, ny * SCALE), (18, 20, 26))
        draw = ImageDraw.Draw(img)
        for c in cells:
            if not lo <= c['z'] < hi or not c['stand'] & 1:
                continue
            col = (46, 150, 80) if c['id'] in reach else (190, 50, 50)
            if c['id'] in drop_ids: col = (255, 60, 200)
            if c['id'] in rail_ids: col = (255, 230, 0)
            elif c['id'] in detail_ids: col = (255, 150, 40)
            # Bow to the right, port (+Y) up; UE +Y is starboard-reflected in Blender, keep UE axes.
            px, py = c['ix'] * SCALE, (ny - 1 - c['iy']) * SCALE
            draw.rectangle([px, py, px + SCALE - 1, py + SCALE - 1], fill=col)
        for r in rows:
            if r['kind'] == 'door' or not lo <= r['local'][2] - 88 < hi and r['kind'] not in ('spawn', 'delivery'):
                continue
            if r['kind'] in ('spawn', 'delivery') and not lo <= r['local'][2] < hi + 90:
                continue
            ix = (r['local'][0] - grid['min_x']) / grid['step']; iy = (r['local'][1] - grid['min_y']) / grid['step']
            px, py = ix * SCALE, (ny - 1 - iy) * SCALE
            col = colors['kind'].get(r['kind'], (255, 255, 255))
            draw.ellipse([px - 7, py - 7, px + 7, py + 7], outline=col, width=3 if r['usable'] else 1)
            if not r['usable']:
                draw.line([px - 9, py - 9, px + 9, py + 9], fill=(255, 0, 0), width=3); draw.line([px - 9, py + 9, px + 9, py - 9], fill=(255, 0, 0), width=3)
        for r in rows:
            if r['kind'] != 'door' or not lo <= r['local'][2] - 88 < hi:
                continue
            ix = (r['local'][0] - grid['min_x']) / grid['step']; iy = (r['local'][1] - grid['min_y']) / grid['step']
            px, py = ix * SCALE, (ny - 1 - iy) * SCALE
            draw.rectangle([px - 2, py - 2, px + 2, py + 2], fill=(255, 255, 255) if r['usable'] else (255, 0, 0))
        # X ruler every 10 m
        for xm in range(-60, 61, 10):
            px = int((xm * 100 - grid['min_x']) / grid['step'] * SCALE)
            draw.line([px, 0, px, 6], fill=(120, 120, 140)); draw.text((px + 2, 0), '%dm' % xm, fill=(120, 120, 140))
        draw.text((4, ny * SCALE - 14), '%s  green reachable  red unreachable  magenta drop  yellow rail-blocked  orange detail-blocked' % name, fill=(200, 200, 210))
        img = img.resize((img.width, img.height * 2), Image.NEAREST)  # Y is 2.5x shorter than X: stretch for readability
        img.save(maps / (name + '.png'))


def write_summary(folder, r):
    lines = ['# Ship playability survey %s' % r['survey'], '',
             'Reachable floor from player starts (all doors open): **%.0f m2**' % r['reachable_m2'], '']
    lines += ['| Band | Reachable m2 |', '|---|---|'] + ['| %s | %.0f |' % kv for kv in r['reachable_by_band_m2'].items()] + ['']
    lines += ['## Targets', '', '| Kind | Name | Usable | Use cells (reachable) | Local |', '|---|---|---|---|---|']
    for t in r['targets']:
        if t['kind'] != 'door':
            lines.append('| %s | %s | %s | %d (%d) | %s |' % (t['kind'], t['name'], 'yes' if t['usable'] else '**NO**', t['use_cells'], t['reachable_use_cells'], t['local']))
    lines += ['', '## Doors not usable from both sides (%d)' % len(r['doors_not_usable_both_sides']), '']
    lines += ['- %s: %s' % (d, s) for d, s in sorted(r['doors_not_usable_both_sides'].items())]
    lines += ['', '## Islands (walkable, never reachable): %d' % len(r['islands']), '']
    lines += ['- %.1f m2 %s bbox x/y/z %s' % (i['area_m2'], i['band'], i['bbox_xxyyzz']) for i in r['islands'][:40]]
    lines += ['', '## Unguarded drop clusters: %d' % len(r['unguarded_drop_clusters']), '']
    lines += ['- %d cells at %s x %s y %s' % (c['cells'], c['center'], c['x_range'], c['y_range']) for c in r['unguarded_drop_clusters'][:60]]
    lines += ['', '## Guardrail-blocked clusters: %d (cells reachable only without rails: %d)' % (len(r['guardrail_blocked_clusters']), r['cells_reachable_only_without_guardrails']), '']
    lines += ['- %d cells at %s' % (c['cells'], c['center']) for c in r['guardrail_blocked_clusters'][:40]]
    lines += ['', '## Detail-blocked clusters: %d (cells reachable only without details: %d)' % (len(r['detail_blocked_clusters']), r['cells_reachable_only_without_details']), '']
    lines += ['- %d cells at %s' % (c['cells'], c['center']) for c in r['detail_blocked_clusters'][:40]]
    (folder / 'playability-summary.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main(sys.argv[1])
