# Перенос и восстановление на другом устройстве

[Главный handover](../../HANDOVER.md). Проверка на втором ПК **ещё не выполнена**. Ссылки документов относительные; исходники проекта переносимы, но Windows toolchain и локальные программы нужно установить отдельно. Linux/macOS build не тестировался.

## 1. Сохранить до копирования

Штатно сохранить/закрыть свои Blender/UE редакторы, дождаться записывающих скриптов. Не убивать чужие процессы. Выполнить `./Scripts/Maintenance/Update-HandoverInventory.ps1`; скопировать полученный Docs/Handover/INVENTORY.json вместе с проектом. Копия на том же диске не является независимым backup.

На 2026-09-11 есть локальный Git+LFS baseline ([B1](../Changes/GIT_BASELINE_B1_2026-09-11.md), commit `7bb0048…`). Клон возможен только с Git LFS и **не** заменяет копию `Evidence/` (она в `.gitignore`). Remote/push нет. `*.log` по-прежнему игнорируются. ZIP в `E:/Game/Releases` и корневые junction/symlink лежат вне этого репозитория — копировать отдельно.

## 2. Что переносить

| Папки/файлы | Назначение и обязательность |
|---|---|
| CalmyPiratesCo.uproject, Source, Config, Content | Обязательно, текущее игровое состояние и все UE-зависимости |
| SourceAssets целиком | Обязательно для авторинга: модели, текстуры, референсы, лицензии, предыдущие кандидаты/backup |
| Scripts, Docs, Evidence, root MD, .vsconfig, .gitattributes, .gitignore | Процедуры, знания, пройденные/проваленные прогоны, правила |
| .codex | Project MCP config; проверить локальный путь uvx и доверие на новом устройстве, не переносить глобальные credentials |
| Build | Сохранить custom build inputs; сейчас найдены преимущественно file-order logs |
| .git | Локальная история B1; для clone нужен LFS. Без remote — переносить каталог `.git` или сделать bundle только по явному запросу |
| Binaries, Intermediate, DerivedDataCache, .vs, *.sln/*.slnx | Восстанавливаемые данные; не полагаться на DLL другого ПК, собрать заново |
| Saved | Не нужен для сборки, но содержит НЕ все ещё замороженные diagnostics/verification, пользовательские настройки и crash logs; для полного архива сохранить отдельно |
| Builds | Старые пакеты, не актуальная игра; сохранить отдельно при необходимости исторического воспроизведения |

Для полной сохранности проще сначала сделать полную файловую копию, включая Saved/Builds, затем на новом ПК не использовать старые кэши. **Этот документ не просит удалять оригиналы.** Сначала проверка новой копии, потом отдельное решение о чистке.

Вне корня UE: Unity `E:/Game/CalmyPiratesCo`, старый stack MD в Downloads, соседние setup-документы E:/Game, сами UE/Blender/VS и пользовательский Codex skill. При необходимости переносить отдельно; инвентарь этого handover не включает содержимое соседних проектов/личного профиля.

## 3. Проверить копию до запуска

На новом ПК до обновления INVENTORY:

```powershell
./Scripts/Maintenance/Update-HandoverInventory.ps1 -Verify
```

Сверяются SHA256 всех primary файлов Source/Config/Content/Scripts и выбранных authoring/refs, записанных в старом инвентаре. Отсутствие необязательной DLL показывается отдельно и не заменяет сборку. Исторические freeze manifests можно дополнительно проверить по их собственным путям; не исправлять их хеши при переносе. Полный список legacy/source asset файлов с размерами — в инвентаре; SHA256 не считается для каждого старого backup/кэша.

## 4. Инструменты

- Unreal **5.8.2**, проект EngineAssociation `5.8`. Нужны включённые в .uproject plugins, в том числе ModelContextProtocol/AllToolsets для Editor, PythonScriptPlugin, EditorScriptingUtilities; наличие в иной поставке проверить. Движок автоматически не обновлять.
- Blender **5.2.1 LTS**. R8 использует bpy/numpy; GUI нужен для интерактивного Blender MCP, background Python достаточно для авторинга/аудита. Все 20 PNG должны оставаться рядом с R8.blend.
- Visual Studio 2026 Community, MSVC 14.50, Windows SDK 10.0.22621.0 — версии последней успешной сборки; .vsconfig описывает компоненты. PowerShell **7+** нужен новым maintenance scripts и старому doc checker.
- uvx/blender-mcp 1.9.0 — только если нужен Blender MCP. `.codex/config.toml` сейчас с пользовательским абсолютным путём; заменить на установленный uvx на этом ПК. Loopback 9876; Unreal MCP 127.0.0.1:8000. Наличие записи не означает живое подключение. Не менять firewall/слушать публичный адрес.
- Проектный calmy-dream-loop: [переносимая копия инструкции](Tooling/calmy-dream-loop/SKILL.md); это snapshot, **не автоматически установленный skill**. При ручной установке на новом устройстве исправить старый путь проекта. Учетные записи/API-токены не переносить через публичный репозиторий.

## 5. Пути, build и безопасный первый запуск

Из корня копии:

```powershell
$enginePath='D:/Unreal/UE_5.8' # заменить на существующий путь этого ПК
$blenderPath='D:/Blender/blender.exe'
./Scripts/Project.ps1 -Action Check -EngineRoot $enginePath -BlenderExe $blenderPath
./Scripts/Project.ps1 -Action Build -EngineRoot $enginePath
./Scripts/Project.ps1 -Action Automation -EngineRoot $enginePath
./Scripts/Project.ps1 -Action Play -EngineRoot $enginePath -StartPaused
```

Launcher по умолчанию открывает WaterLab; `-MapName OceanFreighterR7` — карта с **незавершённым меню рубки**. Не утверждать, что неотображение меню вызвано переносом. Check и `-DryRun` ничего не запускают и не устанавливают; отсутствие инструментов Check завершает ошибкой.

`Scripts/run_network_smoke.ps1 -Editor <UnrealEditor.exe>` и `run_helper_solo.ps1 -Editor <UnrealEditor-Cmd.exe>` допускают другой путь движка. GPU/старые start_dev/Open-OceanFreighter runners часто имеют жёстко заданные пути: см. [индекс скриптов](../../Scripts/README.md); адаптировать конкретный runner, не делать глобальную замену по SourceAssets/Evidence. Все тесты выполнять на актуальной собранной DLL, записывать новые логи отдельно.

Для открытия R8: `./Scripts/Project.ps1 -Action Models -BlenderExe $blenderPath`. Это открывает Blender, не импортирует в UE и не запускает генератор. Если материалы пропали, проверить 20 PNG и File → External Data → Find Missing Files, сохранить отдельную копию.

Не запускать `package_windows.ps1`, `install_dev_firewall_rule.ps1`, `publish_*`, `import_*`, `create_*`, `fix_*` для «настройки всего». Это отдельные изменения с рисками; импорт R8 требует отдельной задачи и регрессий.
