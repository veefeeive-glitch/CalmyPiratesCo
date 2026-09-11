"""Pure-Python negative controls for review tooling, not Unreal movement tests."""
import copy
import unittest
from pathlib import Path
from recheck_ocean_traversal import compare, classify
from traversal_camera_profile import get_profile


def fixture():
    walks=[dict(name='route'+d,pass_=True) for d in ('_forward','_reverse')]
    walks += [dict(name=f'CONTROL_{i}',pass_=True) for i in range(4)]
    for w in walks:
        w['pass']=w.pop('pass_')
    return dict(source_sha256='hash',walks=walks,total=6,passed=6)


class RecheckTests(unittest.TestCase):
    def test_same_evidence(self):
        r=fixture()
        self.assertEqual(compare(r,r,[dict(name='route')])['regressions'],[])

    def test_regression_is_not_hidden(self):
        old=fixture();new=copy.deepcopy(old)
        new['walks'][0]['pass']=False;new['passed']-=1
        self.assertEqual(compare(old,new,[dict(name='route')])['regressions'],['route_forward'])

    def test_missing_route_rejected(self):
        old=fixture();new=copy.deepcopy(old)
        new['walks'].pop(0);new['passed']-=1;new['total']-=1
        with self.assertRaises(ValueError):compare(old,new,[dict(name='route')])

    def test_duplicate_rejected(self):
        r=fixture();r['walks'].append(r['walks'][0]);r['passed']+=1;r['total']+=1
        with self.assertRaises(ValueError):compare(r,r,[dict(name='route')])

    def test_bad_control_rejected(self):
        r=fixture();r['walks'][-1]['pass']=False;r['passed']-=1
        with self.assertRaises(ValueError):compare(r,r,[dict(name='route')])

    def test_other_model_rejected(self):
        old=fixture();new=copy.deepcopy(old);new['source_sha256']='other'
        with self.assertRaises(ValueError):compare(old,new,[dict(name='route')])

    def test_bad_count_rejected(self):
        r=fixture();r['passed']=0
        with self.assertRaises(ValueError):compare(r,r,[dict(name='route')])

    def test_unknown_contact_not_excused(self):
        self.assertEqual(classify('new_fault_forward'),('unreviewed',None))

    def test_room_has_specific_alternative(self):
        self.assertEqual(classify('AUDIT_room_Crew CABIN 1_reverse')[1],'AUDIT_usable_Crew CABIN 1')

    def test_camera_profiles(self):
        p=get_profile('bodycam')
        self.assertEqual((p['forward_cm'],p['up_cm'],p['fov']),(20,36,102))
        self.assertEqual(get_profile('steady')['fov'],92)
        self.assertEqual(get_profile('legacy-r5')['up_cm'],64)
        with self.assertRaises(ValueError):get_profile('typo')

    def test_camera_matches_current_source(self):
        source=(Path(__file__).resolve().parents[1]/'Source/CalmyPiratesCo/Private/Gameplay/CalmyCharacter.cpp').read_text(encoding='utf-8-sig')
        p=get_profile('bodycam');steady=get_profile('steady')
        self.assertIn(f"const FVector Offset({p['forward_cm']}.f,0.f,{p['up_cm']}.f+Motion.Z",source)
        self.assertIn(f"Out.FOV=bSteadyCamera?{steady['fov']}.f:{p['fov']}.f",source)


if __name__=='__main__':unittest.main()
