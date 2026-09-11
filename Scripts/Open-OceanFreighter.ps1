$ErrorActionPreference = 'Stop'

$blender = 'E:\Tools\blender-5.2.1-windows-x64\blender.exe'
$projectRoot = Split-Path -Parent $PSScriptRoot
$master = Join-Path $projectRoot 'SourceAssets\Generated\OceanFreighter\CalmyOceanFreighter.blend'
$startup = Join-Path $PSScriptRoot 'open_ocean_freighter_gui.py'

Start-Process -FilePath $blender -ArgumentList @(
    ('"{0}"' -f $master),
    '--python',
    ('"{0}"' -f $startup)
)
