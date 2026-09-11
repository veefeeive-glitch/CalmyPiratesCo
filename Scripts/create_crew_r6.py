"""R6 original maritime crew, centimetre-space geometry / ten-part physical rig.
Run with Blender --background --python. New output only; never edits R5.
"""
from pathlib import Path
import bpy, json, math
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / 'SourceAssets/Generated/CrewR6B'
assert not OUT.exists(), 'Preserve candidate before rebuilding'
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
COLORS = dict(teal=(.17,.245,.225), ochre=(.38,.205,.085),
    navy=(.065,.086,.105), cuff=(.042,.052,.06), glove=(.09,.105,.11),
    sole=(.028,.035,.038), skin=(.44,.30,.215), misha=(.49,.345,.24),
    beard=(.355,.265,.20), lip=(.27,.155,.12), eye=(.025,.022,.019),
    sclera=(.39,.34,.28), trim=(.44,.46,.405), lens=(.085,.15,.16),
    helmet=(.57,.345,.095), seam=(.10,.12,.115))
MAT={}
for name,color in COLORS.items():
    m=bpy.data.materials.new('CR6_'+name);m.diffuse_color=(*color,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value={'helmet':.62,'glove':.76,'sole':.8,'lens':.42}.get(name,.87)
    p.inputs['Specular IOR Level'].default_value=.22
    MAT[name]=m
PARTS=[];MANIFEST=[];CHARS={}

def mesh(name,verts,faces,mat):
    d=bpy.data.meshes.new(name);d.from_pydata([Vector(v)*.01 for v in verts],[],faces);d.update()
    o=bpy.data.objects.new(name,d);bpy.context.collection.objects.link(o);d.materials.append(MAT[mat]);PARTS.append(o)
    return o

PROFILE=[(1,0),(.96,.45),(.65,.86),(0,1),(-.65,.86),(-.96,.45),(-1,0),(-.96,-.45),(-.65,-.86),(0,-1),(.65,-.86),(.96,-.45)]
def loft(name,rings,mat,fold=0,open_front=False):
    verts=[]
    for k,(z,rx,ry,cx,cy) in enumerate(rings):
        for i,(x,y) in enumerate(PROFILE):
            # Offset selected intermediate rings diagonally: no stacked accordion armor.
            dz=fold*math.sin(i*1.7+k*1.9) if 0<k<len(rings)-1 else 0
            verts.append((cx+x*rx,cy+y*ry,z+dz))
    faces=[tuple(range(11,-1,-1))] if not open_front else []
    for k in range(len(rings)-1):
        for i in range(12):
            if open_front and i in (0,1,10,11):continue
            faces.append((k*12+i,k*12+(i+1)%12,(k+1)*12+(i+1)%12,(k+1)*12+i))
    if not open_front:faces.append(tuple((len(rings)-1)*12+i for i in range(12)))
    return mesh(name,verts,faces,mat)

def box(name,p,d,mat,bevel=.35):
    bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)*.01);o=bpy.context.object;o.name=name;o.dimensions=Vector(d)*.01
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('Edge facets','BEVEL');mod.width=min(bevel,min(d)*.22)*.01;mod.segments=1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(MAT[mat]);PARTS.append(o);return o

def bar(name,a,b,width,depth,mat):
    a=Vector(a);b=Vector(b);o=box(name,(a+b)/2,(depth,width,(b-a).length),mat,.1)
    o.rotation_euler=(b-a).to_track_quat('Z','Y').to_euler();return o

def finish(name,pivot,y):
    bpy.ops.object.select_all(action='DESELECT')
    for o in PARTS:o.select_set(True)
    bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();o=PARTS[0];o.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
    o.data.calc_loop_triangles();tri=len(o.data.loop_triangles)
    assert tri<1900,(name,tri)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    MANIFEST.append(dict(name=name,triangles=tri,pivot_cm=pivot,materials=[m.name for m in o.data.materials]))
    o.location=Vector(pivot)*.01+Vector((0,y,0));PARTS.clear();return o

