# Phased ops — remote SKIP + ops hardening — 2026-09-11

После [Off-device backup B1](BACKUP_OFFDEVICE_B1_2026-09-11.md).

## Phase 1 — Remote SKIP

URL не передан. Remote не добавлялся, push не выполнялся. Локальный `main` и backup на D: без изменений политики публикации.

Включить позже: URL → `git remote add` → `git lfs push --all` → `git push -u` (без force).

## Phase 2 — Ops hardening PASS

| Шаг | Результат |
|---|---|
| `.gitattributes` `*.jpg` / `*.jpeg` → LFS | Добавлено |
| Renormalize существующих JPG | 30 файлов в LFS (ambientCG + PolyHaven barrel textures уже были в tree) |
| `git config --global --add safe.directory E:/Game/Archive/Unity/CalmyPiratesCo` | Добавлено; `git status`/`log` читаются (ветка `autopilot/20260811-041122`, warning про old-style insteadOf остаётся) |
| Supermemory | CLI отсутствует; MCP `mcp_auth` показал success, но `supermemory_add` всё ещё «Not authenticated» — **SKIP до ручного `cursor-supermemory login`** |

Проверки: `git lfs track` показывает jpg/jpeg; Unity repo больше не `dubious ownership`.

## Not done here

- Automation smoke (фаза 3)
- R8 / gameplay (фаза 4+)
- Remote
