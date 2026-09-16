# Full project audit — 2026-09-16

Срез `E:\Game\CalmyPiratesCoUE` после волны 13–16 сентября (Voyage G4, audio, heavy doors, guardrails/details S1–S3, playability rework). Не заменяет human fun gate и не перезапускает полный smoke в этой сессии — опирается на свежие артефакты + статическую сверку.

## Вердикт

**Играбельный editor-цикл PortHub → вахта R7 → задачи есть и закрыт машинными/игровыми пробами.** Главный риск — **незакоммиченная гора работы без remote**: одна копия на диске. Packaged share (12 сен) и Windows-Current (4 сен) **не** отражают текущий корабль.

## Снимок фактов

| Показатель | Значение |
|---|---|
| Git HEAD | `92df982` (`main`), **remote = 0** |
| Working tree | ~214 porcelain: 65 M, 117 ??, 32 D (`__pycache__`) |
| Stash | 6 устаревших park-stash (с 12 сен) |
| Editor DLL | `DD728A0F…` · 2026-09-16 05:04 |
| Automation (последний лог) | **61/61** `Saved/Logs/Automation-20260916-062107.log` |
| Docs | **DOCS_PASS** 141 docs / 933 links / 0 missing |
| Source module | ~166 файлов · ~18.4k LOC |
| Packaged | Share `Builds/Share-Windows-20260912-*`; ZIP `CalmyPiratesCo-Windows-2026-09-11.zip`; Windows-Current ≈ 4 сен |

Заявленные playability (из [SHIP_PLAYABILITY_REWORK](SHIP_PLAYABILITY_REWORK_2026-09-15.md), не пересчитаны сегодня): doors **61/61**, routes **291/332** / 0 регрессий, fall clusters **15**, character playthrough **PASS**, smokes **21/21**, HOLD1 FPS после densify **~84**.

Предыдущий полный аудит: [SUPER_AUDIT_2026-09-13](SUPER_AUDIT_2026-09-13.md).

## Что выросло с 12–16 сентября

1. **Voyage loop G4** — PortHub, MissionBoard, контракты, RETURN TO PORT.
2. **Audio** — procedural SFX + ambient beds; затем позиционный/окклюзия (14 сен).
3. **Heavy doors** — Amnesia-style drag; DoorMotion.
4. **Breakdown arrow** — HUD compass к течи/топливу.
5. **Ship densify** — SM_R7_80 service / 81 guardrails / 82 details + Poly Haven CC0 props.
6. **Playability stack** — Playability/Playthrough/PhotoTour probes; перенос клапана/топлива/меток на осмысленные места; леер с палубных кромок.
7. **Fixes** — rider carry, Settings/Melee isolation, door Nanite project materials, Tutorial→PortHub.

## Системы (кратко)

| Область | Состояние |
|---|---|
| Tutorial → PortHub → R7 → port | Работает в editor; accept/return **host/solo** (`IsLocalController`) |
| LAN 4 peers | Есть UI; Steam/OSS нет |
| Doors 61 | Visual + heavy motion; audit clear |
| Cargo / leak / fuel / storm | Якоря задач автотестом; playthrough PASS |
| Flies | Travel PASS после rider fix; pose replication тяжёлая |
| HUD / bridge screens | Минимальный HUD + displays; **нет dedicated automation на BridgeDisplays** |
| Crew | Runtime **CrewR8** в коде; GAMEPLAY/ASSETS всё ещё пишут R6B (**doc drift**) |
| Audio | Procedural; GC на каждый one-shot — известный tradeoff |

## Находки (свежий срез)

| # | Серьёзность | Находка | Рекомендация |
|---|---|---|---|
| 1 | **Критическая** | Нет remote; PortHub + SM_R7_80/81/82 + ThirdParty + куча Source/Docs **не в git** / огромный dirty tree | Коммит (или внешний backup) **сегодня**; remote/off-device |
| 2 | Высокая | Packaged build отстаёт на 4+ дня от editor DLL | Новый share cook только после коммита; smoke ZIP |
| 3 | Высокая (дизайн) | Voyage/MissionBoard accept & RETURN требуют local host | Явно задокументировать или ServerRPC для клиентов |
| 4 | Средняя | Doc drift: GAMEPLAY/ASSETS «R6B runtime», код CrewR8 | Починить Current docs |
| 5 | Средняя | 15 кластеров обрывов остаются; закрытие «продлением» ломало маршруты (Round H откат) | Оставить или точечные стыки, не массовый extend |
| 6 | Средняя | God-classes GameMode (~989), Fly (~951), Character (~866) | Рефактор не блокер; риск регрессий при правках |
| 7 | Средняя | Engine material incident 13 сен (скрипт Nanite) — файл движка пересохранён | Не гонять fix-скрипты в Engine/; verify UE install |
| 8 | Низкая | 6 stash устарели | Удалить после подтверждения |
| 9 | Низкая | BridgeDisplays / AmbientBeds без dedicated tests | Добавить probe при следующей UI-волне |
| 10 | Инфо | HOLD1 ~84 FPS после densify (было ~92 S1) | Приемлемо; не ship-wide FPS gate |
| 11 | Инфо | Human fun gate / Steam / Chaos R7 / R7-ART reimport | По-прежнему открыты |
| 12 | Инфо | `Сделано Cursor/` + `.cursor/` untracked | Ок по правилу проекта |

## Чистые зоны (подтверждено ранее + docs)

- `SetActorLabel` / editor API под `WITH_EDITOR` (game target собирается).
- DOCS_PASS 141/933.
- Automation 61/61 на DLL от 16 сен.
- TODO/FIXME в Source — 0 (по обзору модуля).
- Poly Haven CC0 + provenance manifest.

## Что аудит **не** перепроверял сегодня

- Полный повтор 21 network smoke.
- GPU фото-тур / playthrough заново.
- Packaged EXE smoke.
- Off-device backup свежести.
- Хаотический Chaos / fluid на всём R7.

## Рекомендуемый порядок дальше

1. **Сохранить** — git commit (+ LFS) или полный off-device backup; remote.
2. **Doc sync** — CrewR8 в GAMEPLAY/ASSETS/OPERATIONS.
3. **Human play** fun gate полного цикла со звуком.
4. **Packaged share** после 1.
5. По желанию: client voyage RPC, оставшиеся 15 fall clusters, BridgeDisplays test.

## Ключевые пути

- Отчёты волны: `Docs/Changes/SHIP_PLAYABILITY_REWORK_2026-09-15.md`, `SHIP_DETAILS_S3_…`, `HEAVY_DOORS_…`, `VOYAGE_LOOP_G4_…`, `SUPER_AUDIT_2026-09-13.md`
- Status: `Docs/Current/STATUS.md`, `HANDOVER.md`
- Canvas: workspace `canvases/calmy-full-audit-2026-09-16.canvas.tsx`
