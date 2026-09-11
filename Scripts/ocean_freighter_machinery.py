"""R4 machinery art pass. Original game assets, not a certified plant design.

Runs AFTER the R3 arrangement. Shared shaft axes, foundations and pipe endpoints
are explicit; route contracts are retained and extended with service aisles.
No third-party mesh, brand logo or performance rating is copied.
"""
import bpy
import math
import json
from mathutils import Vector


def apply_machinery(g):
    M = g['MAT']; C = g['COL']['40_MACHINERY_SYSTEMS']
    def remove(*prefixes):
        for o in list(bpy.data.objects):
            if o.name.startswith(prefixes): bpy.data.objects.remove(o, do_unlink=True)
    def b(n,p,d,m='blue',edge=.02):
        return g['box']('R4_'+n,p,d,M[m],C,edge)
    def c(n,p,r,h,m='steel',axis='Z',v=24):
        rot={'X':(0,math.pi/2,0),'Y':(math.pi/2,0,0),'Z':(0,0,0)}[axis]
        return g['cylinder']('R4_'+n,p,r,h,M[m],C,rot,v)
    def rod(n,a,z,r=.025,m='steel',v=16):
        return g['cylinder_between']('R4_'+n,a,z,r,M[m],C,v)
    def ring(n,p,r,t=.018,m='steel',axis='Z'):
        rot={'X':(0,math.pi/2,0),'Y':(math.pi/2,0,0),'Z':(0,0,0)}[axis]
        return g['torus']('R4_'+n,p,r,t,M[m],C,rot)
    def text(n,body,p,size=.07,front=-1,m='white'):
        data=bpy.data.curves.new('R4_'+n,'FONT');data.body=body;data.size=size
        data.align_x='CENTER';data.align_y='CENTER';data.extrude=.0005
        o=bpy.data.objects.new('R4_'+n,data);C.objects.link(o);o.location=p
        o.rotation_euler=(math.pi/2,0,0 if front<0 else math.pi)
        data.materials.append(M[m]);return o
    def pipe(n,points,r=.05,m='pipe_green',bend=.14):
        # Rounded polygon elbows, tangential to adjoining straights. Unlike a
        # polyline with sharp 90-degree mitres, the swept tube has real bends.
        pts=[Vector(p) for p in points]; smooth=[pts[0]]
        for a,p,z in zip(pts,pts[1:],pts[2:]):
            trim=min(bend,(p-a).length*.4,(z-p).length*.4)
            enter=p+(a-p).normalized()*trim;leave=p+(z-p).normalized()*trim
            smooth.append(enter)
            for i in range(1,9):
                t=i/8;smooth.append((1-t)**2*enter+2*(1-t)*t*p+t*t*leave)
        smooth.append(pts[-1])
        o=g['curve_tube']('R4_'+n,smooth,r,M[m],C);o.data.use_fill_caps=True
        return o
    def flange(n,p,r=.13,axis='Z'):
        c(n+'Plate',p,r,.055,'steel',axis,40)
        # Bolt circle in flange plane, six physical hex fasteners.
        p=Vector(p);axes={'X':(Vector((0,1,0)),Vector((0,0,1))),
                         'Y':(Vector((1,0,0)),Vector((0,0,1))),
                         'Z':(Vector((1,0,0)),Vector((0,1,0)))}
        u,v=axes[axis]
        for i in range(6):
            q=p+r*.76*(math.cos(i*math.tau/6)*u+math.sin(i*math.tau/6)*v)
            c(n+'Bolt'+str(i),q,.013,.075,'dark',axis,6)
    def gauge(n,p,front=-1):
        x,y,z=p;c(n+'Housing',p,.094,.06,'steel','Y')
        c(n+'Dial',(x,y+front*.037,z),.080,.008,'white','Y')
        for i in range(9):
            a=math.radians(-40+i*32)
            rod(n+'Tick'+str(i),(x+math.cos(a)*.055,y+front*.043,z+math.sin(a)*.055),
                (x+math.cos(a)*.071,y+front*.043,z+math.sin(a)*.071),.0025,'dark',6)
        rod(n+'Needle',(x,y+front*.047,z),(x+.027,y+front*.047,z+.035),.003,'red',8)
        c(n+'Pin',(x,y+front*.05,z),.008,.006,'dark','Y')
    def wheel(n,p,r=.11,axis='Z'):
        ring(n+'Rim',p,r,.014,'orange',axis)
        u,v={'Z':(Vector((1,0,0)),Vector((0,1,0))),
             'Y':(Vector((1,0,0)),Vector((0,0,1)))}[axis]
        for i in range(3): rod(n+'Spoke'+str(i),p,Vector(p)+r*(math.cos(i*math.tau/3)*u+math.sin(i*math.tau/3)*v),.01)
    def foot(n,x,y,z,w=.22):
        b(n+'Plate',(x,y,z+.025),(w,.24,.05),'steel',.006)
        c(n+'Mount',(x,y,z+.09),.075,.08,'dark')
        for dx in (-w*.34,w*.34): c(n+'Bolt'+str(dx),(x+dx,y,z+.06),.019,.055,'steel','Z',6)

    # Two independent diesel/alternator sets; all rotating parts share X axis.
    remove('Generator_')
    for i,side in enumerate((-1,1)):
        x=-46.;y=side*6.;z=3.02;n='DG'+str(i+1)+'_';front=-side
        b(n+'DripPan',(x,y,2.13),(4.35,1.95,.10),'dark')
        for dy in (-.89,.89): b(n+'PanLip'+str(dy),(x,y+dy,2.22),(4.35,.06,.13),'steel')
        for dy in (-.55,.55):
            b(n+'BedRail'+str(dy),(x,y+dy,2.37),(4.15,.18,.26),'steel')
            for dx in (-1.65,-.45,.8,1.65): foot(n+'Mount'+str((dx,dy)),x+dx,y+dy,2.18)
        # Diesel aft, alternator forward, with enclosed aligned coupling.
        b(n+'Sump',(x-.72,y,2.63),(2.12,.84,.26),'dark')
        b(n+'Crankcase',(x-.72,y,3.0),(2.08,1.03,.62),'blue',.06)
        for k,dx in enumerate((-1.51,-1.0,-.49,.02)):
            b(n+'Cylinder'+str(k),(x+dx,y,3.40),(.45,.90,.42),'blue',.04)
            b(n+'RockerCover'+str(k),(x+dx,y,3.66),(.43,.80,.14),'steel',.035)
            for s in (-1,1):
                b(n+'CrankDoor'+str((k,s)),(x+dx,y+s*.527,3.03),(.39,.055,.41),'steel')
                for dz in (-.14,.14):
                    c(n+'DoorBolt'+str((k,s,dz)),(x+dx,y+s*.56,3.03+dz),.014,.035,'dark','Y',6)
            pipe(n+'Injector'+str(k),[(x+dx,y+front*.58,3.16),(x+dx,y+front*.58,3.53),(x+dx,y+front*.3,3.66)],.018,'brass',.06)
        pipe(n+'FuelHeader',[(x-1.75,y+front*.58,3.16),(x+.1,y+front*.58,3.16)],.025,'brass')
        b(n+'CouplingGuard',(x+.45,y,z),(.55,1.05,.89),'yellow',.10)
        c(n+'Alternator',(x+1.25,y,z),.57,1.10,'blue','X',40)
        for xx in (x+.73,x+1.76): flange(n+'AlternatorFlange'+str(xx),(xx,y,z),.59,'X')
        for k in range(14):
            a=k*math.tau/14
            rod(n+'CoolingRib'+str(k),(x+.85,y+.58*math.cos(a),z+.58*math.sin(a)),
                (x+1.65,y+.58*math.cos(a),z+.58*math.sin(a)),.018,'blue')
        c(n+'EndCover',(x+1.83,y,z),.50,.10,'dark','X',40)
        for k in range(9):
            zz=z-.38+k*.095;span=math.sqrt(max(.01,.46**2-(zz-z)**2))
            b(n+'EndLouver'+str(k),(x+1.89,y,zz),(.035,span*2,.025),'steel',.004)
        b(n+'TerminalBox',(x+1.22,y,3.72),(.62,.63,.32),'blue')
        for dx in (-1.45,-.05):
            c(n+'Filter'+str(dx),(x+dx,y+front*.66,2.83),.11,.31,'white')
            rod(n+'FilterNeck'+str(dx),(x+dx,y+front*.66,3.02),(x+dx,y+front*.5,3.02),.035)
        # Exhaust header and lagged riser sit outboard, never over the service aisle.
        for k,dx in enumerate((-1.51,-1.0,-.49,.02)):
            pipe(n+'ExhaustBranch'+str(k),[(x+dx,y,3.60),(x+dx,y+side*.65,3.83)],.058,'dark')
        pipe(n+'ExhaustHeader',[(x-1.6,y+side*.65,3.83),(x+.1,y+side*.65,3.83),
             (x+.1,y+side*1.05,3.83),(x+.1,y+side*1.05,6.65),(-54,y+side*1.05,6.65),(-57.7,0,8.05)],.105,'steel',.24)
        for zz in (4.48,6.55): flange(n+'ExhaustCollar'+str(zz),(x+.1,y+side*1.05,zz),.16)
        b(n+'LocalPanel',(x-.8,y+front*.60,3.48),(.61,.10,.27),'dark')
        text(n+'Name','DIESEL GENERATOR '+str(i+1),(x-.8,y+front*.657,3.53),.043,front)
        for k in range(3): c(n+'PanelLamp'+str(k),(x-1+k*.18,y+front*.663,3.43),.024,.02,'pipe_green' if k!=2 else 'red','Y')
        for k in range(3):
            yy=y+side*(.24+k*.09)
            pipe(n+'PowerCable'+str(k),[(x+1.4,yy,3.8),(x+1.4,y+side*1.05,3.8),
                 (x+1.4,y+side*1.05,4.2),(-36.4,y+side*1.05,4.2),
                 (-36.4,side*6.5,4.2),(-36.2,side*6.5,4.2)],.025,'dark')
        for dy in (.93,1.16): b(n+'TrayRail'+str(dy),(-40.5,y+side*dy,4.16),(8.5,.035,.12),'steel',.004)
        for xx in (-44.5,-43,-41.5,-40,-38.5,-37):
            b(n+'TrayRung'+str(xx),(xx,y+side*1.045,4.12),(.035,.25,.035),'steel',.004)
            rod(n+'TrayHanger'+str(xx),(xx,y+side*1.16,4.16),(xx,y+side*1.16,4.44),.012)

    # Pump casings formerly had a Y axis while the motor/coupling used X.
    # Replace complete sets, not extra ornament on top of the wrong topology.
    remove('BilgeFirePump_')
    for i,(x,side) in enumerate(((-50,-1),(-50,1),(-40,-1),(-40,1))):
        y=side*5.65;z=2.88;n='P'+str(i+1)+'_';front=-side
        b(n+'Base',(x,y,2.22),(2.65,1.05,.18),'steel')
        for dx in (-1.05,1.05):
            for dy in (-.37,.37): foot(n+'Foot'+str((dx,dy)),x+dx,y+dy,2.08)
        c(n+'Motor',(x+.53,y,z),.30,.95,'blue','X',32)
        c(n+'FanCover',(x+1.07,y,z),.315,.16,'dark','X',32)
        for k in range(14):
            a=k*math.tau/14
            rod(n+'Fin'+str(k),(x+.13,y+.305*math.cos(a),z+.305*math.sin(a)),
                (x+.92,y+.305*math.cos(a),z+.305*math.sin(a)),.014,'blue')
        b(n+'MotorFeet',(x+.5,y,2.455),(.72,.71,.33),'blue')
        for dx in (.23,.77):
            for dy in (-.28,.28): c(n+'MotorAnchor'+str((dx,dy)),(x+dx,y+dy,2.39),.022,.18,'steel','Z',6)
        b(n+'Terminal',(x+.50,y,z+.36),(.28,.31,.16),'blue')
        c(n+'Shaft',(x-.16,y,z),.07,.55,'steel','X')
        c(n+'CouplingGuard',(x-.12,y,z),.17,.27,'yellow','X')
        c(n+'BearingHousing',(x-.43,y,z),.13,.32,'blue','X')
        c(n+'Volute',(x-.76,y,z),.39,.35,'blue','X',40)
        flange(n+'CasingJoint',(x-.94,y,z),.365,'X')
        b(n+'CasingFoot',(x-.76,y,2.43),(.42,.46,.26),'blue')
        # Axial suction, radial discharge, flanged isolation valve with stem.
        pipe(n+'Suction',[(x-.94,y,z),(x-1.36,y,z),(x-1.36,side*7.55,z)],.092,'pipe_green',.18)
        flange(n+'SuctionFlange',(x-1.10,y,z),.15,'X')
        pipe(n+'Discharge',[(x-.76,y,z+.26),(x-.76,y,3.77),(x-.76,side*7.9,3.77),
             (x-.76,side*7.9,6.9),(x-.76,side*8.0,6.9)],.075,'pipe_green',.20)
        flange(n+'DischargeFlange',(x-.76,y,3.42),.13)
        b(n+'ValveBody',(x-.76,y,3.60),(.21,.23,.22),'blue')
        rod(n+'ValveStem',(x-.76,y,3.6),(x-.76,y+front*.28,3.6),.024,'brass')
        wheel(n+'Isolation',(x-.76,y+front*.31,3.6),.115,'Y')
        rod(n+'GaugeNeck',(x-.76,y,3.77),(x-.76,y+front*.19,3.77),.017)
        gauge(n+'Pressure',(x-.76,y+front*.24,3.81),front)
        b(n+'TagPlate',(x+.55,y+front*.33,2.96),(.38,.022,.17),'dark')
        text(n+'Tag','SEA WATER\nP-'+str(i+1),(x+.55,y+front*.35,2.96),.052,front)
        # Suction branches terminate in one outboard manifold, not blind stubs.
        for j,(xx,yy,zz) in enumerate(((x-1.36,side*7.55,2.88),(x-.76,side*7.9,3.77))):
            b(n+'PipeShoe'+str(j),(xx,yy,zz-.12),(.26,.28,.10),'steel')
            b(n+'PipeStandBase'+str(j),(xx,yy,2.11),(.30,.30,.06),'steel')
            rod(n+'PipeStand'+str(j),(xx,yy,2.12),(xx,yy,zz-.16),.035)
    for side in (-1,1):
        pipe('CoolingSuctionHeader'+str(side),[(-51.36,side*7.55,2.88),(-23.5,side*7.55,2.88)],.105,'pipe_green')
        # Visible capped maintenance end, distinct from a disconnected pipe.
        for xx in (-51.36,-23.5): flange('SuctionBlind'+str((side,xx)),(xx,side*7.55,2.88),.16,'X')

    # Plate coolers: a clamping frame and tie bolts, not a solid box hiding plates.
    remove('JacketWaterHeatExchanger_')
    for i,side in enumerate((-1,1)):
        x=-43.;y=side*5.65;n='HX'+str(i+1)+'_';front=-side
        b(n+'Base',(x,y,4.65),(1.78,1.35,.14),'steel')
        for xx in (-.72,.72):
            for yy in (-.48,.48): foot(n+'Foot'+str((xx,yy)),x+xx,y+yy,4.58)
        for xx in (-.66,.66):
            b(n+'EndPlate'+str(xx),(x+xx,y,5.48),(.13,1.05,1.52),'blue')
            for yy in (-.38,.38): b(n+'Leg'+str((xx,yy)),(x+xx,y+yy,4.83),(.16,.16,.30),'blue')
        for k in range(24):
            xx=x-.57+k*.049
            b(n+'Plate'+str(k),(xx,y,5.48),(.027,.90,1.38),'steel',.007)
            b(n+'Gasket'+str(k),(xx+.014,y,5.48),(.009,.86,1.34),'dark',.002)
        for yy in (-.49,.49):
            for zz in (5.03,5.92):
                rod(n+'TieRod'+str((yy,zz)),(x-.87,y+yy,zz),(x+.85,y+yy,zz),.032)
                for xx in (-.78,.78): c(n+'Nut'+str((yy,zz,xx)),(x+xx,y+yy,zz),.058,.075,'steel','X',6)
        rod(n+'TopCarryBar',(x-.83,y,6.33),(x+.83,y,6.33),.035)
        for xx in (-.75,.75): rod(n+'CarryUpright'+str(xx),(x+xx,y,6.18),(x+xx,y,6.33),.035)
        for j,(dy,zz) in enumerate(((-.29,5.1),(.29,5.1),(-.29,5.88),(.29,5.88))):
            flange(n+'Port'+str(j),(x+.75,y+dy,zz),.13,'X')
            # Headers are individually separated; no overlapping four-way stubs.
            px=x+1.0+j*.18;out=side*(7.55+j*.17)
            pipe(n+'Circuit'+str(j),[(x+.75,y+dy,zz),(px,y+dy,zz),(px,out,zz),
                 (px,out,6.9),(px,side*(8.0 if j%2==0 else 8.28),6.9)],.055,'pipe_green' if j%2==0 else 'pipe_red',.14)
            rod(n+'Hanger'+str(j),(px,side*7.1,zz+.055),(px,side*7.1,7.18),.014)
            ring(n+'HangerClamp'+str(j),(px,side*7.1,zz),.064,.008,'steel','Y')
        b(n+'NamePlate',(x,y+front*.545,5.51),(.6,.025,.25),'dark')
        text(n+'Tag','JACKET COOLER\nHX-'+str(i+1),(x,y+front*.562,5.51),.065,front)

    # Propulsion-engine service side: break up the slab with actual access
    # fasteners, stiffeners, relief covers, inspection handles and a fuel header.
    remove('R3_InjectorLine','MainEngine_FuelRail_','R3_CylinderManifoldLink','MainEngine_Exhaust_')
    for i,x in enumerate((-47,-45.5,-44,-42.5,-41)):
        for side in (-1,1):
            n='ME'+str((i,side))+'_'
            for xx in (-.51,.51):
                b(n+'Rib'+str(xx),(x+xx,side*2.06,4.02),(.10,.12,2.35),'blue')
            # Bolted cylinder-jacket inspection doors and lifting handle.
            for xx in (-.32,.32):
                for zz in (-.24,.24): c(n+'JacketBolt'+str((xx,zz)),(x+xx,side*1.906,6.22+zz),.022,.025,'dark','Y',6)
            pipe(n+'Handle',[(x-.18,side*1.9,6.06),(x-.18,side*1.98,6.06),
                 (x+.18,side*1.98,6.06),(x+.18,side*1.9,6.06)],.014,'steel',.05)
            c(n+'ReliefCover',(x,side*2.10,3.65),.20,.10,'blue','Y')
            for k in range(5): b(n+'ReliefLouver'+str(k),(x,side*2.16,3.52+k*.063),(.26,.028,.018),'dark',.004)
            pipe(n+'Injection',[(x,side*1.97,5.63),(x-.43,side*1.97,5.63),
                 (x-.43,side*1.97,7.02),(x,side*.83,7.19)],.025,'brass',.15)
            flange(n+'FuelUnion',(x,side*1.97,5.63),.062,'X')
            for zz in (5.95,6.73): b(n+'LineClamp'+str(zz),(x-.43,side*1.96,zz),(.11,.09,.04),'steel')
        # Exhaust riser terminates AT the manifold rather than continuing into space.
        pipe('MEExhaustBranch'+str(i),[(x,.65,7.25),(x,.65,8.05),(x,1.18,8.05)],.16,'steel',.23)
        flange('MEExhaustHead'+str(i),(x,.65,7.43),.23)
    for side in (-1,1):
        pipe('MEFuelHeader'+str(side),[(-48,side*1.97,5.63),(-40.4,side*1.97,5.63),
             (-40.4,side*1.97,4.82),(-39.48,side*1.45,4.82)],.047,'brass',.18)
        b('MEFuelTerminal'+str(side),(-39.42,side*1.45,4.82),(.16,.24,.28),'blue')
        for x in (-48,-40):
            gauge('MEGauge'+str((side,x)),(x,side*2.18,4.76),side)
            rod('MEGaugeStem'+str((side,x)),(x,side*1.99,4.76),(x,side*2.18,4.76),.02)
        b('MENamePlate'+str(side),(-43.7,side*2.08,5.24),(2.1,.024,.29),'dark')
        text('MEName'+str(side),'MAIN PROPULSION / ME-01',(-43.7,side*2.098,5.24),.09,side)

    # Focused additional regression paths beside both new generator banks.
    routes=json.loads(bpy.context.scene['route_contracts'])
    for side in (-1,1):
        routes.append(dict(name='R4_generator_service_'+str(side),points=[(-48.4,side*4.5,2.08),(-44,side*4.5,2.08)]))
        routes.append(dict(name='R4_cooler_service_'+str(side),points=[(-45,side*4.7,4.58),(-40,side*4.7,4.58)]))
        for x in (-50,-40):
            routes.append(dict(name='R4_pump_service_'+str((side,x)),points=[(x-1.1,side*4.5,2.08),(x+1.1,side*4.5,2.08)]))
    # Ceiling-mounted sealed fixtures: actual tank-top lighting, below the
    # intermediate deck that previously shadowed these mechanisms completely.
    M['r4_lens']=g['material']('MAT_R4_MachineryLight',(0.8,.86,.9,1),roughness=.4,emission=(.8,.88,1,1))
    for side in (-1,1):
        for x in (-49,-46,-40):
            n='LowerLight'+str((side,x));y=side*4.8
            b(n+'Mount',(x,y,4.38),(.9,.28,.06),'steel')
            b(n+'Lens',(x,y,4.31),(.82,.19,.07),'r4_lens')
            for dx in (-.3,0,.3): b(n+'Guard'+str(dx),(x+dx,y,4.265),(.014,.23,.014),'steel',.003)
            lamp=g['area_light']('R4_'+n,(x,y,4.23),90,1.2,(.8,.9,1))
            lamp.data.use_shadow=True
    g['COL']['00_REFERENCE'].hide_render=True
    bpy.context.scene['route_contracts']=json.dumps(routes)
    bpy.context.scene['refit_revision']='R4-20260905-Machinery'
    bpy.context.scene['machinery_scope']='2 diesel alternators, 4 sea-water pumps, 2 plate coolers, main-engine service detail. Static authored geometry; no certified circuit simulation.'
    g['machinery_pipe']=pipe
    print('CALMY_R4_MACHINERY',flush=True)