for variant,y in [('Deckhand',-.59),('Misha',.59)]:
    coat='teal' if variant=='Deckhand' else 'ochre';skin='skin' if variant=='Deckhand' else 'misha';objects=[]
    def done(part,pivot):objects.append(finish('SM_'+variant+'_'+part,pivot,y))
    loft('Tailored storm coat',[(0,13,19.5,0,0),(5,13.6,20.2,0,0),(17,12.6,18.3,.4,0),
        (29,14.1,21,0,0),(39,13.5,25,0,0),(46,11.5,25,0,0),(50,8.5,18,0,0),(52,6.5,9,0,0)],coat,1.2)
    loft('Soft waist hem',[(0,13.1,19.7,0,0),(2,13.6,20.2,0,0)],coat)
    bar('Covered zipper',(14,0,5),(14,0,41),1.4,.65,coat)
    for s in (-1,1):
        # Flush welt openings, instead of projecting blocks strapped to a torso.
        bar('Slanted welt pocket',(12.8,s*10,9),(13.7,s*13.4,21),1.3,.7,'cuff')
        bar('Pocket welt stitch',(13.3,s*8.7,9),(14.1,s*12.1,21),.65,.45,coat)
        bar('Chest reflective strip',(13.3,s*8,39),(12.5,s*17,39),1.5,.5,'trim')
    # Rolled collar closes the long exposed-neck gap; open throat, not a solid disk.
    collar=loft('Rolled storm collar',[(44,8.2,10.7,0,0),(51,9.1,11.2,-1,0),(56,7.8,9.5,-1,0)],coat,open_front=True)
    mod=collar.modifiers.new('Collar thickness','SOLIDIFY');mod.thickness=.007;bpy.context.view_layer.objects.active=collar;bpy.ops.object.modifier_apply(modifier=mod.name)
    loft('Soft inner collar',[(49,6.7,8,0,0),(56.5,6.8,8,0,0)],'navy',.3)
    box('Chest camera',(15.2,6.1,36),(3,5.8,6.2),'cuff',.6)
    box('Camera lens',(16.9,6.1,37),(1.1,2.7,2.7),'lens',.35)
    box('Radio',(15,-10,32),(3.2,5.3,9),'cuff',.5)
    bar('Flexible antenna',(15,-11.5,36),(14.6,-11.5,44),.65,.65,'cuff')
    for z in (30,32,34):box('Radio grille',(16.7,-10,z),(.3,2.8,.4),'trim',0)
    done('Torso',(0,0,-6))

    loft('Neck',[(-6,5.8,6.1,0,0),(8,6.2,6.6,0,0)],skin)
    face=loft('Facial anatomy',[(4,5.7,6.7,1,0),(7,8.1,8.2,1,0),(12,9.2,10,.3,0),
        (17,10.3,10.6,0,0),(22,10,10.3,0,0),(26,9.4,9.9,-.4,0),(30,6.5,7.5,-1,0),(31,2,3,-1.5,0)],skin)
    for s in (-1,1):
        # Almond eyelids with small muted pupils; neither cartoon white eyes nor black bars.
        cy=s*4.5
        mesh('Eye socket',[(9.6,cy-2,19.5),(10.25,cy-1.3,20.25),(10.4,cy+1.3,20.25),(9.7,cy+2,19.5),(10.1,cy+1,18.9),(10.1,cy-1,18.9)],[(0,1,2,3,4,5)],'lip')
        mesh('Eye almond',[(10.4,cy-1.65,19.55),(10.48,cy,19.95),(10.4,cy+1.65,19.55),(10.45,cy,19.17)],[(0,1,2,3)],'sclera')
        box('Dark iris',(10.54,cy,19.55),(.18,.95,.8),'eye',.08)
        mesh('Eyebrow',[(10.25,cy-2.4,21.3),(10.2,cy-1.5,22),(10.1,cy+2.1,21.9),(10.3,cy+2.1,21.25)],[(0,1,2,3)],'beard')
        loft('Ear',[(12,1.4,1,-.3,s*10.7),(16,1.9,1.35,-.3,s*10.7),(20,1.3,1.2,-.3,s*10.7)],skin)
    mesh('Nose',[(10,-1.2,21),(10,1.2,21),(10,-2.2,14.9),(10,2.2,14.9),(12.4,-1.3,15.8),(12.4,1.3,15.8),(11,0,15)],[(0,1,5,4),(0,4,2),(1,3,5),(2,4,6),(4,5,6),(5,3,6),(2,6,3),(0,2,3,1)],skin)
    if variant=='Misha':
        # Pigment on the existing jaw surface, not a floating beard-shaped plate.
        face.data.materials.append(MAT['beard'])
        for f in face.data.polygons:
            c=sum((face.data.vertices[i].co for i in f.vertices),Vector())/len(f.vertices)
            if c.z<.115 and c.x>.025:f.material_index=1
        bar('Mouth seam',(9.75,-2.2,10.9),(9.75,2.2,10.9),.35,.25,'lip')
        loft('Helmet dome',[(25.5,11.6,12.7,-.3,0),(29,11.3,12.5,-.3,0),(33,8.5,9.7,-1,0),(35,3.5,4.6,-1,0)],'helmet')
        loft('Helmet rolled brim',[(24.6,12.4,13.5,.1,0),(26,12.4,13.5,.1,0)],'helmet')
        box('Rain peak',(11.5,0,25.2),(7,18,1.2),'helmet',.4)
        box('Helmet spine',(-1,0,33.4),(15,2.4,1.3),'helmet',.3)
        for s in (-1,1):
            box('Ear defender cushion',(-1,s*11.8,17),(6.5,2.5,8),'cuff',.5)
            box('Ear defender shell',(-1,s*13,17),(6.5,2.5,8.3),'navy',.7)
    else:
        hood=loft('Storm hood',[(0,8.5,10.8,-1,0),(13,11.4,13,-1,0),(25,11.8,13.2,-1,0),(31,9.5,11.2,-1,0)],coat,open_front=True)
        mod=hood.modifiers.new('Finished hood lip','SOLIDIFY');mod.thickness=.01;bpy.context.view_layer.objects.active=hood;bpy.ops.object.modifier_apply(modifier=mod.name)
        loft('Hood crown',[(30,9.5,11.2,-1,0),(33.5,6.7,8.1,-1.5,0),(35,2.5,3.8,-2,0)],coat)
        loft('Soft gaiter',[(-3,6.6,7.6,0,0),(5,8.8,9.6,.5,0),(13,10.25,10.6,.25,0)],'navy',.5)
        for s in (-1,1):bar('Short hood cord',(7.3,s*9,5),(8.5,s*8,-2),.7,.7,'trim')
    # Shorter visible neck and forehead, retaining the exact physical bind pivot.
    for o in PARTS:
        for v in o.data.vertices:
            world=o.matrix_world @ v.co
            z=world.z
            if z>-.04:world.z-=.035
            if z>.22:world.z-=(z-.22)*.18
            v.co=o.matrix_world.inverted() @ world
    done('Head',(0,0,51))

    for side,s in [('L',-1),('R',1)]:
        loft('Raglan rounded shoulder',[(-28,6.2,6.4,0,0),(-21,6.7,6.7,.25,0),(-15,7.2,7.2,0,-s),
            (-10,6.4,6.6,0,-s*2),(-7,4.2,4.6,0,-s*4),(-5,1.2,1.5,0,-s*5)],coat,.5)
        loft('Sleeve reflective band',[(-19,7,7,0,0),(-16.5,7.3,7.3,0,-s*.5)],'trim',.3)
        done('UpperArm'+side,(0,s*27,44))
        loft('Directional sleeve folds',[(-22,4.2,4.7,0,0),(-18,4.9,5.1,.4,0),(-11,5.6,5.7,-.15,.2),(-4,5.8,5.9,0,0),(2,5.8,5.9,0,0)],coat,.7)
        loft('Elastic wrist',[(-24,4.3,4.8,0,0),(-21,4.5,5,0,0)],'cuff')
        loft('Glove palm',[(-31,2.8,3.7,.4,0),(-28,3.5,4.3,0,0),(-24,3.4,4.3,0,0),(-23,2.6,3.4,0,0)],'glove')
        for j in range(4):
            yy=(j-1.5)*1.9;length=5.3-abs(j-1.5)*.65
            loft('Curled finger',[(-29-length,.8,.8,1.4,yy),(-28-length,1.25,.9,1.5,yy),(-30,1.35,.95,.8,yy),(-28.8,1.2,.95,.3,yy)],'glove')
        loft('Opposable thumb',[(-29,1.2,1.2,3,-s*3.7),(-26.5,1.6,1.4,3.9,-s*4),(-24.4,1.4,1.3,2.7,-s*3.8)],'glove')
        box('Glove wrist label',(-4.45,0,-23.8),(.4,4.3,1.2),'trim',.1)
        bar('Glove back seam',(-3.4,-2.5,-26.3),(-3.4,2.5,-26.3),.45,.35,'trim')
        done('Forearm'+side,(0,s*29,17))
        loft('Tapered work trousers',[(-39,7.6,7.7,.1,0),(-31,8,8.3,.8,0),(-19,9.1,9.2,0,0),(-7,10.3,10.6,-.3,0),(2,10.6,10.8,0,0)],'navy',1.2)
        if side=='R':
            box('Flat thigh pocket',(-1,s*10.1,-12),(9,1.2,11),'navy',.35)
            box('Thigh pocket flap',(-1,s*10.6,-7),(9.2,.6,1.5),'cuff',.12)
        done('Thigh'+side,(0,s*12,-6))
        loft('Trouser lower leg',[(-30,6.8,7.3,.1,0),(-24,7.3,7.8,-.2,0),(-13,7.6,8,.3,0),(-5,7.8,8,0,0),(2,7.8,8,0,0)],'navy',.8)
        loft('Ankle reflective band',[(-26,7.4,7.9,0,0),(-23.8,7.6,8,0,0)],'trim',.25)
        loft('Safety boot shaft',[(-39,7.8,7.2,1,0),(-32,7.1,7.1,0,0),(-27,6.7,6.9,0,0)],'cuff')
        loft('Rounded safety toe',[(-41,12.7,7.5,4.5,0),(-37,12.5,7.5,4.5,0),(-34,10.3,7,4.8,0),(-29,5.3,6.4,.3,0)],'glove')
        loft('Boot outsole',[(-43.5,12.8,7.8,4.5,0),(-40.7,13,7.8,4.5,0)],'sole')
        box('Boot pull loop',(-6.8,0,-28),(1,3,4),'cuff',.2)
        done('Shin'+side,(0,s*12,-44))
    CHARS[variant]=objects

