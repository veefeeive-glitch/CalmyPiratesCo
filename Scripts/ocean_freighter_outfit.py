"""R5 service-room props: authored for this layout, kept outside stair aisles."""
import bpy,math,json

def apply_outfit(g):
    M=g['MAT'];C=g['COL']['40_MACHINERY_SYSTEMS']
    def b(n,p,d,m='steel',e=.015):return g['box']('R5_'+n,p,d,M[m],C,e)
    def c(n,p,r,h,m='steel',rot=(0,0,0),v=32):return g['cylinder']('R5_'+n,p,r,h,M[m],C,rot,v)
    def rod(n,a,z,r=.018,m='steel'):return g['cylinder_between']('R5_'+n,a,z,r,M[m],C,16)
    def ring(n,p,r,t=.016,m='dark',rot=(0,0,0)):return g['torus']('R5_'+n,p,r,t,M[m],C,rot)
    def label(n,s,p,size=.08):
        o=g['add_text']('R5_'+n,s,p,size,C);o.data.materials.clear();o.data.materials.append(M['white'])
        return o
    x=-29.5;y=8.05;f=2.08
    # Bench: separate legs, drawer carcass, wood top, steel vise and tool backboard.
    b('BenchTop',(x,y,f+.92),(2.1,.70,.09),'wood')
    for dx in (-.89,.89):
        for dy in (-.25,.25):
            b('BenchLeg'+str((dx,dy)),(x+dx,y+dy,f+.43),(.065,.065,.86))
            b('BenchFoot'+str((dx,dy)),(x+dx,y+dy,f+.035),(.16,.16,.07))
    b('BenchLowerShelf',(x,y,f+.2),(1.86,.57,.045),'blue')
    b('DrawerCarcass',(x+.59,y,f+.65),(.65,.61,.43),'blue')
    for k in range(3):
        z=f+.51+k*.13;b('DrawerFront'+str(k),(x+.59,y-.318,z),(.60,.035,.105),'blue')
        rod('DrawerHandle'+str(k),(x+.40,y-.356,z),(x+.78,y-.356,z),.012)
    for dx in (-.9,.9):b('BoardPost'+str(dx),(x+dx,y+.27,f+1.15),(.045,.045,1.1))
    b('ToolBoard',(x,y+.28,f+1.48),(1.85,.05,.7),'dark')
    for i in range(6):
        xx=x-.73+i*.20;zz=f+1.47
        rod('ToolShaft'+str(i),(xx,y+.228,zz-.19),(xx,y+.228,zz+.12),.018)
        ring('ToolRing'+str(i),(xx,y+.228,zz+.15),.044,.012,'steel',(math.pi/2,0,0))
        rod('ToolHook'+str(i),(xx,y+.25,zz+.2),(xx,y+.18,zz+.2),.01)
    label('BenchSign','ENGINE WORKSHOP',(x,y+.235,f+1.72),.10)
    b('ViseBase',(x-.6,y-.1,f+1.0),(.35,.31,.09),'blue')
    b('ViseBody',(x-.6,y-.1,f+1.12),(.30,.23,.2),'blue')
    for dy in (-.23,.06):b('ViseJaw'+str(dy),(x-.6,y+dy,f+1.25),(.31,.07,.12))
    rod('ViseScrew',(x-.6,y-.12,f+1.13),(x-.6,y-.50,f+1.13),.022)
    rod('ViseHandle',(x-.6,y-.50,f+.98),(x-.6,y-.50,f+1.28),.012)
    # Fire post: freestanding structural backing, extinguisher cradle, actual
    # hose reel flanges and wound hose, nozzle clipped to the board, not a ring on a box.
    x=-29.2;y=-8.02
    b('FireBacking',(x,y,f+1.02),(1.75,.12,1.95),'red')
    for dx in (-.62,.62):b('FireFoot'+str(dx),(x+dx,y,f+.045),(.22,.42,.09))
    label('FireID','FIRE POINT',(x,y+.078,f+1.80),.12).rotation_euler.z=math.pi
    c('ExtinguisherBottle',(x-.47,y+.23,f+.57),.105,.52,'red')
    c('ExtinguisherShoulder',(x-.47,y+.23,f+.86),.076,.08,'red')
    c('ExtinguisherValve',(x-.47,y+.23,f+.94),.033,.10,'brass')
    b('ExtinguisherLever',(x-.42,y+.23,f+.99),(.16,.035,.025),'dark')
    b('ExtinguisherBand',(x-.47,y+.336,f+.64),(.14,.015,.13),'white')
    b('BottleCradle',(x-.47,y+.22,f+.295),(.24,.29,.05),'steel')
    for zz in (.44,.74):
        ring('BottleClamp'+str(zz),(x-.47,y+.23,f+zz),.108,.012,'steel')
        rod('BottleBracket'+str(zz),(x-.47,y+.08,f+zz),(x-.47,y+.14,f+zz),.023)
    hx=x+.30;hz=f+1.11
    c('HoseSpindle',(hx,y+.25,hz),.045,.38,'steel',(math.pi/2,0,0))
    for yy in (.15,.44):
        c('HoseFlange'+str(yy),(hx,y+yy,hz),.32,.032,'red',(math.pi/2,0,0))
    for k in range(10):ring('HoseCoil'+str(k),(hx,y+.18+k*.025,hz),.27,.022,'dark',(math.pi/2,0,0))
    g['curve_tube']('R5_HoseTail',[(hx+.25,y+.36,hz),(hx+.53,y+.36,hz-.28),(hx+.53,y+.36,f+.56)],.022,M['dark'],C)
    rod('HoseNozzle',(hx+.53,y+.36,f+.56),(hx+.53,y+.36,f+.82),.035,'brass')
    b('NozzleClip',(hx+.53,y+.20,f+.72),(.11,.32,.05),'steel')
    # Portable cable drum, hand frame, two grounded wheels and support toe.
    x=-32.5;y=8.0
    for dx in (-.31,.31):
        c('ReelWheel'+str(dx),(x+dx,y+.16,f+.13),.13,.07,'dark',(0,math.pi/2,0))
        c('ReelHub'+str(dx),(x+dx,y+.16,f+.13),.049,.08,'steel',(0,math.pi/2,0))
    rod('ReelAxle',(x-.35,y+.16,f+.13),(x+.35,y+.16,f+.13),.027)
    for dx in (-.23,.23):
        rod('ReelStand'+str(dx),(x+dx,y-.23,f+.035),(x+dx,y+.22,f+.79),.024,'orange')
        rod('ReelHandlePost'+str(dx),(x+dx,y+.22,f+.79),(x+dx,y+.24,f+1.02),.024,'orange')
    rod('ReelHandle',(x-.23,y+.24,f+1.02),(x+.23,y+.24,f+1.02),.032,'dark')
    rod('ReelToe',(x-.25,y-.23,f+.025),(x+.25,y-.23,f+.025),.025,'orange')
    for dx in (-.18,.18):c('CableDrumFlange'+str(dx),(x+dx,y,f+.51),.28,.04,'orange',(0,math.pi/2,0))
    for k in range(10):ring('CableCoil'+str(k),(x-.145+k*.032,y,f+.51),.245,.018,'dark',(0,math.pi/2,0))
    rod('DrumAxle',(x-.30,y,f+.51),(x+.30,y,f+.51),.035)
    for dx in (-.24,.24):
        b('DrumBearingMount'+str(dx),(x+dx,y+.025,f+.51),(.07,.17,.15),'steel')
        c('DrumBearing'+str(dx),(x+dx,y,f+.51),.075,.08,'steel',(0,math.pi/2,0))
    c('SocketPanel',(x-.211,y,f+.51),.16,.02,'dark',(0,math.pi/2,0))
    for dz in (-.065,.065):c('SocketCover'+str(dz),(x-.23,y,f+.51+dz),.046,.022,'blue',(0,math.pi/2,0))
    # Broaden the narrow aisle between the former console face and stair guard.
    for obj in bpy.data.objects:
        if obj.name.startswith('R3_EngineControl'):obj.location.y-=.4
    for obj in list(bpy.data.objects):
        if obj.name.startswith('R4_CoolingSuctionHeader'):bpy.data.objects.remove(obj,do_unlink=True)
    for side in (-1,1):
        yy=side*7.55
        g['machinery_pipe']('R5_SuctionAccessBypass'+str(side),[(-51.36,yy,2.88),(-35.35,yy,2.88),
            (-35.35,yy,4.12),(-24.05,yy,4.12),(-24.05,yy,2.88),(-23.5,yy,2.88)],.105,'pipe_green',.24)
        for xx in (-34,-30,-26):
            rod('PipeHanger'+str((side,xx)),(xx,yy,4.23),(xx,yy,4.44),.014)
            ring('PipeHangerBand'+str((side,xx)),(xx,yy,4.12),.114,.01,'steel',(0,math.pi/2,0))
        b('ServiceLightBase'+str(side),(-30,side*7.12,4.38),(.80,.22,.06))
        b('ServiceLightLens'+str(side),(-30,side*7.12,4.32),(.74,.16,.06),'r4_lens')
        lamp=g['area_light']('R5_ServiceLight'+str(side),(-30,side*7.12,4.23),70,1.2,(.8,.9,1))
        lamp.data.use_shadow=True
    routes=json.loads(bpy.context.scene['route_contracts'])
    for side in (-1,1):
        routes.append(dict(name='R5_outfit_service_'+str(side),points=[(-33,side*3.1,2.08),(-33,side*7.12,2.08),(-26.6,side*7.12,2.08)]))
    bpy.context.scene['route_contracts']=json.dumps(routes)
    bpy.context.scene['refit_revision']='R5-20260905-PhysicsOutfit'
    bpy.context.scene['outfit_revision']='R5-20260905-ServiceProps'
    print('CALMY_R5_OUTFIT',flush=True)
