# Среда, запуск и продолжение — 2026-09-10

**11 сентября — [файловая структура F1](../Changes/FILE_ORGANIZATION_F1_2026-09-11.md).** Предпочтительный путь готового ZIP: E:/Game/Releases/Windows/2026-09-11/CalmyPiratesCo-Windows-2026-09-11.zip. Рабочий проект остаётся E:/Game/CalmyPiratesCoUE, ярлык рабочего стола не менялся. Старые адреса в корне обслуживают скрытые ссылки совместимости. Старый Unity — Archive/Unity, Studio v7 — Archive/Studio-v7, инструкции — Archive/Guides, desktop-fly — References/desktop-fly/Source. README в E:/Game объясняет структуру; при переносе на другой диск проверить абсолютные targets ссылок.

**11 сентября — [Windows share P1](../Changes/WINDOWS_SHARE_P1_2026-09-11.md).** Файл для друга: E:/Game/CalmyPiratesCo-Windows-2026-09-11.zip. Распаковать целиком → CalmyPiratesCo.exe; Play-Ship.cmd открывает корабль без обучения. При отсутствии Visual C++ — вложенный Engine/Extras/Redist/en-us/vc_redist.x64.exe. Unreal Editor не нужен. Кандидат Builds/Share-Windows-20260911-172821/Windows, Windows-Current и существующий ярлык не заменялись. Новый run_packaged_share_review.ps1 принимает -PackagedExe и проверяет extracted ZIP с отдельными профилями; missing runtime assets являются ошибкой. Подробные ограничения/хеши/логи в отчёте.

**11 сентября — [Settings / performance S1](../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md).** Старый ярлык Calmy Pirates Co. и start_game.ps1 запускают текущую editor-game; принудительные -windowed/-ResX/-ResY/t.MaxFPS 60 убраны, чтобы сохранялись настройки пользователя. Меню: Esc → Настройки. Данные: Saved/Config/WindowsEditor/GameUserSettings.ini. Новые run_settings_review.ps1 и run_network_smoke.ps1 -Suite Settings используют отдельные тестовые ini. run_performance_review.ps1 требует закрыть другие Unreal-процессы и восстанавливает исходный ini после benchmark. Package не обновлён; UE 5.8.2 и локальный loopback прежние.

**[Перенос на другой ПК](../Handover/TRANSFER.md)** · [HANDOVER](../../HANDOVER.md) · [Скрипты по назначению](../../Scripts/README.md).

Проверенная среда на исходном ПК: UE **5.8.2** в E:/Games/UE_5.8; Blender **5.2.1 LTS** в E:/Tools/blender-5.2.1-windows-x64/blender.exe; VS Community 2026 / MSVC 14.50 / SDK 10.0.22621.0 по последнему build log. [Компоненты VS](../../.vsconfig). EngineAssociation — 5.8; автоматическая смена движка не разрешена.

## Предпочтительный переносимый launcher

```powershell
# Из корня проекта. На другом ПК заменить пути:
./Scripts/Project.ps1 -Action Check -EngineRoot 'E:/Games/UE_5.8' -BlenderExe 'E:/Tools/blender-5.2.1-windows-x64/blender.exe'
./Scripts/Project.ps1 -Action Build -EngineRoot 'E:/Games/UE_5.8'
./Scripts/Project.ps1 -Action Automation -EngineRoot 'E:/Games/UE_5.8'
./Scripts/Project.ps1 -Action Play -EngineRoot 'E:/Games/UE_5.8' -StartPaused
./Scripts/Project.ps1 -Action Models -BlenderExe 'E:/Tools/blender-5.2.1-windows-x64/blender.exe'
```

Play по умолчанию **WaterLab: обучение → корабль**. Пройти 4 м, поднять деревянный ящик E и поставить в зелёную зону G; после приёмки через 8 секунд вся команда переносится в рубку. Enter продолжает игру из терминала. `-MapName OceanFreighterR7` пропускает урок. Для двойного клика на исходном ПК: [Play-Calmy.cmd](../../Scripts/Play-Calmy.cmd). Models открывает **CrewR8.blend**, только candidate; не импортирует его. `-DryRun` показывает команды; `Check` проверяет пути/версию, не gameplay. Scripts требуют PowerShell 7+. [Tutorial T1](../Changes/TUTORIAL_SHIP_T1_2026-09-10.md).

