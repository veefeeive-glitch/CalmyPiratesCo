"""Compile Nanite usage on imported instances only; never edit shared engine materials."""
import unreal
DEST='/Game/Ships/OceanFreighterR7'
lib=unreal.MaterialEditingLibrary
count=0
for path in unreal.EditorAssetLibrary.list_assets(DEST,recursive=True,include_folder=False):
    m=unreal.load_asset(path)
    if not isinstance(m,unreal.MaterialInstanceConstant):continue
    lib.set_material_usage_override(m,unreal.MaterialUsage.MATUSAGE_NANITE,True,True)
    lib.update_material_instance(m)
    unreal.log('R7_USAGE '+path+' parent='+str(m.parent)+' roughness='+str(lib.get_material_instance_scalar_parameter_value(m,'RoughnessFactor')))
    for entry in m.get_editor_property('texture_parameter_values'):
        if str(entry.parameter_info.name)=='MetallicRoughnessTexture':
            tex=entry.parameter_value
            unreal.log('R7_ORM '+tex.get_path_name()+' srgb='+str(tex.get_editor_property('srgb')))
    assert unreal.EditorAssetLibrary.save_loaded_asset(m);count+=1
assert count>=20
unreal.log('R7_USAGE_PASS instances='+str(count))
