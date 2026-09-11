"""Nightwatch presentation pass on current ship + WaterLab. No geometry/collision edits."""
import unreal,hashlib,json,time
from pathlib import Path
root=Path(__file__).resolve().parents[1];evidence=root/'Saved/Verification/Nightwatch-R1-20260908'
assets=unreal.EditorAssetLibrary;lib=unreal.MaterialEditingLibrary;tools=unreal.AssetToolsHelpers.get_asset_tools()
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
dest='/Game/Atmosphere/NightwatchR1'
resume='NightwatchResumeWaterlab' in unreal.SystemLibrary.get_command_line()
assert resume or not assets.does_directory_exist(dest),'Preserve existing Nightwatch revision'
sounds={}
for name in ('Wind','Hull'):
    if resume:sounds[name]=unreal.load_asset(dest+'/'+name);assert sounds[name];continue
    t=unreal.AssetImportTask();t.filename=str(root/'SourceAssets/Generated/NightwatchAudioR1'/(name+'.wav'));t.destination_path=dest;t.automated=True;t.save=True
    tools.import_asset_tasks([t]);sounds[name]=unreal.load_asset(t.imported_object_paths[0]);sounds[name].set_editor_property('looping',True);assert assets.save_loaded_asset(sounds[name])
report={}
for mapname in (('WaterLab',) if resume else ('OceanFreighterR7','WaterLab')):
    path=root/'Content/Maps'/(mapname+'.umap');before=hashlib.sha256(path.read_bytes()).hexdigest()
    backup=root/'SourceAssets/Backups/Before-Nightwatch-R1-20260908'/(mapname+'.umap')
    assert before==hashlib.sha256(backup.read_bytes()).hexdigest(),'Map changed after backup'
    assert levels.load_level('/Game/Maps/'+mapname)
    oldactors=list(actors.get_all_level_actors());lightlog=[]
    for a in oldactors:
        label=a.get_actor_label()
        if isinstance(a,unreal.DirectionalLight):
            c=a.light_component;lightlog.append([label,c.get_editor_property('intensity')])
            a.set_actor_rotation(unreal.Rotator(pitch=-10,yaw=-65),False);c.set_editor_property('intensity',.65);c.set_light_color(unreal.LinearColor(.45,.60,.8))
        elif isinstance(a,unreal.SkyLight):
            c=a.light_component;c.set_editor_property('intensity',.35);c.set_light_color(unreal.LinearColor(.45,.58,.72));c.set_editor_property('real_time_capture',True)
        elif isinstance(a,unreal.PointLight):
            c=a.point_light_component;lightlog.append([label,c.get_editor_property('intensity')])
            c.set_editor_property('use_temperature',True)
            if 'Engine_Worklight' in label:c.set_editor_property('intensity',160.);c.set_editor_property('temperature',3500.)
            elif 'Hold_Worklight' in label:c.set_editor_property('intensity',480.);c.set_editor_property('temperature',4100.)
            elif 'Corridor' in label:c.set_editor_property('intensity',250.);c.set_editor_property('temperature',3800.)
            else:c.set_editor_property('intensity',min(340.,c.get_editor_property('intensity')*.4));c.set_editor_property('temperature',4200.)
        # Avoid local old bias overriding the newly authored global night exposure.
        if isinstance(a,unreal.PostProcessVolume) and label=='EngineDream_R1_Local_Exposure':
            s=a.get_editor_property('settings');s.set_editor_property('override_auto_exposure_bias',False);a.set_editor_property('settings',s)
        if a.get_class().get_name()=='CalmyOcean':
            old=a.get_editor_property('surface_material');target=dest+'/M_'+mapname+'_NightSea';m=unreal.load_asset(target) if resume else assets.duplicate_asset(old.get_path_name().split('.')[0],target);assert m
            # Duplicate graph preserves R7 exact dry-hull opacity mask and existing wave response.
            tint=[n for n in lib.get_material_expressions(m) if isinstance(n,unreal.MaterialExpressionVectorParameter) and str(n.get_editor_property('parameter_name'))=='Tint'];assert len(tint)==1
            tint[0].set_editor_property('default_value',unreal.LinearColor(.008,.021,.029,1))
            lib.recompile_material(m);assert assets.save_loaded_asset(m);a.set_editor_property('surface_material',m)
    p=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector());p.set_actor_label('Nightwatch_Global_Grade');p.set_editor_property('unbound',True);p.set_editor_property('priority',20.)
    s=p.get_editor_property('settings')
    for name,value in {'auto_exposure_bias':-.45,'auto_exposure_min_brightness':-1.,'auto_exposure_max_brightness':3.,'bloom_intensity':.10,'vignette_intensity':.32,'film_grain_intensity':.07}.items():
        s.set_editor_property('override_'+name,True);s.set_editor_property(name,value)
    for name,value in {'color_saturation':unreal.Vector4(.80,.80,.80,1),'color_contrast':unreal.Vector4(1.06,1.06,1.06,1),'color_gain':unreal.Vector4(.94,.99,1.03,1)}.items():
        s.set_editor_property('override_'+name,True);s.set_editor_property(name,value)
    p.set_editor_property('settings',s)
    sea=720 if mapname=='OceanFreighterR7' else -220
    fog=actors.spawn_actor_from_class(unreal.ExponentialHeightFog,unreal.Vector(0,0,sea-100));fog.set_actor_label('Nightwatch_SeaHaze')
    f=fog.get_component_by_class(unreal.ExponentialHeightFogComponent);f.set_fog_density(.018);f.set_fog_height_falloff(.2);f.set_fog_inscattering_color(unreal.LinearColor(.028,.042,.060));f.set_volumetric_fog(True);f.set_volumetric_fog_distance(16000);f.set_volumetric_fog_emissive(unreal.LinearColor(.002,.003,.005));f.set_editor_property('fog_max_opacity',.82)
    # Deck lamps illuminate circulation; no new collision/obstacles. Warm pools break broad daylight.
    lamps=[(-2600,850,1320),(-2600,-850,1320),(-1000,-850,1300),(600,-850,1300),(2600,-850,1300),(4500,-650,1300)] if mapname=='OceanFreighterR7' else [(-950,650,360),(0,-650,360),(1400,0,360)]
    for i,pos in enumerate(lamps):
        a=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(*pos));a.set_actor_label('Nightwatch_DeckPool_'+str(i));c=a.point_light_component
        c.set_editor_property('intensity_units',unreal.LightUnits.LUMENS);c.set_editor_property('intensity',500.);c.set_editor_property('attenuation_radius',700.);c.set_editor_property('source_radius',9.);c.set_editor_property('use_temperature',True);c.set_editor_property('temperature',3100.);c.set_editor_property('cast_shadows',True)
    for name in ('Wind','Hull') if mapname=='OceanFreighterR7' else ('Wind',):
        a=actors.spawn_actor_from_class(unreal.AmbientSound,unreal.Vector());a.set_actor_label('Nightwatch_'+name+'_Bed');c=a.audio_component;c.set_sound(sounds[name]);c.set_volume_multiplier(.18 if name=='Wind' else .12)
    assert levels.save_current_level()
    report[mapname]={'before_sha256':before,'after_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'previous_lights':lightlog,'new_deck_lights':len(lamps),'ship_geometry_changed':False}
    (evidence/('round1-resume.json' if resume else 'round1.json')).write_text(json.dumps(report,indent=2))
unreal.log('NIGHTWATCH_APPLY_PASS maps=2')
_after=None
def finish(dt):
    global _after
    if _after is None:_after=time.monotonic()+12
    if time.monotonic()>=_after:unreal.unregister_slate_post_tick_callback(_handle);unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(finish)
