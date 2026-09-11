"""Original stylized crew + four cargo silhouettes. Isolated new R3 directory.
Reference URLs are recorded separately; no PEAK meshes, textures or trademarks.
"""
from pathlib import Path
import bpy, math
from mathutils import Vector
OUT=Path(__file__).resolve().parents[1]/'SourceAssets/Generated/CrewCargoR3'
OUT.mkdir(parents=True,exist_ok=True)
if (OUT/'CrewCargo_R3.blend').exists(): raise RuntimeError('R3 candidate exists; preserve it')
bpy.ops.wm.read_factory_settings(use_empty=True)
palette={'coat':(.96,.56,.09),'pants':(.075,.19,.23),'skin':(.85,.61,.37),'dark':(.018,.037,.048),
         'white':(.82,.88,.79),'steel':(.23,.32,.34),'wood':(.42,.23,.095),'blue':(.025,.3,.42),
         'red':(.72,.095,.055),'glass':(.13,.64,.68),'sand':(.53,.5,.33)}
M={}
for n,c in palette.items():
    m=bpy.data.materials.new('R3_'+n);m.diffuse_color=(*c,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*c,1);p.inputs['Roughness'].default_value=.58
    p.inputs['Metallic'].default_value=.65 if n=='steel' else 0;M[n]=m
pieces=[]
def shape(n,p,d,mat,roundness=.15,sphere=False):
    if sphere:bpy.ops.mesh.primitive_uv_sphere_add(segments=20,ring_count=12,location=Vector(p)*.01)
    else:bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)*.01)
    o=bpy.context.object;o.name=n;o.dimensions=Vector(d)*.01
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if not sphere:
        b=o.modifiers.new('Soft manufactured edge','BEVEL');b.width=min(d)*.01*roundness;b.segments=3;bpy.ops.object.modifier_apply(modifier=b.name)
        b=o.modifiers.new('Normals','WEIGHTED_NORMAL');b.keep_sharp=True;bpy.ops.object.modifier_apply(modifier=b.name)
    for f in o.data.polygons:f.use_smooth=True
    o.data.materials.append(M[mat]);pieces.append(o);return o
def cylinder(n,p,r,depth,mat):
    bpy.ops.mesh.primitive_cylinder_add(vertices=32,radius=r*.01,depth=depth*.01,location=Vector(p)*.01)
    o=bpy.context.object;o.name=n;o.data.materials.append(M[mat]);pieces.append(o)
    b=o.modifiers.new('Rolled rim','BEVEL');b.width=.008;b.segments=2;bpy.ops.object.modifier_apply(modifier=b.name)
    for f in o.data.polygons:f.use_smooth=True
    return o
def finish(name,pivot):
    bpy.ops.object.select_all(action='DESELECT')
    for o in pieces:o.select_set(True)
    bpy.context.view_layer.objects.active=pieces[0];bpy.ops.object.join();o=pieces[0];o.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    o.location=Vector(pivot)*.01;pieces.clear();return o

# Readable deckhand: rounded waterproof suit, quiet face, no protruding eyes/moustache.
shape('Oilskin body',(0,0,26),(31,43,55),'coat',.38)
shape('Chest bib',(16,0,30),(4,31,30),'pants',.25)
shape('Waist',(0,0,4),(32,44,9),'pants',.3)
for s in (-1,1):
    shape('Safety tape',(17,s*12,47),(2,8,4),'white')
    shape('Vest strap',(17,s*18,32),(3,4,39),'dark')
shape('Body camera',(20,0,42),(5,8,10),'dark')
shape('Lens',(23,0,43),(1,4,4),'glass',sphere=True)
finish('SM_Deckhand_Torso',(0,0,-6))
shape('Neck',(0,0,2),(17,18,10),'pants',sphere=True)
shape('Head',(0,0,20),(31,36,35),'skin',sphere=True)
shape('Beanie',(0,0,33),(33,38,16),'pants',sphere=True)
shape('Beanie cuff',(0,0,29),(34,38,7),'pants',.4)
for s in (-1,1):shape('Simple eye',(14.7,s*7,21),(1.5,4,6),'dark',sphere=True)
shape('Mouth',(15,0,12),(1,6,1.3),'dark',.4)
finish('SM_Deckhand_Head',(0,0,51))
for side,s in [('L',-1),('R',1)]:
    shape('Sleeve',(0,s,-12),(18,18,28),'coat',.42)
    shape('Shoulder',(0,s,0),(19,20,18),'coat',sphere=True)
    finish('SM_Deckhand_UpperArm'+side,(0,s*27,44))
    shape('Forearm',(0,0,-10),(15,15,24),'coat',.4)
    shape('Cuff',(0,0,-22),(16,16,5),'pants',.3)
    shape('Glove palm',(0,0,-28),(13,14,12),'dark',.35)
    for j in range(4):shape('Curled finger',(2,(j-1.5)*3,-34),(7,2.7,7),'dark',.4)
    shape('Opposed thumb',(5,-s*7,-27),(7,5,8),'dark',sphere=True)
    finish('SM_Deckhand_Forearm'+side,(0,s*29,17))
    shape('Trouser',(0,0,-17),(23,21,37),'pants',.38)
    shape('Knee',(10,0,-32),(5,16,10),'dark',.3)
    finish('SM_Deckhand_Thigh'+side,(0,s*12,-6))
    shape('Boot shaft',(0,0,-17),(19,20,34),'pants',.35)
    shape('Boot',(5,0,-35),(33,23,16),'dark',.32)
    shape('Sole',(5,0,-42),(34,24,3),'sand',.15)
    finish('SM_Deckhand_Shin'+side,(0,s*12,-44))

