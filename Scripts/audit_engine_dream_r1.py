"""Read-only scene inventory for the first bounded engine-room dream loop."""
import unreal,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
rows=[]
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.Light):
        c=a.light_component;p=a.get_actor_location()
        row={'label':a.get_actor_label(),'class':a.get_class().get_name(),'xyz':[p.x,p.y,p.z]}
        for key in ['intensity','indirect_lighting_intensity','light_color','attenuation_radius','source_radius','source_length','use_temperature','temperature','cast_shadows']:
            try:row[key]=str(c.get_editor_property(key))
            except Exception:pass
        rows.append(row)
    if isinstance(a,unreal.StaticMeshActor) and a.static_mesh_component.static_mesh and a.static_mesh_component.static_mesh.get_name().startswith('SM_R7_'):
        c=a.static_mesh_component
        rows.append({'mesh':c.static_mesh.get_path_name(),'actor':a.get_actor_label(),'materials':[c.get_material(i).get_path_name() if c.get_material(i) else '' for i in range(c.get_num_materials())]})
        if c.static_mesh.get_name()=='SM_R7_40_MACHINERY_SYSTEMS':
            for i in range(c.get_num_materials()):
                m=c.get_material(i)
                if m.get_name() in ['MAT_Screen','MAT_R4_MachineryLight']:
                    unreal.log('ENGINE_DREAM_PARAMS '+m.get_name()+' vectors='+str(m.get_editor_property('vector_parameter_values'))+' scalars='+str(m.get_editor_property('scalar_parameter_values')))
    if isinstance(a,unreal.PostProcessVolume):rows.append({'postprocess':a.get_actor_label(),'settings':str(a.get_editor_property('settings'))})
out=ROOT/'Saved/Verification/EngineDream-R1-20260908'
out.mkdir(parents=True,exist_ok=True)
(out/'before-scene.json').write_text(json.dumps(rows,indent=2),encoding='utf8')
unreal.log('ENGINE_DREAM_AUDIT_PASS actors='+str(len(rows)))
