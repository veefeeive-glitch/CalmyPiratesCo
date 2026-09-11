"""Bounded light/material round. Map-only overrides; no geometry or gameplay changes."""
import unreal,json,hashlib,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MAP=ROOT/'Content/Maps/OceanFreighterR7.umap'
assert hashlib.sha256(MAP.read_bytes()).hexdigest()=='84021ce738cc862393e05ad1489d66e31410c6f17e7080b77d2ba63d5e5b1880','Unexpected map revision'
DEST='/Game/Ships/OceanFreighterR7/EngineDreamR1'
assets=unreal.EditorAssetLibrary;lib=unreal.MaterialEditingLibrary
assert not assets.does_directory_exist(DEST),'Preserve existing candidate'
backup=ROOT/'SourceAssets/Backups/Before-EngineDream-R1-20260908'
backup.mkdir(parents=True,exist_ok=True)
if not (backup/MAP.name).exists():shutil.copy2(MAP,backup/MAP.name)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
report={'round':1,'lights':[],'materials':[]}
# Local postprocess only, below the weather deck: no global exposure workaround.
p=actors.spawn_actor_from_class(unreal.PostProcessVolume,unreal.Vector(-3950,0,590))
p.set_actor_label('EngineDream_R1_Local_Exposure')
p.set_actor_scale3d(unreal.Vector(16.5,8.0,3.9))
p.set_editor_property('unbound',False);p.set_editor_property('blend_radius',80.)
p.set_editor_property('priority',10.)
center,ext=p.get_actor_bounds(False)
unreal.log('ENGINE_DREAM_VOLUME center='+str(center)+' ext='+str(ext))
assert ext.x>1000 and ext.y>500 and 300<ext.z<500,'Local volume has no valid brush'
s=p.get_editor_property('settings')
s.set_editor_property('override_auto_exposure_bias',True);s.set_editor_property('auto_exposure_bias',-.8)
s.set_editor_property('override_bloom_intensity',True);s.set_editor_property('bloom_intensity',.12)
p.set_editor_property('settings',s);p.set_folder_path('R7/EngineDreamR1')
report['local_volume']={'center':[center.x,center.y,center.z],'extent':[ext.x,ext.y,ext.z],'exposure_bias':-.8,'bloom':.12}
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.PointLight) and a.get_actor_label().startswith('R7_Engine_Worklight'):
        c=a.point_light_component
        c.set_editor_property('intensity',450.)
        c.set_editor_property('source_radius',8.)
        c.set_editor_property('source_length',40.)
        c.set_editor_property('use_temperature',True);c.set_editor_property('temperature',4500.)
        # Keep placement, attenuation and shadow casting; soften the point-source hotspot.
        report['lights'].append({'actor':a.get_actor_label(),'position':str(a.get_actor_location()),'lumens':450,'temperature':4500,'source_radius_cm':8,'source_length_cm':40})
    if not isinstance(a,unreal.StaticMeshActor):continue
    c=a.static_mesh_component
    if not c.static_mesh or c.static_mesh.get_name()!='SM_R7_40_MACHINERY_SYSTEMS':continue
    for i in range(c.get_num_materials()):
        old=c.get_material(i)
        if old.get_name() not in ['MAT_Screen','MAT_R4_MachineryLight']:continue
        new=assets.duplicate_asset(old.get_path_name().split('.')[0],DEST+'/MI_'+old.get_name())
        assert new
        before=str(new.get_editor_property('vector_parameter_values'))
        # Imported glTF emissive factor is a vector, not a generic scalar named Emission.
        factor=lib.get_material_instance_vector_parameter_value(old,'EmissiveFactor')
        assert max(factor.r,factor.g,factor.b)>0,'Missing glTF emissive parameter'
        scale=.10 if old.get_name()=='MAT_R4_MachineryLight' else .12
        desired=unreal.LinearColor(factor.r*scale,factor.g*scale,factor.b*scale,1.)
        # UE 5.8 MaterialEditingLibrary.cpp setter returns false unconditionally;
        # verify persisted parameter values instead of that return value.
        lib.set_material_instance_vector_parameter_value(new,'EmissiveFactor',desired)
        actual=lib.get_material_instance_vector_parameter_value(new,'EmissiveFactor')
        assert max(abs(actual.r-desired.r),abs(actual.g-desired.g),abs(actual.b-desired.b))<1.e-5
        lib.set_material_usage_override(new,unreal.MaterialUsage.MATUSAGE_NANITE,True,True)
        lib.update_material_instance(new);assert assets.save_loaded_asset(new)
        c.set_material(i,new)
        report['materials'].append({'original':old.get_path_name(),'new':new.get_path_name(),'emissive_before':str(factor),'scale':scale,'slot':i})
assert len(report['lights'])==18 and len(report['materials'])==2,str(report)
assert levels.save_current_level()
report['map_sha256']=hashlib.sha256(MAP.read_bytes()).hexdigest()
(ROOT/'Saved/Verification/EngineDream-R1-20260908/round1-changes.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('ENGINE_DREAM_R1_APPLY_PASS lights=18 materials=2 local_volume=1 geometry_changes=0')
