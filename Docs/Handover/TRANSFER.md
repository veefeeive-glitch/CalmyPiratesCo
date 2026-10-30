# Перенос проекта — 30 октября 2026

Проверка восстановления на другом ПК ещё не выполнена. Публикуемая main содержит очищенную историю и изменения 30 октября. Старый локальный commit `d522c61` сохранён только в исходном репозитории; его хеш изменился после удаления токена. Репозиторий: [veefeeive-glitch/CalmyPiratesCo](https://github.com/veefeeive-glitch/CalmyPiratesCo). Git LFS нужен для UE-ассетов, моделей и текстур.

## Состав копии

| Содержимое | Назначение |
|---|---|
| .uproject, Source, Config, Content | Игровой проект |
| SourceAssets, Scripts, Docs, корневые инструкции и Git-правила | Авторинг, процедуры, лицензии, исходные данные и инвентарь |
| Evidence, Saved/Verification, нужные Saved/Logs | Отдельный архив доказательств; Git их игнорирует |
| Saved/Config, Saved/SaveGames | Личные настройки и сохранения; отдельно по необходимости |
| Builds, соседняя Releases | Исторические пакеты; не текущий релиз |
| .git | Локальная история, LFS, ветки и stash; обычный push main не сохраняет всё это |
| Binaries, Intermediate, DerivedDataCache, .vs, .sln/.slnx | Генерируемые данные; на другом ПК пересобрать |

Не переносить ключи и OAuth-данные через репозиторий. .mcp.json и .env-файлы локальные. Absolute junction/symlink из E:/Game вне проекта потребуют проверки после переноса. Соседний CalmyVoyageG3 является worktree основной истории, а не отдельным независимым backup.

## Проверка

Перед новым снимком на исходном ПК запустить `Scripts/Maintenance/Update-HandoverInventory.ps1`. На целевом ПК сначала выполнить `Scripts/Maintenance/Update-HandoverInventory.ps1 -Verify` и разобраться с различиями. Старый снимок 11 сентября был проверен до актуализации: 68 расхождений объяснялись сохранёнными изменениями проекта. Frozen evidence manifests не переписываются.

Для проверки frozen R6 нужен отдельный Evidence-архив. В обычном Git-клоне можно обновить метаданные командой Scripts/Maintenance/Update-HandoverInventory.ps1 -AllowMissingEvidence: инвентарь явно запишет not_available_in_git_clone вместо результата проверки Evidence. Режим -Verify проверяет primary-файлы независимо от наличия Evidence. Ссылки на Saved/Evidence и внешние старые пакеты в исторических отчётах могут быть недоступны в чистом клоне. Это не повод считать отсутствующие проверки пройденными.

## Инструменты и первый запуск

Установить UE **5.8.2**, перечисленные в .uproject Editor-плагины, компоненты из .vsconfig и PowerShell 7+. Blender 5.2.1 LTS нужен для авторинга. 20 внешних PNG CrewR8 должны находиться рядом с .blend. Наличие другой версии движка не является разрешением на миграцию.

```powershell
./Scripts/Project.ps1 -Action Check -EngineRoot 'D:/Unreal/UE_5.8' -BlenderExe 'D:/Blender/blender.exe'
./Scripts/Project.ps1 -Action Build -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Automation -EngineRoot 'D:/Unreal/UE_5.8'
./Scripts/Project.ps1 -Action Play -EngineRoot 'D:/Unreal/UE_5.8' -MapName PortHub -StartPaused
```

WaterLab остаётся начальной картой по умолчанию и ведёт через обучение в PortHub. Свежий packaged build и Host/Client проверяются отдельно. GPU/авторские runners местами имеют машинные пути: перед использованием смотреть параметры конкретного скрипта. [OPERATIONS](../Current/OPERATIONS.md) · [HANDOVER](../../HANDOVER.md).


Предыдущий текст со всеми этапами и деталями сохранён в [истории документа](../History/Before-Cleanup-2026-10-30/Docs/Handover/TRANSFER.md).
