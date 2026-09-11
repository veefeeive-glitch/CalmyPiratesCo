"""One-map render correction after the first GPU review; preserves Blender and WaterLab."""
import unreal, time
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DEST='/Game/Ships/OceanFreighterR7'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
materials=set()
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.StaticMeshActor):
        mesh=a.static_mesh_component.static_mesh
        if mesh and mesh.get_path_name().startswith(DEST+'/SM_R7_'):
            settings=editor.get_nanite_settings(mesh)
            settings.enabled=True
            settings.fallback_target=unreal.NaniteFallbackTarget.PERCENT_TRIANGLES
            settings.fallback_percent_triangles=1.0
            settings.fallback_relative_error=0.0
            editor.set_nanite_settings(mesh,settings,True)
            assert mesh.get_editor_property('body_setup').get_editor_property('collision_trace_flag')==unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
            assert unreal.EditorAssetLibrary.save_loaded_asset(mesh)
            for slot in mesh.get_editor_property('static_materials'):
                if slot.material_interface:materials.add(slot.material_interface.get_path_name())
            unreal.log('SHIP_R7_NANITE '+mesh.get_name())
    if isinstance(a,unreal.PointLight):
        c=a.point_light_component;c.set_mobility(unreal.ComponentMobility.MOVABLE)
        label=a.get_actor_label()
        radius=500 if 'Hold' in label else 350 if 'Engine' in label else 260
        c.set_editor_property('attenuation_radius',radius)
    elif isinstance(a,unreal.DirectionalLight):a.light_component.set_mobility(unreal.ComponentMobility.MOVABLE)
for path in sorted(materials):
    m=unreal.load_asset(path)
    if isinstance(m,unreal.MaterialInstanceConstant):
        unreal.log('SHIP_R7_MATERIAL '+path+' SCALARS '+str(m.get_editor_property('scalar_parameter_values'))+' TEXTURES '+str(m.get_editor_property('texture_parameter_values')))
assert levels.save_current_level()
unreal.log('SHIP_R7_REFINE_PASS nanite=7 fallback_triangles=100percent reduced_light_overlap=1')
_ticks=0
def finish(_dt):
    global _ticks
    _ticks+=1
    if _ticks>600:
        unreal.unregister_slate_post_tick_callback(_handle);unreal.SystemLibrary.quit_editor()
_handle=unreal.register_slate_post_tick_callback(finish)
