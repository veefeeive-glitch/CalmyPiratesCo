"""Create only a new W1 material; preserve old foam, maps and all imports."""
import unreal
path='/Game/Materials/M_WaterInteraction_W1'
assert not unreal.EditorAssetLibrary.does_asset_exist(path), 'Preserve existing W1 candidate; inspect instead of overwriting'
lib=unreal.MaterialEditingLibrary
mat=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_WaterInteraction_W1','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided',True)
color=lib.create_material_expression(mat,unreal.MaterialExpressionVertexColor)
# VertexColor's combined output is unnamed, not 'RGB'. Check every connection.
assert lib.connect_material_property(color,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert lib.connect_material_property(color,'A',unreal.MaterialProperty.MP_OPACITY)
assert lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_EMISSIVE_COLOR) is not None
assert lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_OPACITY) is not None
lib.recompile_material(mat)
assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('WATER_INTERACTION_MATERIAL_CREATED '+path+' emissive=vertex_rgb opacity=vertex_alpha')
