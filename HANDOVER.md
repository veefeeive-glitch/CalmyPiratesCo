# Calmy Pirates Co. — продолжение из GitHub

Приватный проект игры: [veefeeive-glitch/CalmyPiratesCo](https://github.com/veefeeive-glitch/CalmyPiratesCo). Основная ветка main. Перед работой получить Git LFS payloads (`git lfs pull`), проверить актуальный HEAD и рабочее дерево.

Локальная копия для GitHub: `E:/Game/CalmyPiratesCo-GitHub`. Прежняя игровая папка `E:/Game/CalmyPiratesCoUE` и её история сохранены отдельно, без remote. Не подключать её старую main к этому репозиторию: она содержит исходную историю до удаления dev-токена. Авторизация CLI для игры отделена от остальных проектов; ключи и OAuth-данные в репозитории отсутствуют.

## Игра

UE 5.8.2. WaterLab → PortHub → контракт/рейс OceanFreighterR7 → PortHub. Runtime CrewR8, Cargo R3, Fly R2 в HOLD 1, движение/качка ShipMotion, heavy doors, детали S1–S3. Полная Chaos-физика корпуса, художественная приёмка и human fun gate остаются открытыми.

Игровой C++ и UE-ассеты не менялись при подготовке публикации. В рабочем конфиге и во всей публикуемой истории удалён токен Android File Server; плагин отключён, чтобы токен не создавался заново. 61/61 automation прошли 30 октября на имеющейся editor DLL. Чистая компиляция и свежий packaged/network/GPU проход ещё не выполнялись.

## Файлы и перенос

Source/Config/Content/Scripts и исходники авторинга находятся в Git; крупные бинарные файлы — в LFS. Игнорируемые Evidence/Saved/Builds, старые внешние Releases, локальные .env/.mcp и другие worktree не загружаются. Исторические отчёты могут ссылаться на отдельно хранящиеся доказательства.

[Запуск](Docs/Current/OPERATIONS.md) · [перенос](Docs/Handover/TRANSFER.md) · [проверки](Docs/Current/VALIDATION.md) · [статус](Docs/Current/STATUS.md) · [план](Docs/Roadmaps/GAME.md) · [лицензии](Docs/Current/ASSETS.md).

Данные FlyWire остаются под CC BY-NC 4.0, код симуляции имеет отдельную MIT-лицензию. Общая лицензия собственному проекту не назначалась. [Подготовка публикации](Docs/Changes/GITHUB_PREPARATION_2026-10-30.md).
