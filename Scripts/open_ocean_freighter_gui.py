"""Finish GUI startup for the freighter review workspace.

Run after the .blend file on the Blender command line.  The delayed callback
lets the GUI finish creating its screen before selecting the walkthrough
camera and guarantees that the already-installed Blender MCP add-on is live.
"""
import bpy
import runpy
from pathlib import Path


def finish_startup():
    runpy.run_path(str(Path(__file__).with_name('ocean_freighter_review_panel.py')))['register']()
    if "blender_mcp" not in bpy.context.preferences.addons:
        bpy.ops.preferences.addon_enable(module="blender_mcp")

    scene = bpy.context.scene
    scene.blendermcp_auto_start_server = True
    scene.blendermcp_port = 9876
    scene.camera = bpy.data.objects.get("CAM_Walkthrough_FirstPerson") or scene.camera

    server = getattr(bpy.types, "blendermcp_server", None)
    if server is None or not server.running:
        bpy.ops.blendermcp.start_server()

    screen = getattr(bpy.context, "screen", None)
    if screen:
        for area in screen.areas:
            if area.type == "VIEW_3D":
                area.spaces.active.region_3d.view_perspective = "CAMERA"

    print("CALMY_FREIGHTER_GUI_READY camera=CAM_Walkthrough_FirstPerson mcp_port=9876")
    return None


bpy.app.timers.register(finish_startup, first_interval=1.0)