Перед Build сохранить и штатно закрыть свой редактор; не останавливать процессы пользователя вслепую. Это Development Editor, не fresh packaged build. Не запускать setup/import/publish/create скрипты заодно со сборкой.

## Игровые карты и проверки

Runtime герои **CrewR6B**, не R8. BridgeMenu entry/resume исправлен в R2 и повторно проверен после Tutorial T1 travel; это не законченный рейс или проверка другого устройства. Ship/Door fixtures намеренно стартуют на weather deck. Обычная WaterLab теперь туториал; для прежней свободной лаборатории добавить движку `-CalmyLab`. Существующие test-probe флаги также отключают tutorial, кроме CalmyTutorialProbe.

Старые `start_game.ps1` / `start_dev.ps1` / `Open-OceanFreighter.ps1` сохранены с локальными путями. Для сети: `run_network_smoke.ps1 -Suite Character -Editor <полный путь UnrealEditor.exe> -Port 17777`, аналогично Helper/Ship/Water/Pause. Loopback, уникальные порты, без firewall changes. GPU runners местами hardcode engine path, проверять перед запуском.

[VALIDATION](VALIDATION.md) определяет границы результата. Старые Saved логи не означают проверку свежей DLL.

## Blender / MCP

Авторинг корабля — [master](../../SourceAssets/Generated/OceanFreighter/CalmyOceanFreighter.blend), [walk-инструкция](../../SourceAssets/Generated/OceanFreighter/WALKTHROUGH_RU.md). Последний герой — [каталог](../Handover/MODELS.md). Не переоткрывать поверх несохранённой работы, не запускать генератор поверх новой версии.

[Project MCP config](../../.codex/config.toml): Unreal 127.0.0.1:8000/mcp, Blender uvx blender-mcp==1.9.0 / 127.0.0.1:9876 / telemetry disabled. Путь uvx привязан к Valerii: на новом устройстве изменить локально. В background Blender MCP команды не обслуживает; использовать bpy/CLI. Нет гарантии живого MCP только по config; в этом аудите серверы не запускались.

Дополнение 2026-09-09: [setup проверен](../Changes/TOOLING_SETUP_2026-09-09.md). Rigify включён с preferences backup и тестом генерации. `./Scripts/start_dev.ps1 -ShowWindows` запускает редакторы/MCP; без флага новые служебные окна скрыты. `./Scripts/test_unreal_mcp.ps1` проверяет initialize/discovery. Оба listener подтвердили 127.0.0.1, Blender addon compatible/telemetry false. Это живая проверка этой сессии, не обещание работающего сервера после закрытия приложений. Нативный Unreal tool в текущем клиенте не подтверждён, HTTP MCP доступен.

Проектный calmy-dream-loop сохранён как [snapshot для переноса](../Handover/Tooling/calmy-dream-loop/SKILL.md), не установлен автоматически на другие устройства. Aura/MetaHuman/AccuRIG/Cascadeur/Meshy не подтверждены как установленный pipeline. Учетные записи/ключи переносить отдельно, не через публичные документы. Mixamo не desktop install: пользователь уже выполнил Adobe login, модели туда не отправлялись.

## Не закрытые риски

- Два R6 importer shutdown crash в SemanticSearch после save; игровые загрузки впоследствии прошли. CLI DisablePlugins не исправил shutdown; R8 import ещё не запускался.
- Windows-Current от 4 сентября. `package_windows.ps1` build/cook автоматически продвигает пакет и удаляет previous; нужен отдельный candidate → smoke → promote, не выдавать PACKAGE_CURRENT_PASS за игру.
- Git не имеет коммитов/remote/tracked файлов; текущее сохранение — рабочие файлы + локальные Evidence/backups. Off-device backup ещё нужен.
- Исторические одноразовые `publish_*`, `freeze_*`, `fix_*`, `import_*` не являются bootstrap. Повторный отказ existing directory не обходить.
- DX12/SM6, Lumen и VSM настроены в Config; драйвер/производительность другого устройства не проверены. RTX не переключался.

Предыдущее описание Operations сохранено в Docs/Archive/Before-Handover-20260909.zip; данные предыдущих установок/тестов — в Docs/Changes и History.