totals={v:sum(p['triangles'] for p in MANIFEST if p['name'].startswith('SM_'+v+'_')) for v in CHARS}
assert all(n<6500 for n in totals.values()),totals
(OUT/'mesh_manifest.json').write_text(json.dumps(dict(units='metres; bind pivots in cm',style='original faceted low-poly workwear',triangles=totals,parts=MANIFEST),indent=2))
scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=24
scene.world=bpy.data.worlds.new('Neutral studio');scene.world.color=(.16,.16,.16);scene.view_settings.view_transform='AgX'
box('Studio floor',(0,0,-90),(1000,1000,2),'navy',0);PARTS.clear()
for loc,power,size in [((3,-4,5),550,4),((-2,3,4),650,3),((3,3,2),240,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
    o.rotation_euler=(Vector((0,0,.2))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(4,-2,1.35));cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO'
scene.render.resolution_x=1500;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
def render(name,loc,target,scale):
    cam.location=loc;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=scale
    scene.render.filepath=str(OUT/name);bpy.ops.render.render(write_still=True)
cam.data.ortho_scale=3.1;cam.rotation_euler=(Vector((0,0,0))-cam.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Crew_R6.blend'))
render('R6_Lineup.png',(4,-1.7,1.0),(0,0,-.02),2.9)
render('R6_Faces.png',(3,-.8,.8),(0,0,.64),1.65)
render('R6_Back.png',(-4,1.5,1.0),(0,0,0),2.9)
print('CREW_R6_AUTHORED',totals)
