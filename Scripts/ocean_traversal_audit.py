"""Exploratory continuous itineraries, in Blender metres; no scene mutations.

These are proposed player intents, NOT pre-approved clear paths. Failures are
audit findings to inspect, not an instruction to remove whatever was hit.
"""
def build_routes(base, rooms):
    lookup={r['name']:r['points'] for r in base}
    result=[]
    def add(name,points,purpose):
        compact=[]
        for p in points:
            p=list(p)
            if not compact or p!=compact[-1]:compact.append(p)
        result.append(dict(name='AUDIT_'+name,points=compact,purpose=purpose))
    # All accommodation levels connected through actual stair/lobby turns.
    p=[(-49.09,-3.6,10.67),(-49.09,0,10.67),(-48.65,0,10.67)]
    for i in range(3):
        f=10.67+i*2.75;top=f+2.75
        p += [(-48.65,3.77,f)] + lookup['interior_stair_'+str(i)]
        p += [(-48.65,5.57,top),(-48.65,3.77,top)]
    p += [(-48.65,0,18.92),(-36,0,18.92),(-36,-5.6,18.92)]
    add('mess_to_bridge',p,'Meals -> three interior stair turns -> bridge operator')
    for side in (-1,1):
        # Engine stair return lane connects the parallel flights at each floor.
        p=[(-25.3,side*5.8,10.58)]
        for i in (2,1,0):
            p+=list(reversed(lookup[f'engine_access_{side}_{i}']))
            if i:
                f=(4.58,7.33)[i-1]
                p += [(-32.3,side*5.8,f),(-32.3,side*7.05,f),(-25.3,side*7.05,f),(-25.3,side*5.8,f)]
        p+=lookup[f'engine_tanktop_service_{side}']
        add('weather_to_stern_engine_'+str(side),p,'Weather deck -> lower/tween turns -> engine -> stern service')
        # Forward vertical escape and bottom forepeak compartments.
        p=[(50.2,side*3.1,10.58)]
        for i in (2,1,0):
            p+=list(reversed(lookup[f'forepeak_access_{side}_{i}']))
            if i:
                f=(4.58,7.33)[i-1]
                p += [(43.2,side*3.1,f),(43.2,side*5.2,f),(50.2,side*5.2,f),(50.2,side*3.1,f)]
        p += [(43.2,side*3.1,2.08),(43.2,0,2.08),(51,0,2.08),(53,0,2.08)]
        add('weather_to_forepeak_'+str(side),p,'Forward weather deck -> all forepeak levels -> bow store doorway')
        # Connection between superstructure exits and exterior stairs.
        p=[(-32.7,0,10.67),(-32.7,side*9.8,10.67)]
        for i in range(4):
            p+=lookup[f'exterior_return_{side}_{i}']+lookup[f'exterior_{side}_{i}']
        p += [(-32.63,side*9.8,22.08),(-32.63,side*6.8,22.08),(-35,side*5.5,22.08)]
        add('exterior_tower_to_roof_'+str(side),p,'Superstructure exit -> four external stair flights -> roof')
        add('accommodation_to_weather_'+str(side),[(-32.7,0,10.67),(-32.7,side*8.5,10.67),(-25.3,side*8.6,10.58)]+lookup[f'full_length_walk_{side}_10.58'][2:], 'Accommodation exit -> side deck -> forward escape')
        for f in (4.58,7.33):
            add('engine_gallery_'+str((side,f)),[(-25.3,side*5.8,f),(-25.3,0,f),(-36.5,0,f),(-37,side*4.9,f),(-50.8,side*4.9,f),(-51.2,0,f),(-55,0,f)],'Engine stair landing -> central bulkhead -> machinery gallery -> aft doorway')
        add('stern_weather_'+str(side),[(-32.7,side*8.5,10.67),(-40,side*8.7,10.58),(-55.7,side*8.7,10.58),(-57,side*4,10.58),(-57,0,10.58)],'Boat-deck side passage -> stern working deck')
        add('engine_equipment_'+str(side),[(-33,side*3.1,2.08),(-33,side*7.12,2.08),(-26.6,side*7.12,2.08)],'Workshop/fire station approach, not simulated interaction')
    for hold,side in ((1,-1),(2,1)):
        p=list(lookup[f'hold_{hold}_weather_entry'])
        for i in (2,1,0):
            flight=lookup[f'hold_{hold}_{i}'];p+=list(reversed(flight))
            if i:
                # End landing sits outside the tread endpoints; turn there.
                x=flight[0][0]+(-.47 if i==2 else .47)
                nxt=lookup[f'hold_{hold}_{i-1}'][-1]
                p += [(x,flight[0][1],flight[0][2]),(x,nxt[1],nxt[2]),nxt]
        foot=p[-1]
        p += [(foot[0]-.8,foot[1],2.08),(foot[0]-.8,0,2.08)]
        p += lookup[f'hold_{hold}_tanktop']
        add('weather_to_hold_'+str(hold),p,'Coaming entrance -> all stair landing turns -> hold floor')
    # Test room access beyond the already-tested door centreline. Lanes are
    # explicit intended circulation beside furniture, not through desk centres.
    for room in rooms:
        x,_,f=room['door'];s=room['side'];a=room['x0'];b=room['x1'];w=room['width']
        add('room_'+room['name'],[(x,0,f),(x,s*2.1,f),(a+.60,s*2.1,f),(a+.60,s*(w-.65),f),(b-.60,s*(w-.65),f),(b-.60,s*2.1,f),(x,s*2.1,f),(x,0,f)],'Corridor -> room side circulation -> window/bedside -> exit; exploratory')
    add('bridge_watch',[(-48.65,0,18.92),(-40,0,18.92),(-36,0,18.92),(-36,-5.6,18.92),(-32.7,-5.85,18.92),(-36,-5.85,18.92),(-36,5.85,18.92),(-32.7,5.85,18.92)],'Bridge rear entrance -> operator zone -> both wings')
    # Follow-up intent routes avoid deliberately circling behind furniture.
    # Keep original failures in the suite; never replace them to manufacture PASS.
    for room in rooms:
        x,_,f=room['door'];s=room['side'];a=room['x0'];b=room['x1'];w=room['width'];name=room['name']
        p=[(x,0,f),(x,s*2.8,f)]
        if any(t in name for t in ('CABIN','OFFICER','CAPTAIN','MEDICAL')):
            p += [(b-.60,s*2.8,f),(b-.60,s*(w-1.75),f)]
        elif 'GALLEY' in name:
            p += [(-40.9,2.8,f),(-40.9,5.7,f),(-37.6,5.7,f)]
        elif 'LAUNDRY' in name:
            p += [(a+.65,2.8,f),(a+.65,5.7,f),(a+3.4,5.7,f)]
        elif 'WASHROOM' in name:
            p += [(-38.7,-2.8,f),(-38.7,-4.3,f),(-42,-4.3,f)]
        elif 'MESS' in name:
            p += [(a+.65,-2.8,f),(a+.65,-6.4,f),(b-1,-6.4,f)]
        else:p += [(x,s*(w-2.2),f)]
        add('usable_'+name,p,'Door -> usable furniture-side approach; not a loop behind furniture')
    for side in (-1,1):
        for y in (7.9,8.15,8.6):
            add('exit_lane_'+str((side,y)),[(-32.7,0,10.67),(-32.7,side*y,10.67),(-30.8,side*y,10.58),(-25.3,side*8.6,10.58)],'Probe several exit approaches around tower landing guard')
        add('stern_inboard_'+str(side),[(-32.7,side*8.15,10.67),(-40,side*8.15,10.58),(-55.7,side*8.15,10.58),(-56,side*4,10.58)],'Stern side passage using inboard lane; avoid exhaust centre')
        add('forepeak_bypass_'+str(side),[(44,side*3.1,2.08),(43.2,side*3.1,2.08),(43.2,side*1.8,2.08),(51.4,side*1.8,2.08),(51.4,0,2.08),(53,0,2.08)],'Attempt side clearance around bow-thruster body')
        for i,(f,w) in enumerate(((13.42,7.6),(16.17,7.2),(18.92,6.6)),1):
            add('upper_exit_'+str((side,i)),[(-32.7,side*(w-.3),f),(-32.7,side*9.8,f)]+lookup[f'exterior_return_{side}_{i}']+lookup[f'exterior_{side}_{i}'],'Upper level exit -> tower; isolate base-deck rail issue')
    return result
