"""Read-only material/actor inspection; no save, import or map generation."""
import unreal
foam = unreal.load_asset('/Game/Materials/M_WaterLab_FoamR3')
unreal.log('WATER_MATERIAL foam=' + str(foam))
for prop in ('blend_mode', 'shading_model', 'two_sided'):
    unreal.log('WATER_MATERIAL ' + prop + '=' + str(foam.get_editor_property(prop)))
for prop in (unreal.MaterialProperty.MP_EMISSIVE_COLOR, unreal.MaterialProperty.MP_OPACITY):
    unreal.log('WATER_MATERIAL input=' + str(prop) + ' node=' + str(unreal.MaterialEditingLibrary.get_material_property_input_node(foam, prop)))
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
assert levels.load_level('/Game/Maps/WaterLab')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if actor.get_class().get_name() == 'CalmyOcean':
        unreal.log('WATER_MATERIAL ocean_foam=' + str(actor.get_editor_property('foam_material')))
