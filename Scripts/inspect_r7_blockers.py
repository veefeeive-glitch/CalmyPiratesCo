"""Read-only: list R7 master objects around traversal failure points (UE cm, ship-local) to classify blockers.

blender -b SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend --python Scripts/inspect_r7_blockers.py -- out.json
UE cm -> Blender m: X/100, -Y/100, Z/100 (the importer reflects Y).
"""
import bpy, json, sys
from mathutils import Vector

POINTS = {
    'forepeak_stuck_a': (5095, 0, 231),
    'forepeak_stuck_b': (4730, 0, 224),
    'stern_weather_inboard_ps': (-5704, 76, 1062),
    'stern_weather_inboard_sb': (-5704, -76, 1062),
    'stern_weather_outboard_ps': (-5630, 634, 1061),
    'stern_weather_outboard_sb': (-5630, -634, 1061),
    'exit_lane_7_9': (-3244, -764, 1069),
    'cabin1_corner': (-5355, 210, 1344),
    'mess_corner': (-5428, 646, 1069),
}
RADIUS = 1.1
ARGS = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
out = ARGS[0] if ARGS else 'r7-blockers.json'
# Optional second argument: JSON {name: [x, y, z]} in UE cm replaces the built-in failure points.
if len(ARGS) > 1:
    POINTS = {k: tuple(v) for k, v in json.load(open(ARGS[1], encoding='utf-8')).items()}
deps = bpy.context.evaluated_depsgraph_get()
boxes = []
for o in bpy.data.objects:
    if o.type != 'MESH' or not o.visible_get():
        continue
    corners = [o.matrix_world @ Vector(c) for c in o.bound_box]
    lo = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
    hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
    coll = o.users_collection[0].name if o.users_collection else ''
    boxes.append((o.name, coll, lo, hi))

report = {}
for key, (x, y, z) in POINTS.items():
    p = Vector((x / 100, -y / 100, z / 100 + .9))  # capsule centre height above the floor point
    hits = []
    for name, coll, lo, hi in boxes:
        d = Vector((max(lo.x - p.x, 0, p.x - hi.x), max(lo.y - p.y, 0, p.y - hi.y), max(lo.z - p.z, 0, p.z - hi.z))).length
        size = (hi - lo)
        # Skip whole-deck plates and hull shells: they contain every point.
        if d <= RADIUS and max(size.x, size.y) < 30:
            hits.append({'object': name, 'collection': coll, 'distance_m': round(d, 2),
                         'min': [round(v, 2) for v in lo], 'max': [round(v, 2) for v in hi]})
    hits.sort(key=lambda h: h['distance_m'])
    report[key] = {'blender_point_m': [round(v, 2) for v in p], 'objects': hits[:18]}
    print('==', key, report[key]['blender_point_m'])
    for h in hits[:12]:
        print('   %.2f  %-45s %-28s %s..%s' % (h['distance_m'], h['object'][:45], h['collection'][:28], h['min'], h['max']))
json.dump(report, open(out, 'w', encoding='utf-8'), indent=2)
print('R7_BLOCKERS_DONE', out)
