# Fly inscriptions — общие надписи в стиле Dark Souls

Пользователь попросил, чтобы взаимодействие с роем собиралось в общее «осознание»: следы игроков как надписи Dark Souls, влияющие на поведение колонии. Нейронный граф FlyWire уже крутится; этот этап добавляет **общий банк закрытых меток**.

## Геймплей

- Колесо **E** — 8 секторов: крест = разговор (Greet / Calm / Recall / Shoo), диагонали = метки.
- Метки: `BE WARY`, `SAFE HERE`, `PRAISE THE FLIES`, `TRY TALKING`.
- Та же метка рядом (~1.8 м) **усиливает** существующую (echo / appraisal), не плодит дубликат.
- Банк до 24 меток на колонию; реплицируется (`VisibleInscriptions`); сохраняется в SaveGame **v2**.
- HUD рядом с роем: речь + до трёх ближайших `[Nx] LABEL`.
- Настроение колонии: wary → осторожность/Observe; welcome → Socialize; AskMemory считает marks/echoes.

## Честные границы

Это **общая память посетителей + utility**, не феноменальное сознание и не новый connectome. Свободный текст не принимается. MaleCNS из desktop-fly не подключался.

## Проверки

Финальная DLL: `1F2F4DAB9EE280F99E33B18559BD5CA9C1A71E28571588A87498751AF44341F7`.

| Проверка | Результат |
|---|---|
| Automation FlyMind + RadialSelection + FlySwarm | **PASS** — `Saved/Verification/20260912-150834-FlyInscription-Auto.log` (вкл. `SharedInscriptionsGather`) |
| FlyMind Host/Client/LateJoin + Restore | **PASS** — `Saved/Verification/20260912-150853-FlyMind` (`marks=1`, `replicated_marks=1`) |

Maps/mesh/package не менялись.
