"""Original stylized fly mesh. Run with Blender --background --python."""
from pathlib import Path
import bpy, math, json
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SourceAssets/Generated/FlyR2'
OUT.mkdir(parents=True, exist_ok=True)
assert not (OUT / 'Fly_R2.blend').exists(), 'Preserve existing authoring candidate'
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def material(name, color, roughness, metallic=0):
    m = bpy.data.materials.new(name); m.diffuse_color = (*color, 1); m.use_nodes = True
    bs = m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = (*color, 1)
    bs.inputs['Roughness'].default_value = roughness
    bs.inputs['Metallic'].default_value = metallic
    return m
body = material('M_Fly_Chitin', (.035,.022,.012), .36)
amber = material('M_Fly_Amber', (.31,.13,.025), .48)
eyes = material('M_Fly_Eyes', (.48,.017,.011), .24)
wing = material('M_Fly_Wings', (.48,.68,.72), .24, .18)
parts=[]
def ellipsoid(name, p, scale, mat, segments=12, rings=6):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, location=p)
    o=bpy.context.object; o.name=name; o.scale=scale; o.data.materials.append(mat)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    for face in o.data.polygons: face.use_smooth=True
    parts.append(o); return o
def rod(name,a,b,r,mat):
    a,b=Vector(a),Vector(b); d=b-a
    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=r, depth=d.length, location=(a+b)/2)
    o=bpy.context.object; o.name=name; o.rotation_euler=d.to_track_quat('Z','Y').to_euler();o.data.materials.append(mat);parts.append(o)

ellipsoid('Thorax',(0,0,0),(.013,.012,.011),body)
ellipsoid('Abdomen',(-.020,0,-.002),(.023,.010,.009),amber)
for x in (-.011,-.019,-.027,-.034):
    radius=.010*math.sqrt(max(.1,1-((x+.020)/.024)**2))
    ellipsoid('Abdominal_band',(x,0,-.002),(.0018,radius+.0004,radius*.90+.0003),body)
ellipsoid('Head',(.015,0,.003),(.010,.010,.009),amber)
for side in (-1,1):
    ellipsoid('Compound_eye',(.020,side*.007,.005),(.007,.005,.007),eyes)
    rod('Antenna',(.024,side*.003,.008),(.031,side*.006,.014),.0008,body)
    for n,x in enumerate((.007,0,-.008)):
        a=(x,side*.006,-.006)
        b=(x+.006*(1-n),side*.017,-.014)
        c=(x+.010*(1-n),side*.022,-.020)
        rod('Leg_femur',a,b,.001,body);rod('Leg_tibia',b,c,.0007,body)
        rod('Leg_tarsus',c,(c[0]+.006,c[1]+side*.004,c[2]-.001),.00045,body)
    w=ellipsoid('Wing',(-.009,side*.026,.010),(.025,.013,.0006),wing,16,6)
    w.rotation_euler.z=side*-.58
    # Red vertex color marks flap amplitude; body vertices are zero.
    color=w.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
    for i,v in enumerate(w.data.vertices):
        strength=min(1,max(0,(side*(v.co.y+w.location.y)-.009)/.031))
        color.data[i].color=(strength,0,0,1)
    rod('Wing_vein',(-.001,side*.010,.011),(-.019,side*.038,.011),.00025,body)

for o in parts:
    if not o.data.color_attributes.get('Color'):
        col=o.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for v in col.data:v.color=(0,0,0,1)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0]
bpy.ops.object.join();fly=bpy.context.object;fly.name='SM_Fly_R2'
bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
bpy.ops.export_scene.gltf(filepath=str(OUT/'SM_Fly_R2.glb'),export_format='GLB',use_selection=True,export_yup=True,export_apply=True)
tris=sum(len(p.vertices)-2 for p in fly.data.polygons)
(OUT/'manifest.json').write_text(json.dumps({'triangles':tris,'meters_bounds':list(fly.dimensions),'author':'Original procedural model authored for Calmy Pirates Co.','forward':'+X','up':'+Z','textures':'none; material slots + vertex color wing animation'},indent=2))

# Macro authoring preview, separate from the game mesh export.
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
scene.render.resolution_x=1000;scene.render.resolution_y=750;scene.render.resolution_percentage=100
scene.world.color=(.12,.12,.12)
bpy.ops.object.camera_add(location=(.105,-.13,.10));cam=bpy.context.object
cam.rotation_euler=(Vector((-.006,0,0))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=.14;scene.camera=cam
for p,energy,size in [((.05,-.05,.1),8,.09),((-.05,.05,.08),5,.07)]:
    bpy.ops.object.light_add(type='AREA',location=p);l=bpy.context.object;l.data.energy=energy;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(-l.location).to_track_quat('-Z','Y').to_euler()
scene.render.film_transparent=False
scene.render.filepath=str(OUT/'Fly_R2_preview.png')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Fly_R2.blend'))
bpy.ops.render.render(write_still=True)
print('FLY_MODEL_PASS triangles='+str(tris))
