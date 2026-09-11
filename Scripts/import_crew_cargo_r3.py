"""Import only new R3 assets, then scoped WaterLab crew/foam placement."""
from pathlib import Path
import unreal
ROOT=Path(__file__).resolve().parents[1]
tools=unreal.AssetToolsHelpers.get_asset_tools()
mesh_editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
assert mesh_editor
for f in sorted((ROOT/'SourceAssets/Generated/CrewCargoR3').glob('SM_*.glb')):
    dest='/Game/Characters/DeckhandR3' if 'Deckhand' in f.stem else '/Game/Cargo/R3'
    target=dest+'/'+f.stem
    if unreal.EditorAssetLibrary.does_asset_exist(target):mesh=unreal.load_asset(target)
    else:
        task=unreal.AssetImportTask();task.filename=str(f);task.destination_path=dest;task.automated=True;task.save=True
        tools.import_asset_tasks([task]);candidates=[unreal.load_asset(p) for p in task.imported_object_paths]
        mesh=next((a for a in candidates if isinstance(a,unreal.StaticMesh)),None)
    assert mesh,f
    if mesh.get_path_name().split('.')[0]!=target:assert unreal.EditorAssetLibrary.rename_asset(mesh.get_path_name().split('.')[0],target)
    mesh_editor.remove_collisions(mesh)
    if 'Deckhand' in f.stem:assert mesh_editor.add_simple_collisions(mesh,unreal.ScriptCollisionShapeType.NDOP26)>=0
    settings=mesh_editor.get_nanite_settings(mesh);settings.enabled=False;mesh_editor.set_nanite_settings(mesh,settings,True)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh);unreal.log('R3_IMPORT '+target+' '+str(mesh.get_bounds()))

lib=unreal.MaterialEditingLibrary
path='/Game/Materials/M_WaterLab_FoamR3'
foam=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('M_WaterLab_FoamR3','/Game/Materials',unreal.Material,unreal.MaterialFactoryNew())
lib.delete_all_material_expressions(foam)
foam.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT)
foam.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
foam.set_editor_property('two_sided',True)
color=lib.create_material_expression(foam,unreal.MaterialExpressionVertexColor)
lib.connect_material_property(color,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.connect_material_property(color,'A',unreal.MaterialProperty.MP_OPACITY)
lib.recompile_material(foam);unreal.EditorAssetLibrary.save_loaded_asset(foam)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/WaterLab')
crew_class=unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyCrewHelper')
existing=[a for a in actors.get_all_level_actors() if a.get_class()==crew_class]
assert len(existing)<=1,'Do not duplicate authored crew'
npc=existing[0] if existing else actors.spawn_actor_from_class(crew_class,unreal.Vector(850,-80,90),unreal.Rotator(yaw=180))
npc.set_actor_label('R3_Misha_Persistent_Crew')
for a in actors.get_all_level_actors():
    if a.get_class()==unreal.load_class(None,'/Script/CalmyPiratesCo.CalmyOcean'):a.set_editor_property('foam_material',foam)
if not any(a.get_actor_label()=='R3_Crew_Station_Sign' for a in actors.get_all_level_actors()):
    sign=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(920,90,165),unreal.Rotator(yaw=180))
    sign.set_actor_label('R3_Crew_Station_Sign')
    text=sign.get_component_by_class(unreal.TextRenderComponent)
    text.set_text('MISHA - DECK CREW\nApproach and press H\nNeed an extra pair of hands?')
    text.set_world_size(16);text.set_text_render_color(unreal.Color(240,210,120,255))
assert levels.save_current_level()
unreal.log('R3_IMPORT_PASS 14 meshes, persistent crew, water interaction material')
# ExecutePythonScript runs during startup. Let Slate finish constructing the
# level editor before requesting shutdown (early quit can tear down mode tools).
import time
_quit_after=time.monotonic()+12
def _finish_import(_dt):
    if time.monotonic()>=_quit_after:
        unreal.unregister_slate_post_tick_callback(_quit_handle)
        unreal.SystemLibrary.quit_editor()
_quit_handle=unreal.register_slate_post_tick_callback(_finish_import)
