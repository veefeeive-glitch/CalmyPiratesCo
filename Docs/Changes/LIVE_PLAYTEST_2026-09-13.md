# Live playtest — Solo ship watch — 2026-09-13

Ручной проход агентом: окно Editor-game + `play_harness.py` (SendInput + PrintWindow).

## Сделано в сессии

- Запуск Solo `/Game/Maps/OceanFreighterR7` 1280×720.
- Begin Watch с CREW TERMINAL (успешный кадр: `Saved/Verification/Playtest-Manual-20260913/run2/02-after-begin.png` ранее; повтор `run5b/02-gameplay.png` с `WATCH STARTED`).
- Осмотр рубки и дверей вблизи (`run5b/04-forward.png`, `run5b/09-final.png`).

## Находки

1. **Меню терминала чувствительно к hover мыши:** Enter с курсором на другой строке → Settings / Replay Training (ServerTravel WaterLab) / Leave session. Нужен park курсора или disable hover-select while using keyboard.
2. **Esc** открывает паузу/терминал — прерывает исследование.
3. **Двери V1:** полотно уже не синий куб; вживую у пола остаётся щель, проём рубки в кадре часто без читаемого закрытого листа (темнота Nightwatch).
4. **Маршрут трюм → green zone** в этой сессии не завершён (навигация + меню-ловушки).

## Не баг геймплея вахты

`TryBeginWatch` / HUD `WATCH STARTED · deliver the manifest to the green zone` — работают при чистом Begin Watch.

## Evidence

`Saved/Verification/Playtest-Manual-20260913/` (`play_harness.py`, `run2`, `run5`, `run5b`, `game*.log`).
