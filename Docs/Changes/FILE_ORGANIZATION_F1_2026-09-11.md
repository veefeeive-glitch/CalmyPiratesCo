# File organization F1 — порядок в E:/Game

Пользователь попросил упорядочить всю `E:/Game`. В обычном просмотре корня теперь четыре папки и [README](../../../README.md): текущий UE-проект, `Releases`, `Archive`, `References`.

| Категория | Расположение |
|---|---|
| Текущий проект | `E:/Game/CalmyPiratesCoUE` — не перемещён |
| ZIP для друга + SHA256 | [Releases/Windows/2026-09-11](../../../Releases/Windows/2026-09-11) |
| Прежний Unity-проект | `E:/Game/Archive/Unity/CalmyPiratesCo` |
| Studio v7 + прежний ZIP | `E:/Game/Archive/Studio-v7` |
| Старые инструкции | `E:/Game/Archive/Guides` |
| desktop-fly | `E:/Game/References/desktop-fly/Source` — junction к сохранённому исходному каталогу |

Восемь объектов перенесены, для прежних адресов оставлены скрытые junction/symlink. Это сохраняет старый ZIP URL/путь, launcher-ссылки, Unity-пути и исторические ссылки без копирования данных. Каталог desktop-fly — девятый объект плана — физически остался на месте: Windows отказала в переименовании. PowerShell начал частичный перенос его файлов; все перемещённые файлы возвращены, состав корня восстановлен, Git `fsck --no-reflogs` прошёл. Вместо переноса добавлен junction `References/desktop-fly/Source`, старый исходный каталог скрыт в корне. В контейнере References оставалась пустая скрытая служебная `.git`-структура от отказавшей операции; это не второй репозиторий. Позже удалена при [repo check / doc fix](REPO_CHECK_FIX_2026-09-11.md).

Существующие папки `Content`, `SourceAssets`, `Source`, `Config`, `Scripts`, `Builds`, `Evidence`, `Saved` внутри UE не переставлялись. Их пути используются движком, зависимостями Blender, скриптами и frozen evidence. Старые корневые handover-файлы проекта уже являются индексированными ссылками совместимости и сохранены. Кеши, проекты, исходники и прежние сборки не удалялись.

Добавлены поясняющие README для корня, Releases, Archive и References. Журнал переносов с исходными NTFS file IDs, SHA256 файлов и состоянием каждого шага: [moves.json](../../Saved/Verification/File-Organization-20260911-180204/moves.json).

Проверки: до перестановки `Update-HandoverInventory -Verify` подтвердил 1379 файлов проекта. У физически перенесённых объектов сохранены идентификаторы NTFS; файлы сверены по SHA256 через новый и прежний пути, состав корней старых проектов совпал. ZIP остаётся побайтно тем же Windows Share P1: SHA256 `1ac1dc405401b21ba077f4d7530fafeb82ef5445ed59c150b376c07a7fe59d63`. Финальная проверка ссылок/инвентаря и dry-run запуска фиксируется рядом с moves.json. Это файловая организация, не новая сборка или gameplay PASS.

Версии S1/R6/P1, модели/карты, игровой EXE и desktop-ярлык прежние. Следующий шаг — пользоваться Releases для отправки игры, а README/HANDOVER для поиска материалов. При переносе на другой диск проверить абсолютные targets ссылок; простая сортировка не создаёт off-device backup или Git baseline.
