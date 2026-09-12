# FlyWire parity A — desktop-fly на 668 нейронах

Пользователь выбрал вариант **A**: максимально выжать уже встроенный FlyWire-граф (не MaleCNS).

## Что добавлено (как в desktop-fly `sim.js`)

- **SensoryGate / ActivityScale** — Rest ≈ sleep (слабее loom/air и baseline); Withdraw чуть выше arousal.
- **GaitDrive + GaitPhase** — ритмический ввод в Ascending от скорости колонии (sin по фазе, per-fly offset).
- **AirPuff** — JO sensory: метки wary + startle посетителя; умножается на SensoryGate.
- **Burst noise** — редкие 400 мс окна `pNoise×6` на awake-шуме.
- **AscendBias** — welcome-метки как постоянная добавка к ascending.

Граф по-прежнему **668 / 18 968**. `SourceLIFParity` сохранён (gate=1, scale=1, без gait/noise).

## Проверки

Финальная DLL: `B6AA9C6DE645B992E2721458B2C26B3E8F9B657C5E8EF711E66413478FB68806`.

| Проверка | Результат |
|---|---|
| FlySwarm + FlyMind automation | **PASS** (вкл. `DesktopFlyGatesAndGait`, `SourceLIFParity`) |
| FlyMind Host/Client/LateJoin + Restore | **PASS** |

## Границы

MaleCNS / ноги / brain-window 23k — не этот этап. Это паритет сенсомоторного контура на уже экспортированном графе, не «полное сознание».
