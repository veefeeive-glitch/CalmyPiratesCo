"""R6: supported circulation connections and room-specific original fittings."""
import bpy, json, math
from mathutils import Vector

def apply_circulation(g):
    M=g['MAT']; C=g['COL']; decks=C['20_DECKS_BULKHEADS']; room=C['30_SUPERSTRUCTURE']
    def b(n,p,d,m='steel',col=decks,e=.012):
        return g['box']('R6_'+n,p,d,M[m],col,e)
    def line(n,a,z,r=.025,m='steel',col=decks):
        return g['cylinder_between']('R6_'+n,a,z,r,M[m],col,16)
    def remove(prefix):
        for o in list(bpy.data.objects):
            if o.name.startswith(prefix):bpy.data.objects.remove(o,do_unlink=True)
    def guard(n,a,z):
        a,z=Vector(a),Vector(z)
        for h in (.52,1.05):line(n+'Rail'+str(h),a+Vector((0,0,h)),z+Vector((0,0,h)))
        count=max(1,math.ceil((z-a).length/1.4))
        for i in range(count+1):
            p=a.lerp(z,i/count);line(n+'Post'+str(i),p,p+Vector((0,0,1.05)),.029)
        line(n+'Toe',a+Vector((0,0,.08)),z+Vector((0,0,.08)),.045,'dark')
    def text(n,body,p,size=.06):
        o=g['add_text']('R6_'+n,body,p,size,room)
        o.data.materials.clear();o.data.materials.append(M['white']);return o
    # Only the grounded base level receives longitudinal openings. Upper exposed
    # tower ends and the outboard return gallery keep their original guards.
    for s in (-1,1):
        for x in (-39.76,-31.84):
            remove('R3_LandingSide'+str((s,0,x)))
            guard('LandingEnd'+str((s,x)),(x,s*9.25,10.67),(x,s*12.15,10.67))
        # An opening through the old hull-side rail is safe where the tower
        # platform bridges it. Cut only that 1.42 m supported connector.
        for o in list(bpy.data.objects):
            if o.type=='MESH' and o.name.startswith(('RailRun_'+str(s)+'_','RailPost_'+str(s)+'_')):
                pts=[o.matrix_world@Vector(v) for v in o.bound_box]
                if max(p.x for p in pts)<-33.41 or min(p.x for p in pts)>-31.99:continue
                g['cut_mesh_opening'](o.name,'R6_TowerGate'+str(s),(-32.7,s*9.15,11.25),(1.42,.9,1.6))
        for x in (-33.41,-31.99):
            line('GateStanchion'+str((s,x)),(x,s*9.12,10.67),(x,s*9.12,11.72),.032)
        # A real supported side gallery replaces the narrowing 0.4 m stern
        # pinch. Bracketed deck extension, not removal of the exposed-edge rail.
        for segment in range(5):
            for z in (11.25,11.65):
                o=bpy.data.objects.get(f'RailRun_{s}_{z}_{segment}')
                if o:bpy.data.objects.remove(o,do_unlink=True)
        for x in range(-56,-41,3):remove(f'RailPost_{s}_{x}')
        remove('R3_AftCornerGuard'+str(s))
        b('SternSideGallery'+str(s),(-48.25,s*8.5,10.50),(16.5,1.60,.16),'diamond')
        guard('SternOuterGuard'+str(s),(-56.5,s*9.3,10.58),(-41,s*9.3,10.58))
        guard('SternGuardJoin'+str(s),(-41,s*9.3,10.58),(-41,s*8.94,10.58))
        guard('SternAftEdge'+str(s),(-56.5,s*9.3,10.58),(-56.5,s*7.30,10.58))
        for x in (-55.8,-53,-50,-47,-44,-41):
            line('GalleryBracket'+str((s,x)),(x,s*7.7,9.45),(x,s*9.10,10.36),.065)
            b('GalleryBeam'+str((s,x)),(x,s*8.5,10.32),(.12,1.60,.20))
        # Move the aft mooring pair away from the transverse escape aisle.
        for o in bpy.data.objects:
            if o.name.startswith('Bollard_0_'+str(s)+'_'):
                o.location.x-=2.8;o.location.y+=.55 if s==1 else 1.6
    for o in bpy.data.objects:
        if o.name.startswith(('Fairlead_0_','Fairlead_1_')):o.location.x-=1.0
        if o.name.startswith('Aft_Windlass_ChainStopper_'):o.location.x-=1.2
    aft_base=bpy.data.objects['Aft_Windlass_Foundation']
    aft_base.location.x=-58.65;aft_base.scale.x*=3.7/5.4
    b('SternTransverseGallery',(-55.8,0,10.50),(1.4,18.6,.16),'diamond')
    # The old 0.5 m sliver behind the engine atrium cannot fit a capsule. Build
    # a 1.30 m supported bridge, moving only the enclosed-side void guard.
    for f in (4.58,7.33):
        remove('R3_EngineGalleryEnd'+str((f,-51.5)))
        for s in (-1,1):
            remove('R3_EngineGallery'+str((f,s)))
            guard('GalleryLong'+str((f,s)),(-50.6,s*4.25,f),(-37.8,s*4.25,f))
        b('AftCrosswalk'+str(f),(-51.30,0,f-.08),(1.40,8.70,.16),'diamond')
        guard('AftCrosswalkEdge'+str(f),(-50.6,-4.25,f),(-50.6,4.25,f))
        for y in (-3.9,-1.3,1.3,3.9):
            b('CrosswalkBeam'+str((f,y)),(-51.3,y,f-.22),(1.4,.10,.16))
    remove('R3_ExhaustTrunk')
    g['curve_tube']('R6_ExhaustOverhead',[(-40,1.18,8.05),(-40,1.18,9.95),(-54,1.18,9.95),(-57.7,0,9.95),(-57.7,0,21.2)],.29,M['dark'],C['40_MACHINERY_SYSTEMS'])
    # Preserve the machine centrally, shorten its axial package to leave a
    # full-width service apron before bulkhead X52. Keep side bypasses clear.
    remove('BowThrusterMotor')
    foundation=bpy.data.objects['BowThruster_Foundation'];foundation.location.x=49.2;foundation.scale.x*=.74
    machinery=C['40_MACHINERY_SYSTEMS']
    g['cylinder']('R6_ThrusterMotorBody',(49.35,0,3.2),.64,2.3,M['blue'],machinery,(0,math.pi/2,0),64)
    for x in (48.2,50.5):
        g['cylinder']('R6_ThrusterEndCover'+str(x),(x,0,3.2),.69,.14,M['blue'],machinery,(0,math.pi/2,0),64)
        for k in range(8):
            a=k*math.tau/8
            g['cylinder']('R6_ThrusterCoverBolt'+str((x,k)),(x+(.085 if x>49 else -.085),.55*math.sin(a),3.2+.55*math.cos(a)),.034,.04,M['steel'],machinery,(0,math.pi/2,0),6)
    b('ThrusterCradle',(49.35,0,2.56),(2.0,.95,.24),'steel',machinery)
    for y in (-.76,.76):
        b('ThrusterMount'+str(y),(49.35,y,2.38),(2.35,.22,.22),'steel',machinery)
    for i in range(16):
        a=i*math.tau/16
        o=b('ThrusterCoolingFin'+str(i),(49.35,.665*math.sin(a),3.2+.665*math.cos(a)),(2.04,.032,.115),'blue',machinery,.004)
        o.rotation_euler.x=-a
    b('ThrusterJunctionBox',(49.5,0,3.96),(.48,.48,.25),'blue',machinery)
    # Replace the copied cabin furnishings in MEDICAL (room 8) and the radio's
    # generic desk. New assemblies occupy the outboard furnishing band only.
    for prefix in ('R3_Room8_','R3_R3_Room8_','R3_Room14_Radio'):
        remove(prefix)
    f=13.42;x=-52.0;y=6.55
    b('MedicalCotFrame',(x,y,f+.57),(2.15,.93,.10),'steel',room)
    b('MedicalMattress',(x,y,f+.71),(2.03,.86,.20),'white',room,.08)
    b('MedicalPillow',(x-.73,y,f+.85),(.45,.73,.14),'white',room,.06)
    for dx in (-.84,.84):
        for dy in (-.33,.33):
            line('CotLeg'+str((dx,dy)),(x+dx,y+dy,f+.09),(x+dx,y+dy,f+.56),.035,col=room)
            g['cylinder']('R6_CotCaster'+str((dx,dy)),(x+dx,y+dy,f+.09),.09,.05,M['dark'],room,(math.pi/2,0,0),24)
    for dy in (-.49,.49):
        for dx in (-.65,.65):line('CotRailUpright'+str((dy,dx)),(x+dx,y+dy,f+.58),(x+dx,y+dy,f+1.01),.018,col=room)
        line('CotSafetyRail'+str(dy),(x-.65,y+dy,f+1.01),(x+.65,y+dy,f+1.01),.021,col=room)
    x=-54.05;y=6.58
    for h in (.20,.83):b('TreatmentCartShelf'+str(h),(x,y,f+h),(.60,.65,.045),'white',room)
    for dx in (-.25,.25):
        for dy in (-.28,.28):line('CartLeg'+str((dx,dy)),(x+dx,y+dy,f+.06),(x+dx,y+dy,f+.9),.018,col=room)
    b('FirstAidCase',(x,y,f+.99),(.44,.34,.27),'white',room)
    b('MedicalCrossV',(x,y-.177,f+.99),(.055,.015,.18),'red',room)
    b('MedicalCrossH',(x,y-.177,f+.99),(.18,.015,.055),'red',room)
    b('MedicineCabinet',(-50.2,7.43,f+1.45),(.80,.26,.68),'white',room)
    for dx in (-.19,.19):
        b('MedicineDoor'+str(dx),(-50.2+dx,7.284,f+1.45),(.36,.025,.60),'blue',room)
        line('MedicineHandle'+str(dx),(-50.2+dx,7.25,f+1.38),(-50.2+dx,7.25,f+1.52),.012,col=room)
    f=16.17;x=-50.75;y=6.48
    b('RadioWorktop',(x,y,f+.78),(2.15,.9,.065),'wood',room)
    for dx in (-.87,.87):b('RadioPedestal'+str(dx),(x+dx,y+.10,f+.37),(.25,.63,.74),'blue',room)
    for i in range(3):
        xx=x-.69+i*.69
        b('RadioReceiver'+str(i),(xx,y+.05,f+.99),(.63,.48,.34),'dark',room)
        b('RadioDisplay'+str(i),(xx-.10,y-.198,f+1.02),(.29,.018,.13),'screen',room)
        text('RadioFrequency'+str(i),('156.800','2182.0','CH 70')[i],(xx-.10,y-.213,f+1.015),.047)
        for k in range(5):b('RadioVent'+str((i,k)),(xx-.21+k*.095,y-.198,f+1.13),(.048,.015,.012),'steel',room,.002)
        for k in range(4):b('RadioKey'+str((i,k)),(xx-.23+k*.072,y-.212,f+.906),(.045,.02,.032),'white',room,.003)
        for k in range(2):
            g['cylinder']('R6_RadioKnob'+str((i,k)),(xx+.16,y-.225,f+.94+k*.12),.034,.035,M['steel'],room,(math.pi/2,0,0),24)
    b('RadioLogbook',(x-.35,y-.24,f+.837),(.4,.26,.025),'white',room)
    line('RadioMicStem',(x+.80,y-.25,f+.82),(x+.80,y-.25,f+1.20),.018,col=room)
    b('RadioMicrophone',(x+.80,y-.25,f+1.25),(.08,.10,.14),'dark',room)
    # Explicit accepted circulation: retained old exploratory tests still show
    # blocked impossible centre-through-machinery/perimeter intents honestly.
    routes=json.loads(bpy.context.scene['route_contracts'])
    for s in (-1,1):
        routes.append(dict(name='r6_stern_service_'+str(s),points=[(-32.7,s*8.5,10.67),(-40,s*8.5,10.58),(-55.8,s*8.5,10.58),(-55.8,s*3.5,10.58)]))
        routes.append(dict(name='r6_forepeak_service_'+str(s),points=[(43.2,s*3.1,2.08),(43.2,s*1.8,2.08),(51.4,s*1.8,2.08),(51.4,0,2.08),(53,0,2.08)]))
        for f in (4.58,7.33):
            routes.append(dict(name='r6_aft_gallery_'+str((s,f)),points=[(-37,s*4.7,f),(-51.3,s*4.7,f),(-51.3,0,f),(-55,0,f)]))
        lookup={r['name']:r['points'] for r in routes}
        p=[(50.2,s*3.1,10.58)]
        for i in (2,1,0):
            p+=list(reversed(lookup[f'forepeak_access_{s}_{i}']))
            if i:
                f=(4.58,7.33)[i-1];p += [(43.2,s*3.1,f),(43.2,s*5.2,f),(50.2,s*5.2,f),(50.2,s*3.1,f)]
        p+=lookup['r6_forepeak_service_'+str(s)]
        routes.append(dict(name='r6_weather_to_forepeak_'+str(s),points=p))
    bpy.context.scene['route_contracts']=json.dumps(routes)
    bpy.context.scene['refit_revision']='R6-20260905'
    bpy.context.scene['design_status']='R6 circulation and room fittings candidate; human review pending'
    for key in ('qa_report','physical_qa_report'):
        if key in bpy.context.scene:del bpy.context.scene[key]
    # A broad prefix must never remove bow/working-deck safety rails.
    assert all(bpy.data.objects.get(f'RailRun_{s}_{z}_{i}') for s in (-1,1) for z in (11.25,11.65) for i in range(10,37))
