"""Original flat-shaded crew. Metres in Blender, centimetres in rig contract.
No decimation of old assets: authored silhouette, shared ten-part bind pivots.
Run in a separate Blender background process; never touches an open scene.
"""
from pathlib import Path
import bpy, json, math
from mathutils import Vector

OUT = Path(__file__).resolve().parents[1] / 'SourceAssets/Generated/CrewNightwatchR5'
if OUT.exists():
    raise RuntimeError('R4 candidate directory exists; preserve before a new revision')
OUT.mkdir(parents=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
COLORS = {'oilskin':(.105,.155,.145), 'rust':(.23,.125,.065), 'navy':(.025,.036,.043),
          'cloth':(.052,.070,.078), 'rubber':(.018,.022,.024), 'skin':(.38,.255,.18),
          'skin_misha':(.46,.31,.22), 'cream':(.44,.46,.38), 'eye':(.018,.015,.014),
          'white':(.23,.19,.15), 'lens':(.06,.10,.11), 'lip':(.19,.12,.09), 'helmet':(.48,.30,.065)}
MAT = {}
for name, color in COLORS.items():
    m=bpy.data.materials.new('NW5_'+name);m.diffuse_color=(*color,1);m.use_nodes=True
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
    loft('Shaped storm jacket',[(0,13,20,0,0),(8,13,19,0,0),(19,12,18,.3,0),(28,14,21,0,0),(40,13.5,26,0,0),(47,11,21,0,0),(51,7,12,0,0)],coat)
    loft('Hem',[(0,13.4,19.4,0,0),(4,14.3,20.5,0,0)],'navy')
    # A small bib, straps and stitched pockets, no rigid chest-sized slab.
    block('Storm zipper flap',(14,0,25),(1.4,3.2,40),coat,.25)
    block('Waist belt',(13.5,0,8),(1.3,30,3),'navy',.25)
    for s in (-1,1):
        strap=block('Webbing',(13.4,s*14,35),(1.5,2.2,18),'navy',.2)
        block('Reflective shoulder tab',(13.4,s*17,40),(1,6,1.4),'cream',.15)
        block('Bellows pocket',(14.1,s*10,16),(2.2,12,12),coat,.6)
        flap=block('Pocket flap',(15.4,s*10,21),(1.2,12.5,2),coat,.15);flap.rotation_euler.x=math.radians(s*7)
        block('Lower hem stitching',(13.8,s*11,4),(.6,15,.4),'cream',.05)
    loft('Collar',[(45,9,12,0,0),(54,8,10,0,0)],'navy')
    block('Small body camera',(15,4,39),(3,5.5,7),'rubber',.5)
    block('Camera lens',(16.8,4,40),(1,2.8,2.8),'lens',.3)
    block('Chest radio',(15,-11,34),(3.5,6,10),'navy',.5)
    block('Radio antenna',(15,-12,43),(1,1,9),'rubber',.2)
    for z in (31,33,35):block('Radio grille',(17,-11,z),(.4,3,.5),'cream',.05)
    done('Torso',(0,0,-6))
    loft('Neck',[(-5,5.5,6,0,0),(9,5.7,6.2,0,0)],skin)
    loft('Adult angular skull',[(4,6,7,0,0),(7,8,9,0,0),(14,10,11,0,0),(22,10,11,0,0),(28,7.5,8.5,-1,0),(30,4,5,-2,0)],skin)
    # Flat eyes on the face planes, eyebrows and small wedge nose.
    for s in (-1,1):
        block('Inset eye shadow',(10.05,s*4.9,20),(.6,3.4,1.25),'eye',.2)
        brow=block('Brow plane',(10.2,s*4.9,22),(.7,4.3,1.4),'lip',.2)
        brow.rotation_euler.x=math.radians(s*5)
        loft('Ear',[(14,1.6,1.2,-1,s*11),(19,1.8,1.6,-1,s*11),(22,1.4,1.2,-1,s*11)],skin)
    mesh('Nose planes',[(9.9,-1.5,21),(9.9,1.5,21),(10,-2,15.8),(10,2,15.8),(12.5,-1.4,16.5),(12.5,1.4,16.5)],[(0,1,5,4),(0,4,2),(1,3,5),(2,4,5,3),(0,2,3,1)],skin)
    if variant=='Misha':block('Neutral mouth',(8.9,0,10),(.8,4,.65),'lip',.1)
    if variant=='Deckhand':
        hood=loft('Open storm hood',[(-1,8,10,-1,0),(12,12,14,-1,0),(26,12,14,-1,0),(33,8,10,-2,0),(35,3,5,-3,0)],coat)
        import bmesh
        bm=bmesh.new();bm.from_mesh(hood.data);bm.faces.ensure_lookup_table()
        # Remove front and chamfer faces on lower three bands, retain crown.
        bmesh.ops.delete(bm,geom=[bm.faces[1+k*8+j] for k in range(3) for j in (0,6,7)],context='FACES');bm.to_mesh(hood.data);bm.free()
        loft('Face gaiter',[(-2,6,8,0,0),(5,8,10,0,0),(14,10.3,11.4,0,0)],'cloth')
        for s in (-1,1):block('Hood cord',(10,s*8,3),(1,1,12),'navy',.1)
    else:
        loft('Safety helmet crown',[(26,11.5,13,0,0),(29,11.5,13,0,0),(34,9,10,-1,0),(36,4,5,-2,0)],'helmet')
        loft('Helmet brim',[(25,12.5,14,0,0),(27,12.5,14,0,0)],'helmet')
        block('Short rain peak',(11,0,26),(9,20,1.6),'helmet',.5)
        block('Helmet centre ridge',(-1,0,34),(17,3,2.5),'helmet',.4)
        for s in (-1,1):
            block('Hearing protection',(-1,s*13,18),(8,4,10),'navy',1)
    done('Head',(0,0,51))
    for side,s in [('L',-1),('R',1)]:
        sleeve=loft('Sloped shoulder sleeve',[(-28,5.8,6.2,0,0),(-23,6.8,7.2,.5,0),(-17,6,6.5,-.3,0),(-8,8,8,0,-s),(-2,8,8,0,-s*2)],coat)
        for v in sleeve.data.vertices:
            if v.co.z>-.09:v.co.z-=max(0,s*v.co.y+.02)*.45
        fold=block('Sleeve elbow fold',(5.4,0,-21),(1.6,10,1.4),coat,.2);fold.rotation_euler.x=math.radians(s*14)
        block('Sleeve seam',(0,s*8,-8),(1.3,1,14),'navy',.15)
        done('UpperArm'+side,(0,s*27,44))
        loft('Tapered forearm',[(-22,4.5,5,0,0),(-18,5.5,5.8,.4,0),(-14,4.8,5.2,-.3,.3),(-7,6.2,6.3,.3,0),(-4,6,6.3,0,0),(2,6.5,6.5,0,0)],coat)
        loft('Narrow glove cuff',[(-24,4.8,5.4,0,0),(-20,4.8,5.4,0,0)],'navy')
        block('Glove palm',(0,0,-27.3),(7.8,9,8.6),'rubber',1)
        # Four angular fingers, readable thumb; visual only, not individual rigid bodies.
        for j in range(4):
            block('Finger',(1.4,(j-1.5)*2.0,-32.3),(5.8,1.8,5.5-abs(j-1.5)*.5),'rubber',.3)
        thumb=block('Thumb',(3.8,-s*4.3,-26.5),(4.5,3.3,6.2),'rubber',.6)
        thumb.rotation_euler.x=math.radians(s*18)
        done('Forearm'+side,(0,s*29,17))
        loft('Work trousers',[(-38,7.8,7.8,0,0),(-32,8.8,8.5,.4,0),(-27,7.8,8,-.5,0),(-20,9.3,9.1,.6,s*.3),(-6,10,10,0,0),(2,10.5,10.8,0,0)],'cloth')
        block('Knee reinforcement',(8.1,0,-31),(1.8,12,9),'navy',.7)
        if side=='R':block('Tool pocket',(0,s*10,-10),(10,1.8,13),'navy',.7)
        done('Thigh'+side,(0,s*12,-6))
        loft('Trouser cuff and boot shaft',[(-33,7.5,8,0,0),(-27,7,7.5,0,0),(-20,7.5,7.8,.3,0),(-17,8.1,8.2,-.4,0),(-12,7.3,7.5,0,0),(-6,7.6,7.8,0,0),(2,7.8,7.8,0,0)],'navy')
        block('Boot toe',(4.5,0,-35),(25,15,12),'rubber',2)
        block('Flat outsole',(4.5,0,-42),(25.5,15.8,3),'rubber',.6)
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
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'Crew_Nightwatch_R5.blend'))
scene.render.filepath=str(OUT/'R5_Lineup.png');bpy.ops.render.render(write_still=True)
cam.location=(3,-1.5,.65);cam.rotation_euler=(Vector((0,-.1,.60))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.9
scene.render.filepath=str(OUT/'R5_Faces.png');bpy.ops.render.render(write_still=True)
print('CREW_NIGHTWATCH_R5_AUTHORED',len(MANIFEST),'parts')
