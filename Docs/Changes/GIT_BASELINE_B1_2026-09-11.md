# Git LFS baseline B1 — 2026-09-11

Пользователь разрешил локальный Git+LFS baseline после проверки репо. Remote и push не создавались.

## Scope

- Первый коммит в `CalmyPiratesCoUE` на ветке `main`.
- Git LFS для путей из существующего `.gitattributes` (uasset/umap/fbx/glb/gltf/blend/wav/png/tga).
- Без push, без remote, без off-device копии.

## Policy в `.gitignore`

Добавлено к прежнему UE-набору:

- `Evidence/` — frozen прогоны остаются на диске (~854 MB), не в Git; нужны отдельно при переносе/backup.
- `.codex/` — служебный кеш агента.

По-прежнему игнорируются: `Binaries/`, `Builds/`, `Saved/`, `Intermediate/`, `DerivedDataCache/`.

## Tracked

Исходники и рабочая документация: `.uproject`, `Source/`, `Content/`, `Config/`, `SourceAssets/`, `Docs/`, `Scripts/`, корневые handover/compat MD, `AGENTS`, `.gitattributes`, `.gitignore`, `.vsconfig`.

Не tracked: packaged Builds, editor Binaries/DLL, Evidence, Saved.

## Verification

- Commit: `7bb0048b431705a61b74406882677373d01a55d7` (`main`)
- Tracked files: 2880; LFS-managed paths: 1826
- `git status`: clean after baseline commit
- Remote: нет; push не выполнялся

Working tree after this docs commit records the baseline SHA above.

## Next

1. Off-device backup `Evidence/` + при необходимости `Builds/` отдельно от Git.
2. Remote только после явного разрешения пользователя.
3. При переносе на другой диск: junction/symlink в корне `E:/Game` пересоздать; клон Git не восстанавливает ZIP в Releases (он вне этого репо).
