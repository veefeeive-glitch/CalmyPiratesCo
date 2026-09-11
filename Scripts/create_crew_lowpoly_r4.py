"""Original flat-shaded crew. Metres in Blender, centimetres in rig contract.
No decimation of old assets: authored silhouette, shared ten-part bind pivots.
Run in a separate Blender background process; never touches an open scene.
"""
from pathlib import Path
import bpy, json, math
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / 'SourceAssets/Generated/CrewLowPolyR4'
if OUT.exists():
    raise RuntimeError('R4 candidate directory exists; preserve before a new revision')
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
COLORS = {'oilskin':(.64,.34,.055), 'rust':(.40,.12,.065), 'navy':(.025,.085,.11),
          'cloth':(.055,.16,.20), 'rubber':(.018,.03,.033), 'skin':(.63,.38,.22),
          'skin_misha':(.77,.51,.32), 'cream':(.66,.67,.48), 'eye':(.012,.018,.019),
          'white':(.86,.84,.73), 'lens':(.055,.27,.31), 'lip':(.26,.10,.062)}
MAT = {}
for name, color in COLORS.items():
    m=bpy.data.materials.new('LP4_'+name);m.diffuse_color=(*color,1);m.use_nodes=True
    p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1)
    p.inputs['Roughness'].default_value=.86
    p.inputs['Specular IOR Level'].default_value=.2
    MAT[name]=m
PARTS=[];MANIFEST=[];CHARACTERS={}

def mesh(name, verts, faces, mat):
    data=bpy.data.meshes.new(name);data.from_pydata([Vector(v)*.01 for v in verts],[],faces);data.update()
    o=bpy.data.objects.new(name,data);bpy.context.collection.objects.link(o);data.materials.append(MAT[mat])
    for f in data.polygons:f.use_smooth=False
    PARTS.append(o);return o

def loft(name, rings, mat):
    # Octagonal cross-sections: front/back flat, bevel planes on silhouette.
    verts=[]
    for z,rx,ry,cx,cy in rings:
        verts += [(cx+x*rx,cy+y*ry,z) for x,y in [(1,.62),(.62,1),(-.62,1),(-1,.62),(-1,-.62),(-.62,-1),(.62,-1),(1,-.62)]]
    faces=[tuple(range(7,-1,-1))]
    for k in range(len(rings)-1):
        for i in range(8):faces.append((k*8+i,k*8+(i+1)%8,(k+1)*8+(i+1)%8,(k+1)*8+i))
    faces.append(tuple((len(rings)-1)*8+i for i in range(8)))
    return mesh(name,verts,faces,mat)

def block(name,p,d,mat,bevel=.5):
    bpy.ops.mesh.primitive_cube_add(size=1,location=Vector(p)*.01)
    o=bpy.context.object;o.name=name;o.dimensions=Vector(d)*.01
    bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    if bevel:
        mod=o.modifiers.new('One cut, flat facets','BEVEL');mod.width=min(bevel,min(d)*.2)*.01;mod.segments=1
        bpy.ops.object.modifier_apply(modifier=mod.name)
    o.data.materials.append(MAT[mat]);PARTS.append(o);return o

def finish(name,pivot,lineup_y):
    bpy.ops.object.select_all(action='DESELECT')
    for o in PARTS:o.select_set(True)
    bpy.context.view_layer.objects.active=PARTS[0];bpy.ops.object.join();o=PARTS[0];o.name=name
    bpy.context.scene.cursor.location=(0,0,0);bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.smart_project(island_margin=.02);bpy.ops.object.mode_set(mode='OBJECT')
    for poly in o.data.polygons:poly.use_smooth=False
    o.data.calc_loop_triangles();tris=len(o.data.loop_triangles)
    assert tris<1600,(name,tris)
    bpy.ops.export_scene.gltf(filepath=str(OUT/(name+'.glb')),export_format='GLB',use_selection=True,export_apply=True)
    MANIFEST.append({'name':name,'triangles':tris,'vertices':len(o.data.vertices),'flat_shaded':True,'pivot_cm':pivot,'materials':[m.name for m in o.data.materials]})
    o.location=Vector(pivot)*.01+Vector((0,lineup_y,0));PARTS.clear();return o

