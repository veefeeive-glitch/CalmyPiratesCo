"""Reapply already-saved R1B materials after the preview's map file lock was released."""
import unreal,json,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];DEST='/Game/Ships/OceanFreighterR7/MetalR1B'
assert hashlib.sha256((ROOT/'Content/Maps/OceanFreighterR7.umap').read_bytes()).hexdigest()=='32030a9cf403f5f24efdb08be3770aa50769b93c69d2aab3b2a52c7b7ee3dfd4','Map changed since failed save'
lib=unreal.MaterialEditingLibrary;levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
paint={'MAT_Machinery':.68,'MAT_Machinery_Blue':.62,'MAT_Superstructure':.70,'MAT_SeaWaterPipe':.60,'MAT_FireMainPipe':.60,'MAT_Antifouling_Red':.78,'MAT_Hull_Navy':.58,'MAT_BootTop_Black':.68,'MAT_Safety_Orange':.60,'MAT_Hazard_Yellow':.62,'MAT_Emergency_Red':.62}
base=unreal.load_asset(DEST+'/M_WorkingSteel');assert isinstance(lib.get_material_property_input_node(base,unreal.MaterialProperty.MP_ROUGHNESS),unreal.MaterialExpressionClamp)
replacements={};report=[]
for a in actors.get_all_level_actors():
    if not isinstance(a,unreal.StaticMeshActor):continue
    c=a.static_mesh_component;mesh=c.static_mesh
    if not mesh or not mesh.get_name().startswith('SM_R7_'):continue
    for i in range(c.get_num_materials()):
        m=c.get_material(i)
        if not m or not(m.get_name()=='MAT_Steel_PBR' or m.get_name() in paint):continue
        path=m.get_path_name()
        if path not in replacements:
            new=unreal.load_asset(DEST+'/MI_'+m.get_name()+'_'+str(len(replacements)));assert new,path
            for p in ('BaseColorTexture','NormalTexture','MetallicRoughnessTexture'):
                assert lib.get_material_instance_texture_parameter_value(new,p)==lib.get_material_instance_texture_parameter_value(m,p),path+' texture mismatch'
            if m.get_name()=='MAT_Steel_PBR':assert new.get_editor_property('parent').get_path_name().startswith(DEST+'/MI_WorkingSteel_Parent.')
            else:
                assert abs(lib.get_material_instance_scalar_parameter_value(new,'MetallicFactor'))<.001
                assert abs(lib.get_material_instance_scalar_parameter_value(new,'RoughnessFactor')-paint[m.get_name()])<.001
            replacements[path]=new
        c.set_material(i,replacements[path]);report.append({'actor':a.get_actor_label(),'slot':i,'original':path,'new':replacements[path].get_path_name()})
assert len(replacements)==40
assert levels.save_current_level()
(ROOT/'Saved/Verification/MetalR1-materials.json').write_text(json.dumps({'steel_roughness':'clamp(0.48 + 4 * originalRoughness, 0.48, 0.82)','paint_metallic':0,'paint_roughness':paint,'assignments':report},indent=2),encoding='utf8')
unreal.log('SHIP_METAL_R1_PASS instances='+str(len(replacements))+' assignments='+str(len(report))+' mesh_changes=0 master_changes=0')
