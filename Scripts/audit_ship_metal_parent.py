"""Read-only material graph inspection before a scoped steel correction."""
import unreal
m=unreal.load_asset('/Game/Ships/OceanFreighterR7/SM_R7_20_DECKS_BULKHEADS/Materials/MAT_Steel_PBR')
while isinstance(m,unreal.MaterialInstanceConstant):
    unreal.log('METAL_CHAIN '+m.get_path_name());m=m.get_editor_property('parent')
assert isinstance(m,unreal.Material)
unreal.log('METAL_ROOT '+m.get_path_name()+' attrs='+str(m.get_editor_property('use_material_attributes')))
for prop in (unreal.MaterialProperty.MP_ROUGHNESS,unreal.MaterialProperty.MP_BASE_COLOR,unreal.MaterialProperty.MP_MATERIAL_ATTRIBUTES):
    node=unreal.MaterialEditingLibrary.get_material_property_input_node(m,prop)
    unreal.log('METAL_INPUT '+str(prop)+' '+str(node)+' output='+unreal.MaterialEditingLibrary.get_material_property_input_node_output_name(m,prop))
unreal.log('METAL_PARENT_AUDIT_PASS')
