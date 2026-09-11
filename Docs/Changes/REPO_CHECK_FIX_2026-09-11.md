# Repo check / doc fix — 2026-09-11

Пользователь попросил проверить репозиторий, затем исправить найденное и сохранить результат в документации.

## Scope

- Read-only проверка корня `E:/Game`, Git, ссылок совместимости, ZIP, DLL, docs.
- Исправление устаревших формулировок и следа неудачного переноса F1.
- Без нового gameplay build, без Git commit/push, без удаления игровых данных.

## Findings (проверка)

| Проверка | Результат |
|---|---|
| Корень после F1 | `CalmyPiratesCoUE`, `Releases`, `Archive`, `References` + скрытые junction/symlink на старые пути; цели существуют |
| ZIP P1 | `Releases/Windows/2026-09-11/CalmyPiratesCo-Windows-2026-09-11.zip` ~447 MiB; SHA256 `1ac1dc405401b21ba077f4d7530fafeb82ef5445ed59c150b376c07a7fe59d63` совпал с `.sha256` |
| Editor DLL | `Binaries/Win64/UnrealEditor-CalmyPiratesCo.dll` = `dd75bb3120d62900cdb9e2b1c0d3cf0505dc0229a620b98f45b9c7f646e3cf92` (S1, как в INVENTORY) |
| `Builds/Windows-Current` | Существует, дата **2026-09-04**; не равен ZIP P1 |
| Git `CalmyPiratesCoUE` | Ветка `main`, **0 commits / 0 tracked / нет remote**; LFS filter настроен; `fsck` только notice unborn branch |
| Git `E:/Game` | Не репозиторий |
| desktop-fly | Скрытый `desktop-fly-reference` intact; `References/desktop-fly/Source` → junction; `master`=`origin/master` @ `32b0001`; `fsck` чистый |
| Unity Archive `.git` | Есть, но `dubious ownership` у другого Windows SID — без `safe.directory` не читается |
| Docs | `verify_project_docs.ps1` → `DOCS_PASS` 102 docs / 752 links / 0 missing |
| `.gitignore` | Игнорит Binaries/Saved/Builds/Intermediate/DDC; **не** игнорит `Evidence` (~854 MB) и `.codex` — важно перед первым коммитом |

## Fixed

1. **[README проекта](../../README.md)** — дата 2026-09-11; актуальный ZIP в Releases; DLL/S1/R6 вместо устаревшего «последняя итерация T1 / 9 сентября»; явно: Windows-Current ≠ share ZIP.
2. Удалён пустой служебный каталог `E:/Game/References/desktop-fly/.git` (обломок F1 без HEAD/config/objects; настоящий `.git` остаётся в `desktop-fly-reference`).
3. Обновлены [References/README](../../../References/README.md), HANDOVER, STATUS, CHANGELOG, VALIDATION, INDEX под эту проверку.

## Not done

- Git baseline / initial commit / remote.
- Off-device backup.
- Правка ownership Unity Archive `.git`.
- Расширение `.gitignore` для Evidence (только зафиксировано как риск).
- Новый automation / package.

## Next safe step

Пользователь явно разрешает: либо initial Git+LFS commit с исключением Evidence/Builds, либо off-device копия с проверкой хешей. Junction/symlink на другом диске нужно пересоздать — абсолютные пути `E:\Game\...`.
