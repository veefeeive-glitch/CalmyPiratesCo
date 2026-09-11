"""Session-only Blender N-panel for reproducible, human-scale ship review.

Loaded by open_ocean_freighter_gui.py. Does not install an addon or change preferences.
"""
from pathlib import Path
from datetime import datetime
import bpy
import json
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
ZONES={
    'WEATHER':('Палуба',(-24,-6.35,12.26),(8,-6.0,12)),
    'BRIDGE':('Мостик',(-39,0,20.6),(-31.5,0,20.2)),
    'CREW':('Жилая палуба',(-53.8,0,15.1),(-33.4,0,15.1)),
    'CABIN':('Каюта',(-47.7,-2.35,15.1),(-47.7,-6,14.25)),
    'MESS':('Столовая',(-44.4,-2.45,12.35),(-51,-5,11.6)),
    'GALLEY':('Камбуз',(-39,2.35,12.35),(-39,7,11.9)),
    'LAUNDRY':('Прачечная',(-52,2.4,12.35),(-52,7,11.5)),
    'STAIRS':('Внутренняя лестница',(-48.65,2.4,12.35),(-44.5,4.7,12.4)),
    'OFFICERS':('Офицерская палуба',(-50,0,17.85),(-35,0,17.85)),
    'ENGINE':('Машинное отделение',(-49,-5.5,6.26),(-43,0,5.2)),
    'ENGINE_LOW':('Нижняя палуба МО',(-38,-3.3,3.76),(-46,0,4.1)),
    'GENERATOR':('Дизель-генераторы',(-44,-4.1,3.76),(-46,-6,3.05)),
    'PUMP':('Насосная группа',(-38.4,-3.9,3.65),(-40,-5.65,2.9)),
    'COOLER':('Теплообменники',(-41.3,-4.75,6.15),(-43,-5.5,5.4)),
    'WORKSHOP':('Судовой верстак',(-30.5,7.12,3.75),(-29.5,8.05,3.2)),
    'FIRE_POST':('Пожарный пост',(-30,-7.12,3.76),(-29.2,-8.02,3.1)),
    'CABLE_REEL':('Кабельный барабан',(-33.45,7.12,3.55),(-32.5,8.0,2.7)),
    'HOLD1':('Трюм 1',(-8,0,3.76),(-16,-4.7,5.7)),
    'HOLD2':('Трюм 2',(27,0,3.76),(38,4.7,5.7)),
    'BOSUN':('Нос / кладовая',(43.2,0,3.76),(49,-5.8,3.2)),
    'R6_EXIT':('R6: выход к наружным лестницам',(-32.7,-6.4,12.19),(-32.7,-10,11.4)),
    'R6_STERN':('R6: кормовой переход',(-55.8,8.5,12.10),(-55.8,3.5,11.2)),
    'R6_CROSSWALK':('R6: переход за двигателем',(-51.3,-4.8,6.10),(-51.3,2,5.15)),
    'R6_THRUSTER':('R6: носовой двигатель',(51.4,-2.3,3.60),(50,0,3.0)),
    'R6_MEDICAL':('R6: медпункт',(-50.5,4.8,14.94),(-52.4,6.6,14.15)),
    'R6_RADIO':('R6: радиорубка',(-50.75,4.85,17.69),(-50.75,6.5,17.2)),
}

class CALMY_OT_ship_zone(bpy.types.Operator):
    bl_idname='calmy.ship_zone'
    bl_label='Перейти'
    def execute(self,context):
        _,p,target=ZONES[context.scene.calmy_review_zone]
        camera=bpy.data.objects.get('CAM_Walkthrough_FirstPerson')
        if not camera:
            self.report({'ERROR'},'Камера обхода не найдена');return {'CANCELLED'}
        camera.location=p;camera.data.lens=26
        camera.rotation_euler=(Vector(target)-camera.location).to_track_quat('-Z','Y').to_euler()
        context.scene.camera=camera
        context.space_data.region_3d.view_perspective='CAMERA'
        return {'FINISHED'}

class CALMY_OT_ship_issue(bpy.types.Operator):
    bl_idname='calmy.ship_issue'
    bl_label='Сохранить замечание + кадр'
    def execute(self,context):
        scene=context.scene;camera=scene.camera
        if not camera:
            self.report({'ERROR'},'Нет активной камеры');return {'CANCELLED'}
        directory=ROOT/'Saved'/'ReviewIssues'/datetime.now().strftime('%Y%m%d-%H%M%S-%f')
        directory.mkdir(parents=True,exist_ok=False)
        data=dict(note=scene.calmy_review_note,zone=scene.calmy_review_zone,
                  source=bpy.data.filepath,revision=scene.get('refit_revision','unknown'),
                  camera=list(camera.location),rotation=list(camera.rotation_euler),lens=camera.data.lens)
        (directory/'issue.json').write_text(json.dumps(data,ensure_ascii=False,indent=2),encoding='utf-8')
        oldpath=scene.render.filepath
        oldformat=scene.render.image_settings.file_format
        try:
            scene.render.filepath=str(directory/'viewport.png')
            scene.render.image_settings.file_format='PNG'
            bpy.ops.render.opengl(write_still=True,view_context=True)
        finally:
            scene.render.filepath=oldpath;scene.render.image_settings.file_format=oldformat
        self.report({'INFO'},str(directory))
        return {'FINISHED'}

class CALMY_PT_ship_review(bpy.types.Panel):
    bl_label='Calmy / Обход судна'
    bl_idname='CALMY_PT_ship_review'
    bl_space_type='VIEW_3D'
    bl_region_type='UI'
    bl_category='Ship Review'
    def draw(self,context):
        layout=self.layout
        layout.prop(context.scene,'calmy_review_zone',text='Зона')
        layout.operator('calmy.ship_zone',icon='VIEW_CAMERA')
        layout.label(text='Shift + ` : ходить, WASD : движение')
        layout.label(text='Enter: оставить позицию, Esc: отменить')
        layout.separator()
        layout.prop(context.scene,'calmy_review_note',text='Ошибка')
        layout.operator('calmy.ship_issue',icon='FILE_TICK')
        layout.label(text='Обход Blender — не игровая коллизия.')

def register():
    for cls in (CALMY_OT_ship_zone,CALMY_OT_ship_issue,CALMY_PT_ship_review):
        old=getattr(bpy.types,cls.__name__,None)
        if old: bpy.utils.unregister_class(old)
        bpy.utils.register_class(cls)
    bpy.types.Scene.calmy_review_zone=bpy.props.EnumProperty(items=[(key,value[0],'') for key,value in ZONES.items()])
    bpy.types.Scene.calmy_review_note=bpy.props.StringProperty(default='',name='Замечание')

if __name__=='__main__': register()
