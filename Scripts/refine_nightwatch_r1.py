"""Second bounded lighting pass: readable safety routes, softer overcast key.

Run once after the R1 GPU/critic review; guards prevent replay over later work.
"""
import unreal, hashlib, json, time, shutil
from pathlib import Path
root=Path(__file__).resolve().parents[1]
out=root/'Saved/Verification/Nightwatch-R1-20260908'
backup=root/'SourceAssets/Backups/Nightwatch-R1-LightingRound1'
assert not backup.exists(), 'Do not overwrite round1 backup or replay refinement'
expected={'OceanFreighterR7':'96b90380543a57f1c059bb2b57fa6b12f64e5d9454145bb2a85fc584f12835c5',
          'WaterLab':'11b9e4814edca48118fdda1754bd127fbc79de76d7fc876e0a3622b758e454df'}
for name,digest in expected.items():
    assert hashlib.sha256((root/'Content/Maps'/(name+'.umap')).read_bytes()).hexdigest()==digest
backup.mkdir(parents=True)
for name in expected:shutil.copy2(root/'Content/Maps'/(name+'.umap'),backup/(name+'.umap'))
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
report={}
for name in expected:
    assert levels.load_level('/Game/Maps/'+name)
    lights=[]
    for a in actors.get_all_level_actors():
        label=a.get_actor_label()
        if isinstance(a,unreal.DirectionalLight):
            c=a.light_component;c.set_editor_property('intensity',.32)
            c.set_editor_property('light_source_angle',12.)
        elif isinstance(a,unreal.SkyLight):
            c=a.light_component;c.set_editor_property('intensity',1.25)
            c.set_light_color(unreal.LinearColor(.65,.78,1.))
        elif isinstance(a,unreal.PointLight):
            c=a.point_light_component
            c.set_editor_property('indirect_lighting_intensity',3.)
            if 'Nightwatch_DeckPool' in label:
                c.set_editor_property('intensity',230.)
                c.set_editor_property('temperature',3700.)
                c.set_editor_property('attenuation_radius',1000.)
                c.set_editor_property('source_radius',35.)
            else:
                c.set_editor_property('intensity',c.get_editor_property('intensity')*.65)
                if 'Engine_Worklight' in label:
                    c.set_editor_property('temperature',4400.)
                    c.set_editor_property('attenuation_radius',600.)
                    c.set_editor_property('source_radius',35.)
            lights.append([label,c.get_editor_property('intensity')])
        elif isinstance(a,unreal.PostProcessVolume) and label=='Nightwatch_Global_Grade':
            s=a.get_editor_property('settings')
            s.set_editor_property('override_color_gamma',True)
            s.set_editor_property('color_gamma',unreal.Vector4(1.08,1.08,1.08,1))
            s.set_editor_property('color_contrast',unreal.Vector4(1.,1.,1.,1.))
            s.set_editor_property('vignette_intensity',.22)
            a.set_editor_property('settings',s)
    # Actual three below-deck service galleries: light their occupied walk floor,
    # not just the staged engine camera. No collision or geometry modification.
    positions=[(x,700,z+205) for z in (210,460,735) for x in (-3200,-2550)] if name=='OceanFreighterR7' else []
    for i,p in enumerate(positions):
        a=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*p))
        a.set_actor_label('Nightwatch_Service_Fill_'+str(i));c=a.point_light_component
        c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
        c.set_editor_property('intensity',95.)
        c.set_editor_property('attenuation_radius',650.)
        c.set_editor_property('source_radius',30.)
        c.set_editor_property('use_temperature',True);c.set_editor_property('temperature',6200.)
        c.set_editor_property('indirect_lighting_intensity',2.)
        c.set_editor_property('cast_shadows',True)
    assert levels.save_current_level()
    report[name]={'before':expected[name],'after':hashlib.sha256((root/'Content/Maps'/(name+'.umap')).read_bytes()).hexdigest(),'new_service_fill':positions,'lights':lights}
    (out/'round2.json').write_text(json.dumps(report,indent=2))
unreal.log('NIGHTWATCH_REFINE_PASS maps=2 geometry_changed=0')
end=time.monotonic()+8
def finish(dt):
    if time.monotonic()>=end:
        unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