# Cargo authored within +/-50cm normalized root proxy. Distinct functional fittings.
for kind in ('Standard','Unstable','Fragile','Bulky'):
    if kind=='Standard':
        for z in (-34,-14,6,26):
            for y in (-44,44):shape('Side timber',(0,y,z),(96,7,16),'wood',.035)
            for x in (-44,44):shape('End timber',(x,0,z),(7,80,16),'wood',.035)
        for x in (-34,-17,0,17,34):shape('Lid plank',(x,0,43),(16,96,8),'wood',.02)
        shape('Base',(0,0,-44),(96,96,9),'wood',.03)
        for x in (-35,35):
            for y in (-46,46):shape('Corner brace',(x,y,0),(8,4,88),'steel',.08)
        shape('Supply label',(-48,0,0),(1,30,20),'sand',.05)
        for y in (-47,47):shape('Grab rail',(0,y,30),(32,5,7),'dark',.2)
    elif kind=='Unstable':
        cylinder('Liquid drum',(0,0,0),37,82,'blue')
        for z in (-41,-24,24,41):cylinder('Steel rolled band',(0,0,z),39,4,'steel')
        cylinder('Filler cap',(12,8,44),9,6,'red')
        for x in (-44,44):
            for y in (-44,44):shape('Protective cage upright',(x,y,0),(5,5,96),'steel')
        for z in (-46,46):
            for y in (-44,44):shape('Cage cross rail',(0,y,z),(92,5,5),'steel')
            for x in (-44,44):shape('Cage end rail',(x,0,z),(5,92,5),'steel')
        shape('Liquid hazard plaque',(-43,0,5),(3,27,29),'red',.08)
    elif kind=='Fragile':
        shape('Instrument transit case',(0,0,0),(96,87,77),'sand',.14)
        shape('Lid',(0,0,37),(96,90,15),'pants',.12)
        for x in (-43,43):
            for y in (-38,38):shape('Rubber corner',(x,y,0),(12,13,90),'dark',.18)
        for y in (-25,25):shape('Butterfly latch',(-49,y,27),(2,11,14),'steel',.1)
        for y in (-47,47):shape('Carry handle',(0,y,8),(33,5,8),'dark',.3)
        shape('Fragile label',(-49,0,0),(1,24,24),'white')
        shape('Glass symbol stem',(-49.7,0,-3),(.4,2,12),'red')
        shape('Glass symbol cup',(-49.7,0,5),(.4,12,7),'red')
    else:
        shape('Skid',(0,0,-44),(98,96,10),'steel',.04)
        for y in (-39,39):shape('Skid runner',(0,y,-48),(98,10,4),'dark')
        shape('Motor housing',(6,8,-3),(68,62,64),'coat',.22)
        shape('Cooling grille',(-30,8,-3),(5,58,50),'dark',.08)
        for z in range(-24,25,7):shape('Cooling fin',(-33,8,z),(2,56,2),'steel')
        shape('Control console',(5,-29,10),(47,12,38),'pants',.2)
        for x in (-8,4,16):shape('Control button',(x,-36,14),(6,2,6),'red' if x==16 else 'glass',sphere=True)
        for y in (-44,44):
            for x in (-44,44):shape('Carry frame',(x,y,0),(6,6,91),'dark',.2)
            shape('Two-person grab rail',(0,y,43),(94,7,7),'dark',.3)
        shape('Fuel cap',(12,8,33),(17,17,6),'red',.35)
    finish('SM_Cargo_'+kind,(0,160+('Standard','Unstable','Fragile','Bulky').index(kind)*150,0))

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
scene.world=bpy.data.worlds.new('R3 studio');scene.world.color=(.22,.22,.22)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.9));bpy.context.object.data.materials.append(M['pants'])
for p,power in [((4,-3,6),1300),((-3,4,5),1600),((4,7,4),1800)]:
    bpy.ops.object.light_add(type='AREA',location=p);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=5
    o.rotation_euler=(Vector((0,2,0))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(6,-4,3));cam=bpy.context.object
cam.rotation_euler=(Vector((0,2.7,0))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=8;scene.camera=cam
scene.render.resolution_x=1500;scene.render.resolution_y=900;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CrewCargo_R3.blend'))
scene.render.filepath=str(OUT/'R3_Lineup.png');bpy.ops.render.render(write_still=True)
print('CREW_CARGO_R3_AUTHORED 10 crew parts, 4 distinct cargo meshes')
