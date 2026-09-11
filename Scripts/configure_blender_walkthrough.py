"""Persist comfortable first-person navigation defaults for ship review."""
import bpy

inputs = bpy.context.preferences.inputs
inputs.navigation_mode = "WALK"
inputs.view_rotate_method = "TURNTABLE"
inputs.view_zoom_axis = "VERTICAL"
inputs.view_zoom_method = "DOLLY"

walk = inputs.walk_navigation
walk.walk_speed = 2.2
walk.walk_speed_factor = 3.0
walk.view_height = 1.72
walk.jump_height = 0.45
walk.teleport_time = 0.15
walk.use_gravity = True

bpy.ops.wm.save_userpref()
print("CALMY_BLENDER_WALK_PREFS_PASS mode=WALK speed=2.2 height=1.72 gravity=1")
