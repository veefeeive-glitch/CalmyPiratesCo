# Среда и запуск — 30 октября 2026

Основной проект — CalmyPiratesCoUE, Unreal Engine 5.8.2 (EngineAssociation 5.8). Локальный движок проверен по Build.version. Blender 5.2.1 LTS, VS Community 2026 / MSVC 14.50 / SDK 10.0.22621.0 — версии прежних успешных прогонов; их работоспособность на другом ПК нужно подтвердить. [Компоненты VS](../../.vsconfig).

## Команды из корня проекта

```powershell
./Scripts/Project.ps1 -Action Check -EngineRoot 'E:/Games/UE_5.8' -BlenderExe 'E:/Tools/blender-5.2.1-windows-x64/blender.exe'
./Scripts/Project.ps1 -Action Build -EngineRoot 'E:/Games/UE_5.8'
./Scripts/Project.ps1 -Action Automation -EngineRoot 'E:/Games/UE_5.8'
./Scripts/Project.ps1 -Action Play -EngineRoot 'E:/Games/UE_5.8' -StartPaused
./Scripts/Project.ps1 -Action Play -EngineRoot 'E:/Games/UE_5.8' -MapName PortHub
./Scripts/Project.ps1 -Action Models -BlenderExe 'E:/Tools/blender-5.2.1-windows-x64/blender.exe'
```

Требуется PowerShell 7+. `Check` проверяет пути и версию UE, но не компилятор/GPU. `-DryRun` у Build/Automation/Play/Models показывает команду. Models открывает исходник уже импортированного CrewR8; сам импорт не выполняется. После удаления Intermediate/DDC/.vs они создадутся заново по мере работы. Решение IDE можно повторно сгенерировать для .uproject.

Play по умолчанию открывает WaterLab. Обучение ведёт в PortHub; доска контрактов отправляет команду на OceanFreighterR7. Сохранённый прогресс учитывается игрой. `-MapName PortHub` и `-MapName OceanFreighterR7` дают прямой вход. [Play-Calmy.cmd](../../Scripts/Play-Calmy.cmd) использует start_game.ps1. Путь движка можно передать через `-EngineRoot` или переменную `CALMY_UE_ROOT`; запасной локальный путь — E:/Games/UE_5.8.

## Сеть и проверки

```powershell
./Scripts/start_game.ps1 -Mode Host -MapName OceanFreighterR7 -EngineRoot 'E:/Games/UE_5.8'
./Scripts/start_game.ps1 -Mode Client -JoinAddress '127.0.0.1:17777' -EngineRoot 'E:/Games/UE_5.8'
./Scripts/run_network_smoke.ps1 -Suite Session -Peers 4 -Editor 'E:/Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
```

По умолчанию используется loopback; LAN host/join доступен из меню экипажа. Steam/internet invites не реализованы. Не менять firewall автоматически. Automation использует отдельный UserDir и проверяет результаты тестов, а не только код выхода. [Границы проверок](VALIDATION.md).

Перед компиляцией сохранить свою работу и штатно закрыть редактор. Пакетирование — отдельная операция: старый package_windows.ps1 автоматически продвигает результат и удаляет previous. Он не является bootstrap-командой. Сентябрьские пакеты устарели; свежего релиза эта чистка не создаёт.

## Авторинг и локальные инструменты

[Модели](../Handover/MODELS.md) · [мастер судна](../../SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend) · [каталог скриптов](../../Scripts/README.md). Сохранять внешние PNG рядом с Blender-файлами. Старые import/create/fix/freeze/publish скрипты выполняют изменения и не должны запускаться пакетом для «настройки всего».

MCP-конфигурация .codex/.mcp.json локальная; наличие файла не подтверждает живой сервер. Unreal/Blender MCP должны оставаться на loopback. Включённые Editor-плагины ModelContextProtocol/AllToolsets необходимо иметь в установке UE. [Снимок инструкции calmy-dream-loop](../Handover/Tooling/calmy-dream-loop/SKILL.md) не означает установленный навык на другом ПК. Higgsfield отложен: [запись 23 сентября](../Changes/HIGGSFIELD_MCP_DEFERRED_2026-09-23.md); подписка и импорт не выполнялись.

GitHub-копия использует отдельную очищенную историю и приватный remote игры. Evidence/Saved/Builds не входят в обычный клон. [Перенос и резервирование](../Handover/TRANSFER.md).


Предыдущий текст со всеми этапами и деталями сохранён в [истории документа](../History/Before-Cleanup-2026-10-30/Docs/Current/OPERATIONS.md).
