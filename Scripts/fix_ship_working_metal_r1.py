"""Scoped R7 material correction. New material assets + map overrides; no mesh/master edits."""
from pathlib import Path
import unreal,json,hashlib
ROOT=Path(__file__).resolve().parents[1]
DEST='/Game/Ships/OceanFreighterR7/MetalR1B'
lib=unreal.MaterialEditingLibrary;assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert not assets.does_directory_exist(DEST),'Preserve existing material candidate'
assert levels.load_level('/Game/Maps/OceanFreighterR7')
paint={'MAT_Machinery':.68,'MAT_Machinery_Blue':.62,'MAT_Superstructure':.70,
       'MAT_SeaWaterPipe':.60,'MAT_FireMainPipe':.60,'MAT_Antifouling_Red':.78,
       'MAT_Hull_Navy':.58,'MAT_BootTop_Black':.68,'MAT_Safety_Orange':.60,
       'MAT_Hazard_Yellow':.62,'MAT_Emergency_Red':.62}
targets=[]
for a in actors.get_all_level_actors():
    if not isinstance(a,unreal.StaticMeshActor):continue
    c=a.static_mesh_component;mesh=c.static_mesh
    if not mesh or not mesh.get_name().startswith('SM_R7_'):continue
    for i in range(c.get_num_materials()):
        m=c.get_material(i)
        if m and (m.get_name()=='MAT_Steel_PBR' or m.get_name() in paint):targets.append((a,c,i,m))
assert any(m.get_name()=='MAT_Steel_PBR' for _,_,_,m in targets)

# Duplicate the glTF root and its opaque/double-sided parent. Preserve texture,
# normal, UV and inherited static switches; never modify shared Interchange content.
steel=next(m for _,_,_,m in targets if m.get_name()=='MAT_Steel_PBR')
parent=steel.get_editor_property('parent');root=parent.get_editor_property('parent')
assert isinstance(parent,unreal.MaterialInstanceConstant) and isinstance(root,unreal.Material)
base=assets.duplicate_asset(root.get_path_name().split('.')[0],DEST+'/M_WorkingSteel')
assert base and not base.get_editor_property('use_material_attributes')
rough=lib.get_material_property_input_node(base,unreal.MaterialProperty.MP_ROUGHNESS)
output=lib.get_material_property_input_node_output_name(base,unreal.MaterialProperty.MP_ROUGHNESS)
assert rough
scale=lib.create_material_expression(base,unreal.MaterialExpressionMultiply)
scale.set_editor_property('const_b',4.)
offset=lib.create_material_expression(base,unreal.MaterialExpressionAdd)
offset.set_editor_property('const_b',.48)
clamp=lib.create_material_expression(base,unreal.MaterialExpressionClamp)
clamp.set_editor_property('min_default',.48);clamp.set_editor_property('max_default',.82)
assert lib.connect_material_expressions(rough,output,scale,'A')
assert lib.connect_material_expressions(scale,'',offset,'A')
assert lib.connect_material_expressions(offset,'',clamp,str(lib.get_material_expression_input_names(clamp)[0]))
assert lib.connect_material_property(clamp,'',unreal.MaterialProperty.MP_ROUGHNESS)
lib.set_material_usage(base,unreal.MaterialUsage.MATUSAGE_NANITE)
errors=lib.recompile_material(base);assert not errors,str(errors)
assert assets.save_loaded_asset(base)
steel_parent=assets.duplicate_asset(parent.get_path_name().split('.')[0],DEST+'/MI_WorkingSteel_Parent')
lib.set_material_instance_parent(steel_parent,base);lib.update_material_instance(steel_parent)
assert assets.save_loaded_asset(steel_parent)
replacements={};report=[]
for a,c,index,original in targets:
    path=original.get_path_name()
    if path not in replacements:
        mat=assets.duplicate_asset(path.split('.')[0],DEST+'/MI_'+original.get_name()+'_'+str(len(replacements)))
        assert isinstance(mat,unreal.MaterialInstanceConstant)
        if original.get_name()=='MAT_Steel_PBR':
            lib.set_material_instance_parent(mat,steel_parent)
            lib.set_material_instance_scalar_parameter_value(mat,'NormalScale',.18)
        else:
            lib.set_material_instance_scalar_parameter_value(mat,'MetallicFactor',0.)
            lib.set_material_instance_scalar_parameter_value(mat,'RoughnessFactor',paint[original.get_name()])
        lib.set_material_usage_override(mat,unreal.MaterialUsage.MATUSAGE_NANITE,True,True)
        lib.update_material_instance(mat);assert assets.save_loaded_asset(mat)
        replacements[path]=mat
    c.set_material(index,replacements[path])
    report.append({'actor':a.get_actor_label(),'slot':index,'original':path,'new':replacements[path].get_path_name()})
assert levels.save_current_level()
record={'scope':'R7 active static-mesh component material overrides only','steel_roughness':'clamp(0.48 + 4 * originalRoughness, 0.48, 0.82)',
        'steel_sample_range':[.48+4*4/255,.48+4*15/255],'paint_metallic':0,'paint_roughness':paint,
        'map_sha256':hashlib.sha256((ROOT/'Content/Maps/OceanFreighterR7.umap').read_bytes()).hexdigest(),'assignments':report}
(ROOT/'Saved/Verification/MetalR1-materials.json').write_text(json.dumps(record,indent=2),encoding='utf8')
unreal.log('SHIP_METAL_R1_PASS instances='+str(len(replacements))+' assignments='+str(len(report))+' mesh_changes=0 master_changes=0')