for variant,lineup_y in [('Deckhand',-.58),('Misha',.58)]:
    coat='oilskin' if variant=='Deckhand' else 'rust';skin='skin' if variant=='Deckhand' else 'skin_misha'
    objs=[]
    def done(part,pivot):
        objs.append(finish('SM_'+variant+'_'+part,pivot,lineup_y))
    loft('Tailored rain jacket',[(0,13,19,0,0),(7,14,21,0,0),(33,15,22,0,0),(45,13,23,0,0),(50,9,15,0,0)],coat)
    loft('Hem',[(0,13.4,19.4,0,0),(4,14.3,20.5,0,0)],'navy')
    # A small bib, straps and stitched pockets, no rigid chest-sized slab.
    block('Bib',(15.2,0,26),(1.8,27,30),'cloth',.6)
    for s in (-1,1):
        strap=block('Webbing',(14.1,s*13,40),(1.5,3.3,23),'navy',.2)
        block('Reflective shoulder tab',(15,s*16,42),(1,7,2.5),'cream',.15)
        block('Angled pocket',(16.3,s*8,18),(1.4,10,9),'cloth',.4)
        block('Pocket flap',(17,s*8,22),(1.2,10.5,2),'navy',.15)
    loft('Collar',[(45,9,12,0,0),(54,8,10,0,0)],'navy')
    block('Small body camera',(17,0,39),(3,6,7),'rubber',.5)
    block('Camera lens',(18.8,0,40),(1,3,3),'lens',.3)
    done('Torso',(0,0,-6))
    loft('Neck',[(-5,6,7,0,0),(9,6.5,7.5,0,0)],skin)
    loft('Faceted head',[(6,7,10,0,0),(10,11,13,0,0),(19,14,16,0,0),(28,13.5,15,0,0),(34,9,11,-1,0)],skin)
    # Flat eyes on the face planes, eyebrows and small wedge nose.
    for s in (-1,1):
        block('Eye white',(14.12,s*6.8,23),(1,4.6,4.8),'white',.5)
        block('Pupil',(14.74,s*6.6,23),(0.45,2.2,3.4),'eye',.25)
        brow=block('Eyebrow',(14.5,s*6.8,27.1),(.65,5.6,1.3),'navy',.15)
        brow.rotation_euler.x=math.radians(s*(5 if variant=='Deckhand' else -8))
        loft('Ear',[(17,2.1,2.2,-1,s*16),(23,2.8,2.4,-1,s*16),(26,1.4,1.7,-1,s*16)],skin)
    mesh('Nose wedge',[(13.5,-2,23),(13.5,2,23),(13.7,-2.7,17.7),(13.7,2.7,17.7),(19,-2,18.8),(19,2,18.8)],[(0,1,5,4),(0,4,2),(1,3,5),(2,4,5,3),(0,2,3,1)],skin)
    block('Quiet smile',(12.25,0,12.8),(1,6.3,1.15),'lip',.15)
    if variant=='Deckhand':
        loft('Knitted beanie',[(29,14,16,0,0),(34,13.7,15.8,0,0),(40,10,12,-1,0),(43,3,5,-2,0)],'navy')
        loft('Folded beanie brim',[(28,14.3,16.4,0,0),(32,14.5,16.6,0,0)],'cloth')
        block('Hat patch',(14.8,-5,30),(.7,4.5,2.8),'cream',.1)
    else:
        loft('Watch cap crown',[(29,14,16,0,0),(37,12,14,-2,0),(40,5,8,-3,0)],'cloth')
        loft('Cap band',[(27.5,14.4,16.4,0,0),(31,14.4,16.4,0,0)],'navy')
        block('Short cap visor',(16,0,28.8),(17,27,2.3),'navy',1)
    done('Head',(0,0,51))
    for side,s in [('L',-1),('R',1)]:
        loft('Tapered upper sleeve',[(-28,6.5,6.5,0,0),(-19,7.3,7.5,0,0),(-5,9,9,0,0),(4,7.6,8,0,0)],coat)
        block('Sleeve seam',(0,s*8,-8),(1.3,1,14),'navy',.15)
        done('UpperArm'+side,(0,s*27,44))
        loft('Tapered forearm',[(-22,4.5,5,0,0),(-18,4.7,5.4,0,0),(-4,6,6.3,0,0),(2,6.5,6.5,0,0)],coat)
        loft('Narrow glove cuff',[(-24,4.8,5.4,0,0),(-20,4.8,5.4,0,0)],'navy')
        block('Glove palm',(0,0,-27.3),(7.8,9,8.6),'rubber',1)
        # Four angular fingers, readable thumb; visual only, not individual rigid bodies.
        for j in range(4):
            block('Finger',(1.4,(j-1.5)*2.0,-32.3),(5.8,1.8,5.5-abs(j-1.5)*.5),'rubber',.3)
        thumb=block('Thumb',(3.8,-s*4.3,-26.5),(4.5,3.3,6.2),'rubber',.6)
        thumb.rotation_euler.x=math.radians(s*18)
        done('Forearm'+side,(0,s*29,17))
        loft('Work trousers',[(-38,7.8,7.8,0,0),(-29,8.2,8.5,0,0),(-6,10,10,0,0),(2,10.5,10.8,0,0)],'cloth')
        block('Knee reinforcement',(8.1,0,-31),(1.8,12,9),'navy',.7)
        if side=='R':block('Tool pocket',(0,s*10,-10),(10,1.8,13),'navy',.7)
        done('Thigh'+side,(0,s*12,-6))
        loft('Boot upper',[(-33,7.5,8,0,0),(-27,7,7.5,0,0),(-6,7.6,7.8,0,0),(2,7.8,7.8,0,0)],'navy')
        block('Boot toe',(4.5,0,-35),(26,17,13),'rubber',2)
        block('Flat outsole',(4.5,0,-42),(26.5,17.8,3),'rubber',.6)
        block('Heel pull tab',(-7,0,-24),(1.5,4,7),'cream',.2)
        done('Shin'+side,(0,s*12,-44))
    CHARACTERS[variant]=objs

