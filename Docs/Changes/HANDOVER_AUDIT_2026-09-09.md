# Аудит хендовера / порядка / переносимости — 2026-09-09

Запрос: собрать всю информацию и поддерживаемый handover для любого другого устройства, упорядочить репозиторий, учесть модели и незавершённое.

## Исправленные расхождения

README рекомендовал старые R3/R4 и надёжный запуск меню корабля; Operations называл R5. Исходники реально загружают CrewR6B. Найдены более поздние CrewR7/CrewR8 в SourceAssets и два dated отчёта, но Content/Characters/CrewR7/CrewR8 отсутствуют. Это авторинг, не runtime; принятое качество R8 и новые игровые тесты не выдуманы.

Read-only Blender 5.2.1 открыл R8: 20 FILE images, не packed, относительные пути //T_SM_*.png. В папке 20 GLB, 20 texture PNG, четыре renders, blend и manifest. Передавать папку целиком. Корабельный master сохранил SHA256 `8b4fe72d31e549649c0b36947a6cb5b906ebbd19b0955d5bd1f76d5f7da44f93`; машинный hash также в inventory.

## Организация без поломки ссылок

- Root HANDOVER — единая подробная точка продолжения; README сокращён до навигации.
- Docs/Handover: модели, перенос, журнал, шаблон записи, inventory, snapshot пользовательского calmy-dream-loop.
- SourceAssets/README и Scripts/README: каталоги по назначению/версии/риску. SourceAssets/Incoming/Characters — для файлов пользователя из другой программы.
- STATUS/OPERATIONS обновлены, остальные Current и индекс уточнены, game roadmap ссылается на актуальных кандидатов.
- AGENTS требует поддерживать handover/journal/status/roadmap и проверять inventory/links после работы. Это процесс для исполнителя, не фоновой сервис, не auto-sync/push.
- Рабочие ассеты, старые script paths, History и Evidence не перемещены: перенос сломал бы root вычисления scripts, Blender и UE dependencies. Корневые compatibility ссылки оставлены. Ничего материального не удалено.
- Предыдущие документы сохранены в Docs/Archive/Before-Handover-20260909.zip.

## Инструменты продолжения

Scripts/Project.ps1 — Check/Build/Automation/Play/Models, EngineRoot/BlenderExe или CALMY_UE_ROOT/CALMY_BLENDER_EXE, DryRun. Не меняет системные настройки, не импортирует models, не покупает/ставит программы; Play по умолчанию WaterLab. Build/Play реальным запуском в этом аудите не проверялись.

Scripts/Maintenance/Update-HandoverInventory.ps1 — относительные пути, file metadata, primary SHA256, автоматически обнаруженные CrewR-number кандидаты, runtime path из C++, флаги наличия их Content-папок, сравнение Source с frozen R6, имена scripts с machine paths. -Verify не пишет inventory, сверяет перенесённые primary файлы. Semantic done/not-done остаётся ручной ответственной записью; тесты не запускаются автоматически.

## Что проверено и чего не делали

- 28/28 automation и 15 сетевых role PASS — **из R6 от 8 сентября**, не повтор в этом аудите. Текущие DLL/maps совпали; Source diff с frozen R6 — 0. Frozen R6 manifest 289 entries без изменений.
- После создания документов doc checker: 88 документов, 545 local links, active_missing=0, historical_missing=0 (до добавления этого отчёта; финальное число проверяется командой).
- PowerShell parser двух новых scripts PASS. Check нашёл UE 5.8.2/Blender; build/play/automation/models команды прошли DryRun. Missing engine path отклонён.
- Relocation DryRun: копия launcher и .uproject в Saved/Handover Relocation … с пробелами корректно вычисляет новый root. Это не запуск на другом ПК и не пересборка игры.
- Первичный metadata scan: около 5581 файлов / 1,56 GiB в исходниках/Content/Docs/Evidence/SourceAssets/Scripts; исключены кэши, Saved, Builds, .git, установленные инструменты и соседний Unity. Точные текущие counts/hashes — inventory, не округлённая цифра для копирования.
- Git: 0 tracked files, нет коммитов, remote отсутствует. Никакого commit/push, настройки LFS/remote, переноса на устройство или независимого backup этим аудитом не выполнено.
- Gameplay/Config/Content/Blender masters/рендеры не менялись. Импорт R8, исправление BridgeMenu, shutdown-crash и новая упаковка остаются отдельными задачами.

Основной результат: [HANDOVER](../../HANDOVER.md), [журнал](../Handover/CHANGELOG.md), [перенос](../Handover/TRANSFER.md). Исторический реестр Docs/document-register.json остаётся реестром прежних 29 root документов для старого verifier, не каталогом всех новых файлов; полный живой перечень — inventory.
