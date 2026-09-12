# Fly presence — заметные занятия и речь

Пользователь попросил «довести до ума», чтобы мухи **ощущались присутствующими**: не только крутили SNN/cognition в фоне, а меняли занятие, говорили об этом и показывали это в HUD.

## Поведение

- **Utility:** у посетителя сильнее Observe/Socialize; Rest и energy drain умеренные (после фикса Travel); Withdraw при Alarm > 0.55.
- **Речь:** при смене занятия колония объявляет `DescribeActivity` (~10 с). Пауза путешествия (`SpeechSeconds > 11`) только для диалога колеса E / угрозы (~12 с) — объявления активности рой не «паркуют».
- **Движение:** Observe — standoff ~300 см медленнее; Socialize ~140 см; Rest pin на палубе; Withdraw быстрее; индивидуальный blend чуть живее.
- **HUD / прицел:** `SHIP RESIDENT | {ACTIVITY}`; подсказка E/LMB включает метку занятия. `CanConverse` 650 см (как perception).
- **Память Ask:** чуть богаче ответы по настроению занятия.

Честно: это **видимая агентность** (utility → travel → речь → HUD), не феноменальное сознание.

## Проверки

Финальная DLL: `FC7F68A289CD070C9B08959FB25DEB4377B8A4E54DE015E5391032E825DD0A06`.

| Проверка | Результат |
|---|---|
| FlyTravel Host/Client/LateJoin | **PASS** — `Saved/Verification/20260912-134545-FlyTravel` |
| FlyMind Host/Client/LateJoin + Restore | **PASS** — `Saved/Verification/20260912-134802-FlyMind` |
| Automation `CalmyPiratesCo.Fly*` | **7/7 PASS** — `Saved/Verification/20260912-134900-FlyPresence-Auto.log` (вкл. Observe selection в `AutonomousNeedsAndAttention`) |

Ранний Client FAIL той же ветки (`20260912-134229-FlyTravel`, `max_distance=914`): объявления занятия держали рой (`SpeechSeconds > 8`). Исправлено порогом > 11 + лёгкий откат Rest/energy.

## Границы

Maps/mesh/package не менялись. GPU visual review не снимался. Свободный диалог / STDP / новый connectome не добавлялись.
