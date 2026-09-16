"""Merge fall lists for the S2 guardrail builder and drop falls known to produce a misplaced rail.

python Scripts/combine_guardrail_falls.py <out.json> <report.json> [<report.json> ...]
Keeps every edge fall of the historical traversal reports (their 19 reviewed segments) plus the survey falls, except the
bridge-roof pushes from X -35 m toward the sides: they found the edge of the wheelhouse top at |Y| 6.6 m and railed off
the walkable roof strip (|Y| 6.6-9 m) where the exterior stairs arrive. The strip's real outer edge comes from the survey.
"""
import sys, json
from pathlib import Path

out, sources = Path(sys.argv[1]), sys.argv[2:]
results, dropped = [], 0
for src in sources:
    rep = json.load(open(src, encoding='utf-8'))
    for item in rep['results']:
        if item.get('type') != 'edge' or not item.get('falls'):
            continue
        keep = []
        for f in item['falls']:
            x, y, z = f['from']
            if z > 2150 and f['dir'] in ('port', 'stbd') and abs(y) < 700 and -3600 < x < -3400:
                dropped += 1
                continue
            keep.append(f)
        if keep:
            results.append({'type': 'edge', 'point': item.get('point', keep[0]['from']), 'pass': False, 'falls': keep})
out.write_text(json.dumps({'source': [Path(s).parent.name for s in sources], 'results': results}, indent=1), encoding='utf-8')
print('COMBINED_FALLS edges=%d falls=%d dropped_roof_inner=%d -> %s' % (len(results), sum(len(r['falls']) for r in results), dropped, out))
