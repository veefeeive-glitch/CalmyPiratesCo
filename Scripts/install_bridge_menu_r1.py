"""Append bridge entry points and one screen material to the current Nightwatch map."""
import unreal,hashlib,json,time
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'Content/Maps/OceanFreighterR7.umap'
assert hashlib.sha256(path.read_bytes()).hexdigest()=='89c6b702af47ff1c7d316d56c90704683e9c020e977c7d08bf866d97a16ac665'
assets=unreal.EditorAssetLibrary;lib=unreal.MaterialEditingLibrary
assert not assets.does_asset_exist('/Game/UI/BridgeMenu/M_BridgeTerminal')
m=unreal.AssetToolsHelpers.get_asset_tools().create_asset('M_BridgeTerminal','/Game/UI/BridgeMenu',unreal.Material,unreal.MaterialFactoryNew())
m.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property('two_sided',True)
t=lib.create_material_expression(m,unreal.MaterialExpressionTextureSampleParameter2D)
t.set_editor_property('parameter_name','Screen');t.set_editor_property('texture',unreal.load_asset('/Engine/EngineResources/WhiteSquareTexture'))
lib.connect_material_property(t,'RGB',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(m);assert assets.save_loaded_asset(m)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/Maps/OceanFreighterR7')
for i,y in enumerate((-300,-100,100,300)):
    a=actors.spawn_actor_from_class(unreal.PlayerStart,unreal.Vector(-3800,y,1989),unreal.Rotator())
    a.set_actor_label('Bridge_Entry_'+str(i));a.set_editor_property('tags',[unreal.Name('BR'+str(i))])
assert levels.save_current_level()
(root/'Saved/Verification/BridgeMenu-R1/install.json').write_text(json.dumps({'map_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'new_starts':4,'ship_geometry_changed':False},indent=2))
unreal.log('BRIDGE_MENU_INSTALL_PASS starts=4')
end=time.monotonic()+8
def finish(dt):
    if time.monotonic()>=end:unreal.unregister_slate_post_tick_callback(handle);unreal.SystemLibrary.quit_editor()
handle=unreal.register_slate_post_tick_callback(finish)
