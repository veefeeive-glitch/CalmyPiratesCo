"""Tune our W1 candidate only; softer foam edges and no white emissive hoops."""
import unreal
lib=unreal.MaterialEditingLibrary
mat=unreal.load_asset('/Game/Materials/M_WaterInteraction_W1')
assert mat
color=lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_EMISSIVE_COLOR)
if isinstance(color,unreal.MaterialExpressionMultiply):
    # One bounded correction of our own first soft-edge candidate.
    assert abs(color.get_editor_property('const_b')-.03)<.00001, 'Already final or unknown candidate; inspect first'
    color.set_editor_property('const_b',.12)
    lib.recompile_material(mat);assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log('WATER_INTERACTION_GAIN_FINAL .12')
    # Existing graph is complete. Skip graph construction without a SystemExit.
    color=None
else:
    assert isinstance(color,unreal.MaterialExpressionVertexColor), 'Different candidate; inspect first'
def node(cls): return lib.create_material_expression(mat,cls)
def wire(a,output,b,input_name): assert lib.connect_material_expressions(a,output,b,input_name)
if color is not None:
    gain=node(unreal.MaterialExpressionMultiply);gain.set_editor_property('const_b',.12)
    wire(color,'',gain,'A');assert lib.connect_material_property(gain,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    uv=node(unreal.MaterialExpressionTextureCoordinate)
    edges=[]
    for channel in ('r','g'):
        mask=node(unreal.MaterialExpressionComponentMask)
        mask.set_editor_property('r',channel=='r');mask.set_editor_property('g',channel=='g');mask.set_editor_property('b',False);mask.set_editor_property('a',False)
        wire(uv,'',mask,'')
        inverse=node(unreal.MaterialExpressionOneMinus);wire(mask,'',inverse,'')
        edge=node(unreal.MaterialExpressionMultiply);wire(mask,'',edge,'A');wire(inverse,'',edge,'B');edges.append(edge)
    soft=node(unreal.MaterialExpressionMultiply);wire(edges[0],'',soft,'A');wire(edges[1],'',soft,'B')
    scale=node(unreal.MaterialExpressionMultiply);scale.set_editor_property('const_b',16.);wire(soft,'',scale,'A')
    opacity=node(unreal.MaterialExpressionMultiply);wire(scale,'',opacity,'A');wire(color,'A',opacity,'B')
    assert lib.connect_material_property(opacity,'',unreal.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat);assert unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log('WATER_INTERACTION_MATERIAL_TUNED gain=.12 soft_edges=16*u*(1-u)*v*(1-v)')
