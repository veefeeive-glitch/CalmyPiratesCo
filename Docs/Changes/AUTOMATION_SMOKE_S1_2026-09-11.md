# Automation smoke S1 — 2026-09-11

Фаза 3 очереди backup → ops → игра. Gameplay не менялся; package promote не запускался.

## Environment Check

- UE 5.8.2 at `E:/Games/UE_5.8` (Build.version OK)
- `UnrealEditor.exe` / `UnrealEditor-Cmd.exe` present
- Blender `E:/Tools/blender-5.2.1-windows-x64/blender.exe` present
- Editor DLL still S1: `dd75bb3120d62900cdb9e2b1c0d3cf0505dc0229a620b98f45b9c7f646e3cf92`
- `Project.ps1 -Action Check` requires PowerShell 7 (`pwsh`); **pwsh not installed** — Check выполнен вручную теми же проверками путей/версии

## Automation

```text
AUTOMATION_PASS 42/42
log: Saved/Logs/Automation-20260911-184018.log
exit: 0
```

Команда: `Scripts/run_automation.ps1` → `UnrealEditor-Cmd` NullRHI, tests `CalmyPiratesCo`.

## Not run

- Full Build (DLL уже S1; automation прошла без rebuild)
- `package_windows.ps1` / promote
- Host/Client/LateJoin network smoke (отдельные suites; не часть этого NullRHI automation)

## Next

Фаза 4: импорт R8 в `/Game/Characters/CrewR8` без затирания R6B, затем switch путей + build + automation.
