> Снимок документа до актуализации 30 октября 2026. Статусы и команды ниже относятся к указанным в тексте этапам. Относительные ссылки адаптированы к месту хранения.

# Calmy Pirates Co.

Файлы всей рабочей папки: [E:/Game/README](../../../../README.md). Готовый ZIP для друга — [Releases/Windows/2026-09-11](../../../../Releases/Windows/2026-09-11/CalmyPiratesCo-Windows-2026-09-11.zip); старые Unity-версии — Archive. Текущий UE-проект остаётся здесь, внутренние рабочие пути сохранены. [Сортировка F1](../../Changes/FILE_ORGANIZATION_F1_2026-09-11.md) · [проверка репо 11 сентября](../../Changes/REPO_CHECK_FIX_2026-09-11.md).

Современный грузовой корабль, first-person/bodycam, low-poly и кооперативная физика для 2–4 игроков + solo с NPC. Unreal Engine **5.8.2**. Обновлено 2026-09-11.

## Начать здесь

**[HANDOVER — полное состояние и продолжение на другом устройстве](../../../HANDOVER.md)**

- [Модели и оба AI-концепта](../../Handover/MODELS.md): в игре **R8**; R6B Content сохранён.
- [Перенос проекта и запуск](../../Handover/TRANSFER.md): локальный Git+LFS baseline B1 есть; remote нет; Evidence копировать отдельно.
- [Журнал сделанного и оставшегося](../../Handover/CHANGELOG.md).
- [Все документы](../../INDEX.md), [скрипты по назначению](../../../Scripts/README.md), [исходники ассетов](../../../SourceAssets/README.md).

Обычный запуск — **обучающая платформа WaterLab → корабль OceanFreighterR7**. Пройти 4 м, взять деревянный ящик E, поставить в зелёной зоне G; через 8 секунд после приёмки вся команда оказывается в рубке. Enter — продолжить из терминала. [Play-Calmy.cmd](../../../Scripts/Play-Calmy.cmd) запускает текущую Editor-game версию на исходном ПК; `-MapName OceanFreighterR7` позволяет сразу на корабль. Судно статическое, без управления/качки.

**Отправить другу:** [ZIP P1](../../../../Releases/Windows/2026-09-11/CalmyPiratesCo-Windows-2026-09-11.zip) (SHA256 в соседнем `.sha256`). Старый `Builds/Windows-Current` от 4 сентября **не** заменяет этот ZIP.

```powershell
# Указать установленный на этом устройстве движок:
./Scripts/Project.ps1 -Action Play -EngineRoot 'E:/Games/UE_5.8' -StartPaused
```

Esc/Enter — пауза/продолжить; WASD/мышь — движение/обзор; E — груз/дверь/колесо мух, G — поставить, ЛКМ — удар или бросок, H рядом с Мишей — помощь, V/M — bodycam. [Полное управление](../../Current/GAMEPLAY.md).

Текущая editor DLL — [Settings / performance S1](../../Changes/SETTINGS_PERFORMANCE_S1_2026-09-11.md) поверх [Fly travel R6](../../Changes/FLY_TRAVEL_R6_2026-09-11.md) / Interaction R5 / Tutorial T1; meshes R6B. Automation PASS на своих ревизиях не означает finished game или художественную приёмку. [Проверки](../../Current/VALIDATION.md) · [Игра roadmap](../../Roadmaps/GAME.md) · [Корабль roadmap](../../Roadmaps/SHIP.md).

Старые корневые handover/verification имена — ссылки совместимости. История в Docs/History/Changes, frozen evidence не переписывается. После работы обновлять HANDOVER и журнал согласно [AGENTS](../../../AGENTS.md).
