"""R3 authored refit, invoked by the authoritative generator before saving.

Metric coordinates, +X bow. This is a game arrangement, NOT a certified ship.
Route metadata is deliberately separate from physics certification in Unreal.
"""
import bpy
import math
import json
from mathutils import Vector


def apply_refit(g):
    box, cyl, tube = g['box'], g['cylinder'], g['cylinder_between']
    mesh, cut = g['mesh_object'], g['cut_mesh_opening']
    M, C = g['MAT'], g['COL']
    structure, machinery, fittings = C['30_SUPERSTRUCTURE'], C['40_MACHINERY_SYSTEMS'], C['60_DECK_FITTINGS']
    decks = C['20_DECKS_BULKHEADS']
    routes, rooms = [], []

    def remove(*prefixes):
        for o in list(bpy.data.objects):
            if o.name.startswith(prefixes):
                bpy.data.objects.remove(o, do_unlink=True)

    def b(n, p, d, m='white', c=structure, bevel=.015):
        return box('R3_' + n, p, d, M[m], c, bevel)

    def line(n, a, z, r=.022, m='steel', c=structure):
        return tube('R3_' + n, a, z, r, M[m], c, 12)

    def label(n, text, p, size=.13, facing=0, mat='white'):
        obj = g['add_text']('R3_' + n, text, p, size, structure)
        obj.rotation_euler.z = facing
        obj.data.materials.clear()
        obj.data.materials.append(M[mat])
        obj.data.extrude = .0004
        return obj

    def guard(n, a, z, height=1.05, c=decks):
        a, z = Vector(a), Vector(z)
        length = (z-a).length
        for h in (.52, height):
            line(n+'_Rail'+str(h), a+Vector((0,0,h)), z+Vector((0,0,h)), .026, 'steel', c)
        count = max(1, math.ceil(length/1.4))
        for i in range(count+1):
            p = a.lerp(z, i/count)
            line(n+'_Post'+str(i), p, p+Vector((0,0,height)), .029, 'steel', c)
        line(n+'_Toe', a+Vector((0,0,.08)), z+Vector((0,0,.08)), .045, 'dark', c)

    def opening_all(n, bounds, z, prefixes=('Deck_',)):
        bpy.context.view_layer.update()
        x0,x1,y0,y1 = bounds
        for o in list(bpy.data.objects):
            if o.type == 'MESH' and o.name.startswith(prefixes):
                # Boolean only meshes with overlapping evaluated WORLD bounds.
                pts = [o.matrix_world @ Vector(v) for v in o.bound_box]
                lo,hi = [min(p[k] for p in pts) for k in range(3)], [max(p[k] for p in pts) for k in range(3)]
                if hi[0]>x0 and lo[0]<x1 and hi[1]>y0 and lo[1]<y1 and hi[2]>z-.24 and lo[2]<z+.24:
                    cut(o.name, 'R3_Cut_'+n, ((x0+x1)/2,(y0+y1)/2,z), (x1-x0,y1-y0,.48))

    def light(n, x,y,z, watts=90):
        b(n+'_Base',(x,y,z), (1.12,.3,.08),'dark')
        b(n+'_Diffuser',(x,y,z-.045),(1.0,.24,.02),'light')
        obj=g['area_light']('R3_Light_'+n,(x,y,z-.065),watts,1.0,(1,.88,.72))
        obj.data.use_shadow=True

    def door(n,x,y,z,axis='Y',width=1.2):
        # Thin interior frame, clear opening 1.20 x 2.16 m; leaf parked against wall,
        # not at 90 degrees across the room/route. Coordinates refer to finished floor.
        for s in (-1,1):
            p=(x+s*(width/2+.035),y,z+1.1) if axis=='Y' else (x,y+s*(width/2+.035),z+1.1)
            b(n+'_Jamb'+str(s),p,(.07,.18,2.2) if axis=='Y' else (.18,.07,2.2),'steel')
        b(n+'_Lintel',(x,y,z+2.22),(width+.14,.18,.08) if axis=='Y' else (.18,width+.14,.08),'steel')
        if axis=='Y':
            b(n+'_OpenLeaf',(x+width+.12,y+.13,z+1.05),(width-.08,.055,2.1),'blue')
            b(n+'_Handle',(x+width+.52,y+.087,z+1.03),(.16,.035,.03),'steel')
        else:
            b(n+'_OpenLeaf',(x+.13,y+width+.12,z+1.05),(.055,width-.08,2.1),'blue')

    def wall_y(n,x0,x1,y,floor,ceiling,doors=(),windows=()):
        # Construct around apertures: no coincident glass sticker on an opaque wall.
        apertures=[(x-.6,x+.6,floor,floor+2.18,'D',x) for x in doors]
        apertures += [(x-.63,x+.63,floor+1.05,ceiling-.28,'W',x) for x in windows]
        cursor=x0
        for i,(a,z,low,high,kind,center) in enumerate(sorted(apertures)):
            if a<cursor or z>x1: continue
            if a>cursor:
                b(n+'_Pier'+str(i),((cursor+a)/2,y,(floor+ceiling)/2),(a-cursor,.14,ceiling-floor))
            if low>floor:
                b(n+'_Sill'+str(i),(center,y,(floor+low)/2),(z-a,.14,low-floor))
            b(n+'_Head'+str(i),(center,y,(high+ceiling)/2),(z-a,.14,ceiling-high))
            if kind=='D': door(n+'_Door'+str(i),center,y,floor)
            else:
                b(n+'_Glass'+str(i),(center,y,(low+high)/2),(z-a,.022,high-low),'glass',bevel=.003)
                for q in (a,z): b(n+'_WindowMullion'+str(q),(q,y,(low+high)/2),(.045,.19,high-low),'steel')
                for h in (low,high): b(n+'_WindowRail'+str(h)+str(i),(center,y,h),(z-a+.08,.19,.045),'steel')
            cursor=z
        if cursor<x1:
            b(n+'_End',((cursor+x1)/2,y,(floor+ceiling)/2),(x1-cursor,.14,ceiling-floor))

    # Painted surfaces are paint, not highly reflective bare metal.
    for key in ('white','blue','hull','orange','yellow'):
        node=M[key].node_tree.nodes.get('Principled BSDF')
        node.inputs['Metallic'].default_value=.08
        node.inputs['Roughness'].default_value=.46 if key=='hull' else .58
    glass=M['glass'].node_tree.nodes.get('Principled BSDF')
    glass.inputs['Base Color'].default_value=(.24,.46,.54,1)
    glass.inputs['Alpha'].default_value=.13
    glass.inputs['Roughness'].default_value=.12
    M['glass'].surface_render_method='DITHERED'
    M['screen'].node_tree.nodes.get('Principled BSDF').inputs['Emission Strength'].default_value=.45
    M['floor']=g['material']('R3_MarineVinyl',(.15,.20,.22,1),.02,.8)
    M['linen']=g['material']('R3_Linen',(.46,.52,.47,1),0,.95)
    M['light']=g['material']('R3_WarmDiffuser',(.85,.8,.65,1),0,.7,(1,.87,.65,1))
    M['display']=g['material']('R3_DisplayBlack',(.002,.009,.012,1),0,.62,(.002,.014,.018,1))
    steel_nodes=M['steel'].node_tree.nodes
    steel_links=M['steel'].node_tree.links
    steel_bsdf=steel_nodes.get('Principled BSDF')
    rough_input=steel_bsdf.inputs['Roughness']
    if rough_input.is_linked:
        source=rough_input.links[0].from_socket
        limiter=steel_nodes.new('ShaderNodeMath');limiter.operation='MAXIMUM';limiter.inputs[1].default_value=.55
        steel_links.new(source,limiter.inputs[0]);steel_links.new(limiter.outputs[0],rough_input)
    else: rough_input.default_value=.55

    # Remove independent furniture/layout generators, retaining the rest of the vessel.
    remove('BoatDeck_','Accommodation_','Officers_','Bridge_', 'BridgeWing', 'AccessTower_',
           'Cabin_','Officer_','Mess_','Galley_','Medical_','Laundry','Sanitary_','Crew_0','Crew_1',
           'Toilet_Crew','Washbasin_Crew','Shower_Crew','Funnel','DeckSeam_','Bollard_1_',
           'InteriorLight_Accommodation', 'Hull_Complete_LOD','Accommodation_HLOD','CargoZone_HLOD','BowEquipment_HLOD')
    # Restore the old wrongly placed holes, then author only validated openings.
    remove('Deck_Lower','Deck_Tween','Deck_Weather')
    for name,z in (('Lower',4.5),('Tween',7.25),('Weather',10.5)):
        g['deck_with_holds']('Deck_'+name,z)
    for obj in bpy.data.objects:
        if obj.name.startswith('Aft_Windlass'):
            obj.location += Vector((-4.8,3.3,0))
        if obj.name.startswith(('Vent_0_','Vent_1_')): obj.location.x-=2.0
        if obj.name.startswith(('Vent_2_','Vent_3_')): obj.location.x+=2.4
        for index,side in ((2,-1),(3,1)):
            if obj.name.startswith('Fairlead_'+str(index)+'_'):
                old_y=side*g['half_beam_at'](-49)*.78
                obj.location += Vector((-10.5,side*5.8-old_y,0))
        if obj.name.startswith('ServiceTank'): obj.location.x+=4.5
        if obj.name.startswith('CargoCrane_0_'): obj.location += Vector((-17.,-8.25,0))
        if obj.name.startswith('CargoCrane_1_'): obj.location += Vector((-19.,-8.25,0))
        if obj.name.startswith(('FireStation_2_','FireStation_3_')):
            obj.location.y += .9 if obj.location.y<0 else -.9
        if obj.name=='Propeller_Shaft': obj.location.z+=.6
        if obj.name.startswith('Shaft_Bearing_'): obj.location.z+=.36
        if obj.name.startswith('SteeringGear_HydraulicPump_'): obj.location.x-=3.6
        if obj.name.startswith('BilgeFirePump_1_'): obj.location.y+=1.4
    # Tank-top plating follows the narrower lower hull, not the weather beam.
    remove('Deck_Tank_Top')
    g['deck_plate']('Deck_Tank_Top',-60.8,58,-.86,.86,2.0,.16,M['deck'],decks)
    g['deck_plate']('R3_SternWeatherDeck',-60.8,-57,-.94,.94,10.5,.16,M['deck'],decks)
    # At the narrowing aft corner the original diagonal rail crossed the aft
    # rooms. Terminate it against the outside aft bulkhead instead.
    for side in (-1,1):
        remove(f'RailRun_{side}_11.25_0',f'RailRun_{side}_11.65_0')
        guard('AftCornerGuard'+str(side),(-56,side*g['half_beam_at'](-56)*.91,10.58),(-55.1,side*7.5,10.58))
    # Aft, dedicated uptake outside living rooms; no disguised blocked corridor.
    b('ExhaustUptake',(-57.7,0,16.3),(2.0,2.2,11.4),'white',C['70_NAVIGATION'],.1)
    g['tapered_volume']('R3_Funnel',-57.7,0,20.8,24.8,(2.2,2.6),(1.65,2.1),-.5,M['dark'],C['70_NAVIGATION'],.12)
    for y in (-.65,.65): cyl('R3_ExhaustOutlet'+str(y),(-58.15,y,25.2),.3,1.0,M['dark'],C['70_NAVIGATION'],vertices=32)
    g['curve_tube']('R3_ExhaustTrunk',[(-40,1.18,8.05),(-54,1.18,8.05),(-57.7,0,8.05),(-57.7,0,21.2)],.29,M['dark'],machinery)
    # Obsolete collision filled holes and contained the wrong centre-deck geometry.
    # Do not export misleading UCX. New UE collision remains an explicit acceptance gate.
    remove('UCX_')
    bpy.context.scene['collision_status']='No export collision: R3 must be validated in a separate Unreal map'

    levels=[('Boat',10.67,13.17,-55,-31,7.7),('Crew',13.42,15.92,-55,-31,7.6),
            ('Officers',16.17,18.67,-52.5,-31.5,7.2),('Bridge',18.92,21.92,-49.5,-31.5,6.6)]
    for idx,(name,f,ce,x0,x1,w) in enumerate(levels):
        b(name+'_Floor',((x0+x1)/2,0,f-.09),(x1-x0,w*2,.18),'floor')
        b(name+'_Ceiling',((x0+x1)/2,0,ce+.04),(x1-x0,w*2,.08))
        if idx>0:
            opening_all(name+'_StairVoid',(-48.15,-42.45,3.03,6.30),f-.09,('R3_'+name+'_Floor',))
        if idx<3:
            opening_all(name+'_CeilingVoid',(-48.15,-42.45,3.03,6.30),ce+.04,('R3_'+name+'_Ceiling',))
        b(name+'_AftWall',(x0,0,(f+ce)/2),(.16,w*2,ce-f))
        if name!='Bridge':
            b(name+'_FrontWall',(x1,0,(f+ce)/2),(.16,w*2,ce-f))
            for side in (-1,1):
                wx=[x for x in (x0+2,-46.2,-39.3) if x<x1-4]
                wall_y(name+'_Outer'+str(side),x0,x1,side*w,f,ce,(-32.7,),wx)
            if name=='Boat':
                port=[(x0+.12,-43.3,'MESS'),(-43.3,-36,'WASHROOM')]
                star=[(x0+.12,-49.15,'LAUNDRY'),(-41.8,-36,'GALLEY')]
            elif name=='Crew':
                port=[(a,z,'CABIN '+str(i+1)) for i,(a,z) in enumerate(zip((x0+.12,-50.1,-45.3,-40.5),(-50.1,-45.3,-40.5,-36)))]
                star=[(x0+.12,-49.15,'MEDICAL'),(-41.8,-38.9,'CABIN 5'),(-38.9,-36,'CABIN 6')]
            else:
                port=[(x0+.12,-46.8,'OFFICER 1'),(-46.8,-41.4,'OFFICER 2'),(-41.4,-36,'OFFICER 3')]
                star=[(x0+.12,-49.15,'RADIO'),(-41.8,-36,'CAPTAIN')]
            for side,ranges in ((-1,port),(1,star)):
                for r,(a,z,title) in enumerate(ranges):
                    center=(a+z)/2
                    wall_y(name+'_Corridor'+str(side)+'_'+str(r),a,z,side*1.2,f,ce,(center,))
                    b(name+'_Partition'+str(side)+'_'+str(r),(z,side*(w+1.2)/2,(f+ce)/2),(.12,w-1.2,ce-f))
                    # Continuous skirting follows walls, never crossing doors.
                    b(name+'_Skirting'+str(side)+'_'+str(r),(center,side*(w-.09),f+.075),(z-a,.025,.15),'dark')
                    light(name+'_Room'+str(side)+'_'+str(r),center,side*(w+1.2)/2,ce-.08,75)
                    label(name+'_RoomLabel'+str(side)+'_'+str(r),title,(center,side*1.09,f+2.32),.12,0 if side==1 else math.pi,'dark')
                    rooms.append(dict(name=name+' '+title,x0=a,x1=z,side=side,width=w,floor=f,door=(center,side*1.2,f)))
            # Dedicated stair lobby on starboard; not a cabin with stairs pushed into it.
            wall_y(name+'_StairLobby',-49.15,-41.8,1.2,f,ce,(-48.45,))
            for x in (-49.15,-41.8):
                b(name+'_StairEnd'+str(x),(x,(w+1.2)/2,(f+ce)/2),(.12,w-1.2,ce-f))
        else:
            for side in (-1,1):
                wall_y(name+'_Side'+str(side),x0,x1,side*w,f,ce,(-32.7,),(-46.2,-42.8,-39.3,-36.6))
            # Forward wheelhouse: sill and roof header with seven real openings.
            b('BridgeFront_Sill',(-31.5,0,f+.46),(.16,13.2,.92))
            b('BridgeFront_Header',(-31.5,0,ce-.12),(.16,13.2,.24))
            for i in range(8):
                y=-6.6+i*13.2/7
                b('BridgeFront_Post'+str(i),(-31.5,y,(f+.92+ce-.24)/2),(.18,.10,ce-.24-f-.92),'steel')
            for i in range(7):
                y=-6.6+(i+.5)*13.2/7
                b('BridgeFront_Glass'+str(i),(-31.5,y,20.32),(.022,1.78,1.88),'glass',bevel=.003)
            b('BridgeAftBulkhead',(-41.6,4.2,20.42),(.14,4.8,3.0))
            for x,y in ((-38,-3.8),(-38,3.8),(-46,0)): light('Bridge'+str(x)+str(y),x,y,ce-.08,60)
        if name!='Bridge':
            for x in range(math.ceil(x0+2),-32,5): light(name+'_Hall'+str(x),x,0,ce-.07,45)
            # Cross-lobby stays free all the way to both exterior doors.
            for s in (-1,1):
                label(name+'_Exit'+str(s),'EXIT / STAIRS',(-33.4,s*(w-.1),f+2.28),.14,0 if s==1 else math.pi,'pipe_green')
        if name=='Bridge':
            routes.append(dict(name=name+'_corridor',points=[(x0+.8,0,f),(-36.0,0,f)]))
            routes.append(dict(name=name+'_cross_lobby',points=[(-32.7,-5.85,f),(-36,-5.85,f),(-36,5.85,f),(-32.7,5.85,f)]))
        else:
            routes.append(dict(name=name+'_corridor',points=[(x0+.8,0,f),(-32.7,0,f)]))
            routes.append(dict(name=name+'_cross_lobby',points=[(-32.7,-w+.25,f),(-32.7,w-.25,f)]))
        light(name+'_Stairwell',-45.5,2.35,ce-.08,110)

    # Shared interior companionway: 2 flights per tier, connected mid-landings.
    for idx in range(3):
        f, top = levels[idx][1],levels[idx+1][1]
        mid=(f+top)/2
        prefix='R3_InternalStair'+str(idx)
        g['stairs'](prefix+'_Up',(-48.15,3.77,f),(1,0),mid-f,4.8,steps=8,width=1.34)
        g['stairs'](prefix+'_Return',(-43.35,5.57,mid),(-1,0),top-mid,4.8,steps=8,width=1.34)
        b('StairTurn'+str(idx),(-42.90,4.67,mid-.075),(.9,3.2,.15),'diamond',decks)
        guard('StairTurnGuard'+str(idx),(-42.46,3.07,mid),(-42.46,6.27,mid))
        guard('StairCentral'+str(idx),(-48.15,4.66,f),(-43.35,4.66,mid),.95)
        routes.append(dict(name='interior_stair_'+str(idx),points=[(-48.15,3.77,f),(-43.35,3.77,mid),(-42.9,3.77,mid),(-42.9,5.57,mid),(-43.35,5.57,mid),(-48.15,5.57,top)]))
    # Landing access fits 0.95m at aft end; widened local floor to avoid edge balancing.
    for i,level in enumerate(levels):
        f=level[1]
        b('StairAftLanding'+str(i),(-48.64,4.65,f-.075),(.98,3.3,.15),'diamond',decks)
        routes.append(dict(name='stair_lobby_'+str(i),points=[(-48.65,0,f),(-48.65,5.57,f)]))

    # Furnish by room bounds, never by an unrelated global grid.
    for i,r in enumerate(rooms):
        a,z,s,w,f=r['x0'],r['x1'],r['side'],r['width'],r['floor']
        x=(a+z)/2; tag='R3_Room'+str(i); title=r['name']
        if any(t in title for t in ('CABIN','OFFICER','CAPTAIN','MEDICAL')):
            g['crew_bunk'](tag+'_Berth',x,s*(w-.65),f,s)
            # Desk on outer half, locker at opposite end; >1.2m central aisle remains.
            g['desk_station'](tag+'_Desk',x,s*(w-2.1),f,s)
            g['locker'](tag+'_Locker',a+.65,s*2.05,f,s)
            b(tag+'_BedHead',(x-1.04,s*(w-.65),f+.62),(.08,.88,1.0),'wood')
            b(tag+'_BedStorage',(x,s*(w-.65),f+.19),(1.7,.70,.24),'wood')
            for t in (-.48,.48): b(tag+'_DrawerPull'+str(t),(x+t,s*(w-1.02),f+.22),(.16,.035,.025),'steel')
            b(tag+'_Pinboard',(x,s*(w-.095),f+1.48),(1.05,.04,.6),'wood')
            label(tag+'_BerthID',title,(x,s*(w-.125),f+1.6),.1,0 if s==1 else math.pi)
        elif 'MESS' in title:
            for table,x in enumerate((a+2.1,a+7.0,a+10.1)):
                b(tag+'_Table'+str(table),(x,-4.8,f+.77),(2.0,1.0,.09),'wood')
                for dx in (-.7,.7):
                    for y in (-5.15,-4.45): line(tag+'_Leg'+str((table,dx,y)),(x+dx,y,f),(x+dx,y,f+.72),.035)
                for dx in (-.6,.6):
                    for y in (-5.7,-3.9):
                        before=set(bpy.data.objects)
                        g['chair'](tag+'_Chair'+str((table,dx,y)),x+dx,y,f)
                        # Chair faces table along Y, not sideways.
                        angle=math.pi/2 if y<-4.8 else -math.pi/2
                        for o in set(bpy.data.objects)-before:
                            p=o.location-Vector((x+dx,y,f)); p.rotate(__import__('mathutils').Matrix.Rotation(angle,3,'Z'))
                            o.location=Vector((x+dx,y,f))+p; o.rotation_euler.z+=angle
                for q in (-.5,.5): cyl(tag+'_Plate'+str((table,q)),(x+q,-4.8,f+.83),.16,.015,M['white'],structure,vertices=24)
            b(tag+'_Sideboard',(-53.8,-(w-.55),f+.5),(1.4,.7,1),'wood')
        elif 'GALLEY' in title:
            for j,x in enumerate((-40.6,-39.2,-37.8)):
                b(tag+'_Base'+str(j),(x,w-.62,f+.47),(1.3,.82,.94),'steel')
                b(tag+'_Counter'+str(j),(x,w-.62,f+.98),(1.36,.9,.07),'steel')
                for dx in (-.31,.31):
                    b(tag+'_Door'+str((j,dx)),(x+dx,w-1.045,f+.48),(.59,.03,.83),'white')
                    b(tag+'_Pull'+str((j,dx)),(x+dx,w-1.075,f+.77),(.18,.03,.03),'dark')
            for dx in (-.3,.3):
                for dy in (-.21,.21): g['torus'](tag+'_Burner'+str((dx,dy)),(-39.2+dx,w-.62+dy,f+1.03),.14,.018,M['dark'],structure)
            b(tag+'_Hood',(-39.2,w-.5,f+2.15),(1.55,1.05,.26),'steel')
            b(tag+'_Duct',(-39.2,w-.35,f+2.4),(.4,.4,.3),'steel')
            b(tag+'_SinkRim',(-40.6,w-.62,f+1.025),(.85,.55,.025),'dark')
            line(tag+'_Tap',(-40.6,w-.33,f+1.02),(-40.6,w-.33,f+1.3),.018)
            line(tag+'_Spout',(-40.6,w-.33,f+1.3),(-40.6,w-.56,f+1.3),.018)
            b(tag+'_Fridge',(-37,3,f+1.03),(.85,.8,2.06),'white')
            b(tag+'_FridgeHandle',(-37.32,2.57,f+1.2),(.03,.04,.4),'steel')
            b(tag+'_Prep',(-39,4.6,f+.92),(2.0,.85,.08),'steel')
            for xx in (-39.8,-38.2):
                for yy in (4.3,4.9): line(tag+'_PrepLeg'+str((xx,yy)),(xx,yy,f),(xx,yy,f+.9),.03)
        elif 'LAUNDRY' in title:
            for j in range(3):
                x=a+.9+j*1.25; y=w-.65
                b(tag+'_Washer'+str(j),(x,y,f+.46),(.95,.85,.92),'white')
                g['torus'](tag+'_WasherRim'+str(j),(x,y-.44,f+.43),.25,.045,M['steel'],structure,(math.pi/2,0,0))
                cyl(tag+'_WasherGlass'+str(j),(x,y-.44,f+.43),.205,.025,M['glass'],structure,(math.pi/2,0,0),32)
                b(tag+'_Controls'+str(j),(x,y-.438,f+.79),(.8,.03,.12),'dark')
            b(tag+'_FoldTable',(x,4.4,f+.9),(2.4,.8,.09),'wood')
            for xx in (x-1,x+1): b(tag+'_FoldSupport'+str(xx),(xx,4.4,f+.44),(.08,.65,.88),'steel')
        elif 'WASHROOM' in title:
            for j,x in enumerate((-42,-40.55)):
                g['toilet_module'](tag+'_WC'+str(j),x,-(w-.9),f,-1)
                b(tag+'_WCBase'+str(j),(x,-(w-.9),f+.18),(.32,.42,.36),'white')
                b(tag+'_CubicleSide'+str(j),(x+.70,-(w-1.5),f+1.02),(.06,2.25,2.04),'white')
                g['washbasin'](tag+'_Basin'+str(j),x,-3,f,-1)
            g['shower_module'](tag+'_Shower',-37.4,-(w-.75),f,-1)
        else:
            g['desk_station'](tag+'_Radio',x,s*(w-1.0),f,s)

    # Open wheelhouse: consoles face aft toward the operator, bow is +X.
    for i,y in enumerate((-4.25,-2.1,0,2.1,4.25)):
        b('BridgeConsole'+str(i),(-33.1,y,19.48),(1.05,1.8,1.04),'dark',C['70_NAVIGATION'],.06)
        # Slightly raked back console fascia; individual switchgear/real graphics geometry.
        b('ConsoleFascia'+str(i),(-33.64,y,19.84),(.06,1.72,.54),'blue',C['70_NAVIGATION'])
        b('ScreenBezel'+str(i),(-33.683,y,19.88),(.045,1.03,.49),'dark',C['70_NAVIGATION'])
        b('ScreenFace'+str(i),(-33.71,y,19.88),(.014,.94,.40),'display',C['70_NAVIGATION'])
        title=('RADAR / X BAND','ECDIS / ROUTE','HEADING / AUTO','PROPULSION','ALARMS')[i]
        label('DisplayTitle'+str(i),title,(-33.724,y,20.025),.029,-math.pi/2,'screen')
        if i in (0,1):
            for radius in (.055,.105,.15):
                g['torus']('R3_RadarRing'+str((i,radius)),(-33.725,y+.18,19.855),radius,.0018,M['screen'],C['70_NAVIGATION'],(0,math.pi/2,0))
            line('RadarSweep'+str(i),(-33.727,y+.18,19.855),(-33.727,y+.07,19.95),.002,'screen',C['70_NAVIGATION'])
            label('RadarReadout'+str(i),'RNG  3 NM\nCOG  082\nSOG  8.4',(-33.726,y-.19,19.95),.035,-math.pi/2,'screen')
        else:
            label('DisplayData'+str(i),('082.4 DEG\nTRACK   HOLD\nRUDDER  +02','RPM  124\nLOAD  62%\nOIL   4.2 BAR','SYSTEM  NORMAL\nBILGE  DRY\nFIRE   READY')[i-2],(-33.726,y,19.94),.043,-math.pi/2,'screen')
        for j in range(5):
            cyl('R3_ConsoleSwitch'+str((i,j)),(-33.695,y-.5+j*.25,19.59),.025,.04,M['orange' if j==4 else 'white'],C['70_NAVIGATION'],(0,math.pi/2,0),12)
    for i,y in enumerate((-2.1,2.1)):
        g['chair']('R3_BridgeHelmChair'+str(i),-35.1,y,18.92,1)
        cyl('R3_HelmPedestal'+str(i),(-35.1,y,19.12),.2,.4,M['dark'],C['70_NAVIGATION'],vertices=24)
    g['torus']('R3_HelmWheel',(-33.95,0,19.62),.27,.026,M['dark'],C['70_NAVIGATION'],(0,math.pi/2,0))
    for angle in range(0,360,120):
        a=math.radians(angle)
        line('HelmSpoke'+str(angle),(-33.95,0,19.62),(-33.95,.25*math.cos(a),19.62+.25*math.sin(a)))
    b('ChartTable',(-39.5,-4.9,19.88),(2.3,1.15,.12),'wood')
    for x in (-40.4,-38.6): b('ChartSupport'+str(x),(x,-4.9,19.38),(.10,.95,.92),'steel')
    b('ChartPaper',(-39.5,-4.9,19.95),(1.8,.9,.004),'white')
    for i in range(5): b('ChartContour'+str(i),(-39.5+i*.13,-4.9,19.954),(1.1-i*.15,.013,.004),'blue',bevel=0)
    routes.append(dict(name='bridge_operator',points=[(-40,0,18.92),(-36,0,18.92),(-36,-5.6,18.92)]))

    # External towers keep the SAME centre line at every tier. Wide transverse
    # landings connect the changing facade, with rails only at exposed edges.
    for side in (-1,1):
        y=side*9.8
        for i in range(4):
            f=levels[i][1]; top=levels[i+1][1] if i<3 else 22.08
            # Parallel flights, all +X. A separate outboard return gallery links
            # successive flights. Opposing flights in one lane collide overhead.
            sx=-38.2; dx=1
            g['stairs']('R3_Exterior'+str(side)+'_'+str(i),(sx,y,f),(dx,0),top-f,4.8,steps=18,width=1.35)
            w=levels[i][5]
            for x in (-38.97,-32.63):
                b('ExteriorLanding'+str((side,i,x)),(x,side*(w+12.15)/2,f-.08),(1.58,12.15-w+.2,.16),'diamond',decks)
            b('ExteriorReturn'+str((side,i)),(-35.8,side*11.35,f-.08),(6.5,1.6,.16),'diamond',decks)
            guard('ReturnOutside'+str((side,i)),(-39.76,side*12.15,f),(-31.84,side*12.15,f))
            guard('ReturnInside'+str((side,i)),(-38.18,side*10.55,f),(-33.42,side*10.55,f))
            for x in (-39.76,-31.84):
                guard('LandingSide'+str((side,i,x)),(x,side*w,f),(x,side*12.15,f))
            routes.append(dict(name='exterior_'+str(side)+'_'+str(i),points=[(sx,y,f),(sx+dx*4.8,y,top)]))
            routes.append(dict(name='exterior_return_'+str(side)+'_'+str(i),points=[(-32.63,side*9.8,f),(-32.63,side*11.35,f),(-38.97,side*11.35,f),(-38.97,side*9.8,f),(-38.2,side*9.8,f)]))
        for x in (-39.76,-31.84):
            line('TowerSupport'+str((side,x)),(x,side*12.15,10.4),(x,side*12.15,22.08),.085,'white',decks)
            line('TowerBrace'+str((side,x)),(x,side*9.25,7.7),(x,side*12.15,10.6),.08,'steel',decks)
        b('RoofLanding'+str(side),(-32.63,side*9.4,22.0),(1.58,5.7,.16),'diamond',decks)

    # Engine envelope is open over the tall propulsion engine. Remove the old
    # shelves through its cylinders; retain perimeter service galleries.
    remove('Stair_Aft','Stair_Forward','Engine_TankTopToLower','EngineEscape_', 'Opening_',
           'HoldAccess_', 'EngineCatwalk', 'EngineRoomBranch','EngineRoomValve', 'EngineRoomMain',
           'EngineManifold','InteriorLight_Engine','EngineWorkshop', 'EngineControl')
    for deck,z in (('Lower',4.5),('Tween',7.25)):
        opening_all('EngineAtrium'+deck,(-51.5,-37.8,-4.25,4.25),z)
    # Perimeter stairs are forward of machinery, and cut EVERY overlapping deck
    # mesh, not a named side walk which may not cover the chosen coordinates.
    for side in (-1,1):
        y=side*5.8
        for i,(f,top) in enumerate(((2.08,4.58),(4.58,7.33),(7.33,10.58))):
            sx=-31.5; run=5.4
            opening_all('ServiceStair'+str((side,i)),(sx-.12,sx+run+.02,y-.82,y+.82),top-.08)
            g['stairs']('R3_EngineAccess'+str((side,i)),(sx,y,f),(1,0),top-f,run,steps=18,width=1.4)
            b('ServiceUpperLanding'+str((side,i)),(-25.3,y,top-.08),(1.7,1.64,.16),'diamond',decks)
            for s in (-1,1): guard('ServiceVoid'+str((side,i,s)),(sx,y+s*.86,top),(sx+run,y+s*.86,top))
            if i==2: guard('ServiceVoidBack'+str((side,i)),(sx-.15,y-.86,top),(sx-.15,y+.86,top))
            routes.append(dict(name='engine_access_'+str(side)+'_'+str(i),points=[(sx,y,f),(sx+run,y,top),(-25.3,y,top)]))
    for f in (4.58,7.33):
        for side in (-1,1): guard('EngineGallery'+str((f,side)),(-51.5,side*4.25,f),(-37.8,side*4.25,f))
        for x in (-51.5,-37.8): guard('EngineGalleryEnd'+str((f,x)),(x,-4.25,f),(x,4.25,f))
    # Lower bulkheads split the original engine room; central doorway must face X.
    for o in list(bpy.data.objects):
        if o.name.startswith('Bulkhead_') and '_Door' in o.name: bpy.data.objects.remove(o,do_unlink=True)
    for x in (-52,-35,-20,10,14,42,52):
        for z0 in (2.08,4.58,7.33):
            y=0 if x < -20 or x>42 else 7.8
            z1={2.08:4.5,4.58:7.25,7.33:10.5}[z0]
            remove(f'Bulkhead_X{x}_{z0:.0f}_Header')
            b('BulkheadHeader'+str((x,z0)),(x,y,(z0+2.2+z1)/2),(.16,1.35,z1-z0-2.2),'steel',decks)
            if not (x==-52 and z0==2.08):
                door('BulkheadDoor'+str((x,z0)),x,y,z0,'X',1.35)
            if x in (-20,10,14,42):
                target=f'Bulkhead_X{x}_{z0:.0f}_Port'
                cut(target,'R3_PortAccess'+str((x,z0)),(x,-7.8,z0+1.10),(.5,1.35,2.20))
                door('PortBulkheadDoor'+str((x,z0)),x,-7.8,z0,'X',1.35)
    shaft_wall=b('ShaftPenetration',(-52,0,3.18),(.16,1.35,2.2),'steel',decks)
    cut(shaft_wall.name,'R3_ShaftOpening',(-52,0,3.15),(.5,.8,.8))
    g['torus']('R3_ShaftSeal',(-52.1,0,3.15),.38,.10,M['dark'],machinery,(0,math.pi/2,0))
    for obj in list(bpy.data.objects):
        if obj.name.startswith('Frame_') and any(abs(obj.location.x-x)<.3 for x in (-20,10,14,42)):
            bpy.data.objects.remove(obj,do_unlink=True)

    # Forward compartments keep their own vertical escape instead of depending
    # on returning the whole length of the ship to the engine stairs.
    for obj in bpy.data.objects:
        if obj.name.startswith('LowerDeck_ServiceLocker'):
            obj.location.y += 1.2 if obj.location.y>0 else -1.2
    for side in (-1,1):
        y=side*3.1
        for i,(f,top) in enumerate(((2.08,4.58),(4.58,7.33),(7.33,10.58))):
            sx=44.;run=5.4
            opening_all('ForepeakStair'+str((side,i)),(sx-.12,sx+run+.02,y-.82,y+.82),top-.08)
            g['stairs']('R3_ForepeakAccess'+str((side,i)),(sx,y,f),(1,0),top-f,run,steps=18,width=1.4)
            b('ForepeakLanding'+str((side,i)),(50.2,y,top-.08),(1.7,1.64,.16),'diamond',decks)
            for s in (-1,1): guard('ForepeakVoid'+str((side,i,s)),(sx,y+s*.86,top),(sx+run,y+s*.86,top))
            if i==2: guard('ForepeakBack'+str(side),(sx-.15,y-.86,top),(sx-.15,y+.86,top))
            routes.append(dict(name='forepeak_access_'+str(side)+'_'+str(i),points=[(sx,y,f),(sx+run,y,top),(50.2,y,top)]))
        for f in (4.58,7.33,10.58):
            pts=[(-25.3,side*5.8,f),(-25.3,side*8.6,f),(-21.2,side*8.6,f),(-20,side*7.8,f),
                 (-18.8,side*8.6,f),(8.8,side*8.6,f),(10,side*7.8,f),(11.2,side*8.6,f),
                 (12.8,side*8.6,f),(14,side*7.8,f),(15.2,side*8.6,f),(40.8,side*8.2,f),
                 (42,side*7.8,f),(43.2,side*8.2,f),(43.2,side*3.1,f),(44,side*3.1,f)]
            if f==10.58:
                pts=[(-25.3,side*5.8,f),(-25.3,side*8.6,f),(32,side*8.6,f),(43,side*8.2,f),(44.8,side*5.5,f),(50.2,side*5.5,f),(50.2,side*3.1,f)]
            if f==4.58:
                pts[1]=(-25.3,side*8.8,f);pts[2]=(-21.2,side*8.8,f)
            routes.append(dict(name='full_length_walk_'+str(side)+'_'+str(f),points=pts))

    # Engine crankcase inspection covers, fasteners, connected overhead headers.
    remove('MainEngine_Cylinder_')
    for i,x in enumerate((-47,-45.5,-44,-42.5,-41)):
        b('CylinderJacket'+str(i),(x,0,6.18),(1.34,3.6,1.52),'blue',machinery,.09)
        for side in (-1,1):
            b('CylinderInspection'+str((i,side)),(x,side*1.84,6.22),(.86,.075,.70),'steel',machinery,.045)
            label('CylinderID'+str((i,side)),str(i+1),(x,side*1.89,6.21),.20,0 if side<0 else math.pi)
            g['curve_tube']('R3_InjectorLine'+str((i,side)),[(x,side*1.9,5.65),(x-.35,side*1.9,5.65),(x-.35,side*1.9,7.14),(x,side*.9,7.14)],.035,M['brass'],machinery)
    cyl('R3_EngineFlywheel',(-49.0,0,3.7),1.52,.48,M['dark'],machinery,(0,math.pi/2,0),48)
    g['torus']('R3_FlywheelRim',(-49.26,0,3.7),1.35,.09,M['steel'],machinery,(0,math.pi/2,0))
    b('ReductionGearbox',(-50.1,0,3.3),(1.6,2.25,2.15),'blue',machinery,.15)
    line('GearboxOutput',(-50.95,0,3.15),(-51.8,0,3.15),.29,'steel',machinery)
    for side in (-1,1):
        for i,x in enumerate((-47,-45.5,-44,-42.5,-41)):
            b('CrankcaseCover'+str((side,i)),(x,side*2.035,4.0),(1.08,.07,1.28),'steel',machinery,.055)
            for xx in (-.4,.4):
                for zz in (-.49,.49):
                    cyl('R3_CoverBolt'+str((side,i,xx,zz)),(x+xx,side*2.09,4+zz),.027,.025,M['dark'],machinery,(math.pi/2,0,0),6)
            line('CylinderManifoldLink'+str((side,i)),(x,.65,8.05),(x,1.18,8.05),.16,'dark',machinery)
        # All four straight service mains routed overhead on brackets, no diagonal stubs.
        for j,mat in enumerate(('pipe_green','pipe_red')):
            y=side*(8.0+j*.28); z=6.9
            line('ServiceMain'+str((side,j)),(-54,y,z),(-24,y,z),.08,mat,machinery)
            for x in (-54,-24):
                line('ServiceDown'+str((side,j,x)),(x,y,z),(x,y,2.5),.08,mat,machinery)
                b('ServiceFlange'+str((side,j,x)),(x,y,2.5),(.27,.27,.12),'steel',machinery)
            for x in range(-54,-23,3):
                line('PipeBracket'+str((side,j,x)),(x,side*8.85,z-.12),(x,y,z-.12),.025,'steel',machinery)
        for x in (-49,-43,-38,-28): light('Engine'+str((side,x)),x,side*5.3,7.0,160)
    # Reorient control station towards the clear aisle, with legible panel grouping.
    b('EngineControl',(-32.9,-7.7,3.05),(3.0,.65,1.8),'blue',machinery,.06)
    for i,x in enumerate((-33.8,-32.9,-32)):
        b('EngineControlBezel'+str(i),(x,-7.36,3.48),(.72,.035,.46),'dark',machinery)
        b('EngineControlDisplay'+str(i),(x,-7.33,3.48),(.63,.014,.37),'screen',machinery)
    # Explicit full-height opening through both cross-decks to the engine atrium.
    # Retain machinery coordinates otherwise; steering/auxiliary topology remains audited separately.
    for side in (-1,1):
        target='Bulkhead_X-52_2_'+('Port' if side<0 else 'Starboard')
        cut(target,'R3_SternServiceDoor'+str(side),(-52,side*4.3,3.18),(.5,1.35,2.2))
        door('SternServiceDoor'+str(side),-52,side*4.3,2.08,'X',1.35)
        routes.append(dict(name='engine_tanktop_service_'+str(side),points=[(-31.5,side*5.8,2.08),(-33,side*3.1,2.08),(-33,0,2.08),(-36.5,0,2.08),(-37.5,side*3.3,2.08),(-50,side*3.3,2.08),(-51,side*4.3,2.08),(-53.2,side*4.3,2.08),(-56,side*4.3,2.08)]))

    # Holds receive independent three-tier stair towers rather than 8m vertical
    # ladders trapped between plates. The adjacent hatch bay is visibly OPEN.
    for hold,x0,x1,side in ((1,-19.5,-12.5,-1),(2,34.5,41.5,1)):
        y0,y1=(-6.8,-3.25) if side<0 else (3.25,6.8)
        for o in list(bpy.data.objects):
            if o.name.startswith('Hold'+str(hold)+'_Hatch'):
                px=o.location.x
                if (hold==1 and px < -10) or (hold==2 and px>32): bpy.data.objects.remove(o,do_unlink=True)
            elif o.name.startswith('Hold'+str(hold)+'_InternalFrame') and x0-.4<o.location.x<x1+.4:
                bpy.data.objects.remove(o,do_unlink=True)
        ys=(side*4.08,side*5.97)
        for i,(f,top) in enumerate(((2.08,4.58),(4.58,7.33),(7.33,10.58))):
            # One run per level, alternating lane and direction; mid-height frames do not cross it.
            sx=x0+.85 if i%2==0 else x1-.85
            dx=1 if i%2==0 else -1; y=ys[i%2]
            run=x1-x0-1.7
            g['stairs']('R3_HoldStair'+str((hold,i)),(sx,y,f),(dx,0),top-f,run,steps=18,width=1.35)
            for xx in (x0+.38,x1-.38):
                b('HoldLanding'+str((hold,i,xx)),(xx,(y0+y1)/2,top-.08),(.83,y1-y0,.16),'diamond',decks)
                guard('HoldLandingEnd'+str((hold,i,xx)),(xx+( -.38 if xx<x0+1 else .38),y0,top),(xx+(-.38 if xx<x0+1 else .38),y1,top))
            # Lane not occupied by the incoming stair is a usable return gallery.
            free_y=ys[1-i%2]
            b('HoldReturnGallery'+str((hold,i)),((x0+x1)/2,free_y,top-.08),(run,1.5,.16),'diamond',decks)
            guard('HoldInnerGuard'+str((hold,i)),(x0,side*3.24,top),(x1,side*3.24,top))
            routes.append(dict(name='hold_'+str(hold)+'_'+str(i),points=[(sx,y,f),(sx+dx*run,y,top)]))
        # Side coaming has a dedicated open entrance at the end landing.
        entrance_x=x1-.4
        target='Hold'+str(hold)+'_CoamingSide_'+str(side)
        cut(target,'R3_HoldEntrance'+str(hold),(entrance_x,side*7.35,11.3),(1.15,.65,2.2))
        b('HoldWeatherBridge'+str(hold),(entrance_x,side*7.18,10.50),(1.15,1.2,.16),'diamond',decks)
        for s in (-1,1): guard('HoldEntranceGuard'+str((hold,s)),(entrance_x+s*.57,side*6.65,10.58),(entrance_x+s*.57,side*7.77,10.58))
        label('HoldStairSign'+str(hold),'HOLD '+str(hold)+' / ACCESS',(entrance_x,side*7.31,12.0),.16,0 if side>0 else math.pi)
        # Low-level stiffeners and supported lashing pads, kept off the passage.
        for xx in range(-16 if hold==1 else 18,9 if hold==1 else 32,4):
            for s in (-1,1):
                b('HoldPad'+str((hold,xx,s)),(xx,s*6.85,2.2),(.5,.32,.12),'steel',decks)
        routes.append(dict(name='hold_'+str(hold)+'_tanktop',points=[((-10 if hold==1 else 18),0,2.08),((6 if hold==1 else 30),0,2.08)]))
        routes.append(dict(name='hold_'+str(hold)+'_weather_entry',points=[(entrance_x,side*8.6,10.58),(entrance_x,side*4.08,10.58),(x1-.85,side*4.08,10.58)]))

    # Strip human-sized high floating decorative rings in holds; retain floor fittings.
    for o in list(bpy.data.objects):
        if '_LashingEye_' in o.name and o.location.z>3: bpy.data.objects.remove(o,do_unlink=True)
    bpy.context.scene['refit_revision']='R3-20260905'
    bpy.context.scene['route_contracts']=json.dumps(routes)
    bpy.context.scene['room_layout']=json.dumps(rooms)
    bpy.context.scene['remaining_gates']='Unreal capsule/collision, maritime machinery detail, UV/LOD, full visual QA; NOT production complete'
    bpy.context.scene.eevee.shadow_pool_size='1024'
    print('CALMY_R3_REFIT rooms='+str(len(rooms))+' routes='+str(len(routes)),flush=True)
