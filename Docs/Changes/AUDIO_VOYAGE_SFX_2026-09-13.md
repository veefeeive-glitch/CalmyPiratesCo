# Audio — Voyage / Atmosphere / Combat SFX — 2026-09-13

Оригинальные procedural SFX + ambient beds. Без сторонних записей.

## Сделано (пункты 1–4)

### 1 — Voyage loop / UI
- `FCalmyAudio` + `ECalmySoundCue`: select/confirm, PortAccept, WatchBegin/Outcome, ReturnPort.
- Bridge terminal, MissionBoard, VoyageDirector.

### 2 — Doors / cargo
- Door open/close (rep + host toggle, `LastHeardState`).
- Cargo grab/drop/throw/damage.

### 3 — Atmosphere
- `ACalmyAmbientBeds`: looping wind / hull / sea + leak alarm bed.
- Spawn on ship (`VoyageDirector`) and port (`GameMode` / MissionBoard).
- Storm level scales beds; `StormPulse` one-shot on rise.
- Leak phase-gated alarm (`LastHeardPhase`) + continuous bed via `SetLeakAlarm`.
- Fuel pulse at station; repair tick while repairing.

### 4 — Combat / flies
- Punch, FlySwat, FlyTalk (utterance + inscription).

## Ключевые пути
- `Source/.../Core/CalmyAudio.*`
- `Source/.../Gameplay/CalmyAmbientBeds.*`
- Wiring: BridgeTerminal, Door, Cargo, MissionBoard, VoyageDirector, GameMode, Leak, FuelStation, Character, FlySwarmMob

## Проверки
| Check | Result |
|---|---|
| Editor build | **PASS** |
| Automation | **55/55** |
| Door / BridgeMenu / Ship / Cargo / Carry / FlyTravel | **PASS** (`Saved/Verification/AUDIO-SMOKE-20260913.txt`) |
| Melee | **FAIL** `wheel_hover_failed` (P1 backlog; не регрессия звука) |

DLL SHA256: `F733684A91347E63018DA2CB37D27814B1F2C177CC4C89B8213C6E7765BE93DD`

## Не сделано
- Запечённые WAV Nightwatch / сторонние банки звуков.
- Human listen-pass / fun gate.
- Исправление Melee wheel hover (отдельный backlog).