for variant in CHARACTERS:
    total=sum(p['triangles'] for p in MANIFEST if p['name'].startswith('SM_'+variant+'_'))
    assert total<6500,(variant,total)
(OUT/'mesh_manifest.json').write_text(json.dumps({'style':'authored flat-shaded low-poly','units':'metres; pivot_cm in manifest','parts':MANIFEST,'triangles':{v:sum(p['triangles'] for p in MANIFEST if p['name'].startswith('SM_'+v+'_')) for v in CHARACTERS}},indent=2))

scene=bpy.context.scene;scene.render.engine='CYCLES';scene.cycles.samples=32
scene.world=bpy.data.worlds.new('Neutral studio');scene.world.color=(.18,.18,.18)
scene.view_settings.view_transform='AgX'
floor=block('Studio floor',(0,0,-90),(1000,1000,2),'navy',0);PARTS.clear()
for loc,power,size in [((3,-4,5),500,4),((-2,3,4),650,3),((3,3,2),180,3)]:
    bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=power;o.data.shape='DISK';o.data.size=size
    o.rotation_euler=(Vector((0,0,.2))-o.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(4,-2,1.65));cam=bpy.context.object
cam.rotation_euler=(Vector((0,0,.0))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=3.35;scene.camera=cam
scene.render.resolution_x=1500;scene.render.resolution_y=1100;scene.render.resolution_percentage=100
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Crew_LowPoly_R4.blend'))
scene.render.filepath=str(OUT/'R4_Lineup.png');bpy.ops.render.render(write_still=True)
cam.location=(3,-1.5,.65);cam.rotation_euler=(Vector((0,-.1,.60))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.9
scene.render.filepath=str(OUT/'R4_Faces.png');bpy.ops.render.render(write_still=True)
print('CREW_LOWPOLY_R4_AUTHORED',len(MANIFEST),'parts')
