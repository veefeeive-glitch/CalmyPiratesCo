# 2026-09-13 — Старт «настоящей» игры

Регрессия + baseline модели корабля → грузы в трюм HOLD 1 → длинный watch (Voyage G3b).

## Что сделано

### Фаза 0a — регрессия
- Build + Automation **54/54**
- Полная network-матрица: **14 PASS**, **4 FAIL** (P1 backlog)
  - FAIL: Tutorial (Host START timeout), Settings (preferences), Melee (`wheel_hover`), изначально FlyTravel
- Отчёт: `Docs/Changes/REGRESSION_SWEEP_2026-09-13.md`
- Evidence: `Saved/Verification/REGRESSION-SWEEP-20260913-005458.txt`

### Фаза 0b — модель корабля (скриншоты)
- Baseline **7 PNG**: `Saved/Verification/Ship-Visual-Baseline-20260913/`
- Каталог дефектов R7-ART-01/02, glare, fog-shadow — **без** опасного Blender-reimport
- Отчёт: `Docs/Changes/SHIP_MODEL_FIX_2026-09-13.md`

### Фаза 1 — грузы в трюм
- 4 ящика перенесены в **HOLD 1**: `(-1150…-550, -420, 320)`
- Delivery остаётся на палубе (green zone)
- Backup карты: `SourceAssets/Backups/Before-CargoHold1-20260913/`
- Скрипты: `Scripts/relocate_cargo_hold1.py`, `Scripts/verify_cargo_hold1.py`
- Verify: `CARGO_HOLD1_VERIFY_PASS n=4`

### Фаза 2 — Voyage G3b (~15–20 мин)
- Длительность вахты ~**1080 с** (~18 мин)
- Манифест: Standard + Bulky + Fragile + Unstable
- Шторм по расписанию, течь, топливо + станция у МО (удержание **F**)
- `ACalmyFuelStation`, правки `ACalmyVoyageDirector` / Shift curated manifest
- Отчёт: `Docs/Changes/VOYAGE_G3B_CARGO_HOLD_2026-09-13.md`

### P0 FlyTravel
- Жёсткий clamp облака роя при travel + unstick penetrating + gate `CanFlySegment(..., 2)`
- Итог: **FlyTravel Host/Client/LateJoin PASS** (spread Host ≤228)

## Проверки после изменений
| Проверка | Результат |
|---|---|
| Automation | **54/54** |
| Ship / Door / Cargo / Carry / BridgeMenu | **PASS** |
| FlyTravel | **PASS** |
| Melee | FAIL P1 (`wheel_hover`) — backlog |

DLL: `C53C5189A21F92174C93130D73434B39D0BFAF0329B3ADF25AE852188A9703E4`

## Обновлены доки
- `HANDOVER.md`, `Docs/Current/STATUS.md`, `Docs/Current/SHIP.md`, `Docs/Handover/CHANGELOG.md`

## Не сделано (следующее)
- Порт / доска миссий
- Blender-правка R7-ART-01/02
- P1: Tutorial / Settings / Melee
- Packaged promote
