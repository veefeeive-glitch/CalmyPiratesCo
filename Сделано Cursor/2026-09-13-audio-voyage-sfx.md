# 2026-09-13 — Звуки (пункты 1–4)

## Что сделано
- Procedural SFX helper `FCalmyAudio` + ambient beds (wind/hull/sea/alarm).
- Voyage/UI: меню, доска, begin/outcome watch, return to port.
- Doors/cargo + storm/leak/fuel/repair.
- Punch, fly swat/talk.
- Docs: `Docs/Changes/AUDIO_VOYAGE_SFX_2026-09-13.md`.

## Проверки
- Automation **55/55**
- Smoke Door/BridgeMenu/Ship/Cargo/Carry/FlyTravel **PASS**
- Melee FAIL (wheel_hover — известный P1)
- DLL `F733684A…`

## Не сделано
- WAV banks / human audio acceptance / Melee P1 fix.
