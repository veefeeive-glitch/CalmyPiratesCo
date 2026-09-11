"""Second bounded round: neutral indoor deck, less warm glare, restore local exposure."""
import unreal,json,hashlib,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1];MAP=ROOT/'Content/Maps/OceanFreighterR7.umap'
assert hashlib.sha256(MAP.read_bytes()).hexdigest()=='28d51166b0b6f52b8849f01d05d44a5a018d8dd436e04a93d7df89c8ac1ada82'
DEST='/Game/Ships/OceanFreighterR7/EngineDreamR2'
assets=unreal.EditorAssetLibrary;lib=unreal.MaterialEditingLibrary
assert not assets.does_directory_exist(DEST)
shutil.copy2(MAP,ROOT/'SourceAssets/Backups/Before-EngineDream-R1-20260908/OceanFreighterR7.round1.umap')
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
changes=[];lights=0;volumes=0
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.PostProcessVolume) and a.get_actor_label()=='EngineDream_R1_Local_Exposure':
        s=a.get_editor_property('settings');s.set_editor_property('auto_exposure_bias',-.25);a.set_editor_property('settings',s);volumes+=1
    if isinstance(a,unreal.PointLight) and a.get_actor_label().startswith('R7_Engine_Worklight'):
        c=a.point_light_component;c.set_editor_property('intensity',325.);c.set_editor_property('temperature',5000.);lights+=1
    if not isinstance(a,unreal.StaticMeshActor):continue
    c=a.static_mesh_component
    if not c.static_mesh or c.static_mesh.get_name()!='SM_R7_20_DECKS_BULKHEADS':continue
    for i in range(c.get_num_materials()):
        old=c.get_material(i)
        if old.get_name()!='MAT_Deck_Green_PBR':continue
        parent=old.get_editor_property('parent');root=parent.get_editor_property('parent')
        assert isinstance(root,unreal.Material)
        base=assets.duplicate_asset(root.get_path_name().split('.')[0],DEST+'/M_EngineDeck')
        inp=lib.get_material_property_input_node(base,unreal.MaterialProperty.MP_BASE_COLOR)
        output=lib.get_material_property_input_node_output_name(base,unreal.MaterialProperty.MP_BASE_COLOR)
        position=lib.create_material_expression(base,unreal.MaterialExpressionWorldPosition)
        tint=lib.create_material_expression(base,unreal.MaterialExpressionCustom)
        tint.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT3)
        inputs=[]
        for name in ['C','P']:
            v=unreal.CustomInput();v.set_editor_property('input_name',name);inputs.append(v)
        tint.set_editor_property('inputs',inputs)
        # Static R7 only. Preserve weather deck and every pixel outside machinery bounds.
        tint.set_editor_property('code','if(P.x > -5600 && P.x < -2200 && abs(P.y) < 950 && P.z < 1000) { float l=dot(C,float3(0.2126,0.7152,0.0722)); return l*float3(0.48,0.56,0.57); } return C;')
        assert lib.connect_material_expressions(inp,output,tint,'C')
        assert lib.connect_material_expressions(position,'',tint,'P')
        assert lib.connect_material_property(tint,'',unreal.MaterialProperty.MP_BASE_COLOR)
        lib.set_material_usage(base,unreal.MaterialUsage.MATUSAGE_NANITE)
        assert not lib.recompile_material(base)
        assert assets.save_loaded_asset(base)
        new_parent=assets.duplicate_asset(parent.get_path_name().split('.')[0],DEST+'/MI_EngineDeck_Parent')
        lib.set_material_instance_parent(new_parent,base);lib.update_material_instance(new_parent);assert assets.save_loaded_asset(new_parent)
        new=assets.duplicate_asset(old.get_path_name().split('.')[0],DEST+'/MI_EngineDeck')
        lib.set_material_instance_parent(new,new_parent)
        lib.set_material_usage_override(new,unreal.MaterialUsage.MATUSAGE_NANITE,True,True)
        lib.update_material_instance(new);assert assets.save_loaded_asset(new)
        c.set_material(i,new);changes.append({'actor':a.get_actor_label(),'slot':i,'old':old.get_path_name(),'new':new.get_path_name()})
assert lights==18 and volumes==1 and len(changes)==1
assert levels.save_current_level()
report={'round':2,'lumens':325,'temperature':5000,'exposure_bias':-.25,'materials':changes,'static_world_position_tint_only':True,'map_sha256':hashlib.sha256(MAP.read_bytes()).hexdigest()}
(ROOT/'Saved/Verification/EngineDream-R1-20260908/round2-changes.json').write_text(json.dumps(report,indent=2),encoding='utf8')
unreal.log('ENGINE_DREAM_R2_APPLY_PASS lights=18 local_volume=1 indoor_deck_material=1 geometry_changes=0')
