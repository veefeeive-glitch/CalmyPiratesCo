import copy,unittest
from verify_ocean_cargo_release import validate_carry,validate_route_changes,CONTROLS,POSE
class CargoGateTests(unittest.TestCase):
    def setUp(self):
        self.routes={f'r6_fixture_{i}' for i in range(10)}|{'AUDIT_mess_to_bridge','AUDIT_weather_to_hold_1','AUDIT_weather_to_hold_2'}
        names={n+d+k for n in self.routes for d in ('_forward','_reverse') for k in ('_standard','_bulky')}
        walks=[dict(name=n,pass_=True) for n in sorted(names|CONTROLS)]
        for w in walks:
            w['pass']=w.pop('pass_');w['actual_cargo_grabbed']=True
            w['cargo_kind']='Bulky' if w['name'].endswith('_bulky') else 'Standard'
            if w['cargo_kind']=='Bulky':w['helper_position']=[0,0,0]
        self.r=dict(source_sha256='fixture',carry_audit=True,cargo_overlap_pose=POSE,walks=walks,total=56,passed=56)
    def check(self):return validate_carry(self.r,self.routes,'fixture')
    def test_valid(self):self.assertEqual(self.check()['cargo_walks'],52)
    def test_stale(self):
        self.r['source_sha256']='old'
        with self.assertRaises(ValueError):self.check()
    def test_predicted_pose(self):
        self.r.pop('cargo_overlap_pose')
        with self.assertRaises(ValueError):self.check()
    def test_missing_renamed(self):
        self.r['walks'][-1]['name']='wrong'
        with self.assertRaises(ValueError):self.check()
    def test_duplicate(self):
        self.r['walks'][-1]=copy.deepcopy(self.r['walks'][-2])
        with self.assertRaises(ValueError):self.check()
    def test_no_actual_grab(self):
        next(w for w in self.r['walks'] if not w['name'].startswith('CONTROL_'))['actual_cargo_grabbed']=False
        with self.assertRaises(ValueError):self.check()
    def test_missing_helper(self):
        next(w for w in self.r['walks'] if w['cargo_kind']=='Bulky').pop('helper_position')
        with self.assertRaises(ValueError):self.check()
    def test_failed_control(self):
        next(w for w in self.r['walks'] if w['name'].startswith('CONTROL_'))['pass']=False;self.r['passed']-=1
        with self.assertRaises(ValueError):self.check()
    def test_failed_cargo(self):
        next(w for w in self.r['walks'] if not w['name'].startswith('CONTROL_'))['pass']=False;self.r['passed']-=1
        with self.assertRaises(ValueError):self.check()
class RouteChangeTests(unittest.TestCase):
    def setUp(self):
        self.old={n:[[0,0,0],[0,0,0],[-4975,0,0],[-4975,0,0],[-4975,0,0],[-4975,0,0]] for n in ('AUDIT_room_Boat LAUNDRY','AUDIT_room_Crew MEDICAL','AUDIT_room_Officers RADIO','AUDIT_usable_Crew MEDICAL')}
        self.new=copy.deepcopy(self.old)
        for n in self.new:
            for i in ((2,3) if n.startswith('AUDIT_usable') else (4,5)):self.new[n][i][0]-=40
    def test_declared_only(self):self.assertEqual(len(validate_route_changes(self.old,self.new)),4)
    def test_other_point_rejected(self):
        self.new['AUDIT_room_Boat LAUNDRY'][0][1]+=1
        with self.assertRaises(ValueError):validate_route_changes(self.old,self.new)
    def test_extra_shift_rejected(self):
        self.new['AUDIT_usable_Crew MEDICAL'][2][0]-=1
        with self.assertRaises(ValueError):validate_route_changes(self.old,self.new)
if __name__=='__main__':unittest.main()
