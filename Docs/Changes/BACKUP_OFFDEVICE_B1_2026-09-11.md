# Off-device backup B1 — 2026-09-11

Независимая копия на другой том. Оригинал `E:\Game` не удалялся и не перемещался.

## Destination

`D:\Backups\CalmyPiratesCo-2026-09-11\`

| Путь | Содержимое |
|---|---|
| `CalmyPiratesCoUE\` | Проект без `Intermediate`, `DerivedDataCache`, `.vs` |
| `Releases\` | Share ZIP P1 + SHA256 + README |
| `MANIFEST\` | Логи robocopy и verification JSON |

Источник логов: `Saved/Verification/Backup-OffDevice-B1-20260911-182808/` (на E: и копия в MANIFEST).

## Copy results

- Robocopy project exit 1 (успех, были копирования), **FAILED = 0**
- Robocopy Releases exit 1, **FAILED = 0**
- Catch-up: 5 файлов verification догнаны; FAILED = 0

## Verification

| Check | Result |
|---|---|
| ZIP SHA256 src+dst | `1ac1dc405401b21ba077f4d7530fafeb82ef5445ed59c150b376c07a7fe59d63` PASS |
| `git rev-parse HEAD` | `47efb302f969e3587b966803c4116d9ca214cab0` на src и dst PASS |
| `git status` | `## main` clean PASS |
| Evidence file count | 3793 = 3793 PASS |
| Key paths (uproject, HANDOVER, editor DLL, Build.cs, gitignore, gitattributes) | SHA256 match PASS |

### Documented deltas (не failure)

1. **6** файлов `Saved/Verification/G1-Preferences-*/Profile/Intermediate/ShaderAutogen/*.ush` — каталоги с именем `Intermediate` отсечены `/XD Intermediate` на любом уровне.
2. Два F1 reparse-probe symlink сначала пропущены `/XJ`, затем догнаны `Copy-Item` (attribute-link / zip-link-probe).

**Итог:** backup PASS с задокументированными deltas. Это не remote и не замена Git LFS clone.

## Not done

- Remote/push
- Удаление чего-либо с E:
- Полный byte-for-byte diff всех 16k файлов (выборка ключевых + counts + ZIP + git)
