"""Original modular deckhand, authored in an isolated Blender background process.
Centimetre design, metre Blender scene; articulated origins exported independently.
Does not open or modify any ship file. No external assets.
"""
from pathlib import Path
import bpy, math
from mathutils import Vector
OUT=Path(__file__).resolve().parents[1]/'SourceAssets/Generated/Deckhand'
OUT.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
colors={'Oilskin':(.94,.47,.025),'Navy':(.025,.075,.12),'Rubber':(.018,.026,.034),
        'Skin':(.65,.34,.19),'Reflective':(.85,.91,.79),'White':(.93,.95,.89),
        'Hair':(.085,.033,.012),'Lens':(.013,.08,.11),'Red':(.65,.025,.012)}
M={}
for n,c in colors.items():
    m=bpy.data.materials.new('Deckhand_'+n);m.diffuse_color=(*c,1);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*c,1)
    bs.inputs['Roughness'].default_value=.42 if n in ('Oilskin','Rubber') else .65
    M[n]=m
parts=[]
def shape(n,p,d,mat,roundness=.2,sphere=False):
    if sphere:bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=Vector(p)*.01)
    else:bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)*.01)
    o=bpy.context.object;o.name=n;o.dimensions=Vector(d)*.01
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if not sphere:
        mod=o.modifiers.new('Tailored rounded edges','BEVEL');mod.width=min(d)*.01*roundness;mod.segments=3
        bpy.ops.object.modifier_apply(modifier=mod.name)
    for f in o.data.polygons:f.use_smooth=True
    if not sphere:
        mod=o.modifiers.new('Weighted surface normals','WEIGHTED_NORMAL');mod.keep_sharp=True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(M[mat]);parts.append(o);return o
def finish(name,pivot):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=parts[0];o.name='SM_Deckhand_'+name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.export_scene.gltf(filepath=str(OUT/(o.name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    o.location=Vector(pivot)*.01;parts.clear();return o

shape('Padded jacket',(0,0,27),(33,48,56),'Navy')
shape('Left flotation panel',(16,-13,32),(12,20,42),'Oilskin',.22)
shape('Right flotation panel',(16,13,32),(12,20,42),'Oilskin',.22)
shape('Rear vest',(-17,0,34),(8,46,41),'Oilskin')
for y in (-14,14):
    shape('Reflective chest strip',(22.2,y,36),(1,15,4),'Reflective',.05)
    shape('Shoulder tape',(2,y,55),(30,5,2),'Reflective')
    shape('Vest buckle',(22,y,20),(3,5,5),'Rubber')
shape('Waist belt',(1,0,6),(36,49,5),'Rubber',.1)
shape('Belt latch',(20,0,6),(3,9,6),'Reflective')
shape('Chest camera',(24,0,39),(6,8,11),'Rubber')
shape('Camera glass',(27.5,0,41),(1,5,5),'Lens',sphere=True)
shape('Recording LED',(27.5,-2.5,36.5),(1,1.2,1.2),'Red',sphere=True)
shape('Radio',(22,-16,47),(6,6,9),'Rubber')
shape('Radio aerial',(21,-18,55),(1,1,10),'Rubber')
finish('Torso',(0,0,-6))

shape('Neck',(0,0,3),(17,18,12),'Skin',sphere=True)
shape('Face',(1,0,19),(28,30,31),'Skin',sphere=True)
for y in (-15,15):shape('Ear',(0,y,19),(7,5,10),'Skin',sphere=True)
shape('Nose',(15,0,19),(11,10,10),'Skin',sphere=True)
for y in (-7,7):
    shape('Eye',(13,y,24),(5,8,9),'White',sphere=True)
    shape('Pupil',(15.5,y,24),(2,3.5,5),'Lens',sphere=True)
    shape('Brow',(14,y,30),(3,9,3),'Hair')
    shape('Moustache',(14,y*.55,12),(6,11,5),'Hair',sphere=True)
shape('Smile',(14,0,8),(2,7,2),'Hair')
shape('Helmet dome',(0,0,34),(35,37,15),'Oilskin',sphere=True)
shape('Helmet brim',(3,0,30),(43,42,3),'Oilskin',.45)
shape('Helmet centre stripe',(0,0,40),(29,6,2),'Reflective')
shape('Helmet badge',(18,0,34),(1,9,6),'Navy')
finish('Head',(0,0,51))
for side,s in [('L',-1),('R',1)]:
    shape('Sleeve',(0,s*2,-13),(22,21,29),'Oilskin',.35)
    shape('Shoulder',(0,s*2,0),(23,24,20),'Oilskin',sphere=True)
    shape('Sleeve tape',(0,s*2,-20),(23,22,4),'Reflective',.2)
    finish('UpperArm'+side,(0,s*27,44))
    shape('Forearm',(0,0,-11),(18,18,25),'Navy',.35)
    shape('Cuff',(0,0,-22),(20,20,6),'Oilskin',.2)
    shape('Mitten',(2,0,-29),(21,19,15),'Rubber',.4)
    shape('Thumb',(8,-s*7,-26),(9,8,10),'Rubber',sphere=True)
    finish('Forearm'+side,(0,s*29,17))
    shape('Trouser leg',(0,0,-17),(26,24,38),'Navy',.3)
    shape('Cargo pocket',(3,s*13,-15),(14,5,18),'Oilskin',.15)
    shape('Knee pad',(12,0,-33),(6,18,12),'Rubber',.3)
    finish('Thigh'+side,(0,s*12,-6))
    shape('Boot shaft',(0,0,-18),(23,22,35),'Oilskin',.3)
    shape('Boot rim',(0,0,-3),(25,24,6),'Rubber',.2)
    shape('Oversized boot',(7,0,-34),(40,25,17),'Rubber',.3)
    shape('Sole',(7,0,-41),(42,27,4),'Rubber',.1)
    for x in (-8,0,8,16,24):shape('Sole tread',(x,0,-43),(4,26,2),'Hair',.1)
    finish('Shin'+side,(0,s*12,-44))

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
scene.world=bpy.data.worlds.new('Deckhand review world');scene.world.color=(.2,.2,.2)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.885));floor=bpy.context.object
floor.name='Review floor';floor.data.materials.append(M['Navy'])
for p,power,size in [((4,-3,5),650,4),((-2,2,4),900,3),((1,4,2),400,3)]:
    bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
    o.rotation_euler=(Vector((0,0,.2))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(3.2,-2.7,1.65));cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.05))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.5;scene.camera=cam
scene.render.resolution_x=1000;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Deckhand_Master.blend'))
scene.render.filepath=str(OUT/'Deckhand_Review.png');bpy.ops.render.render(write_still=True)
print('DECKHAND_AUTHORED 10 original articulated mesh parts')
