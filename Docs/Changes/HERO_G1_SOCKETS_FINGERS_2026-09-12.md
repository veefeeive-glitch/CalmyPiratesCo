# Hero G1 — mesh handle sockets + finger contact scaffold — 2026-09-12

Узкий срез G1 поверх [Grip R2a](GRIP_R2A_HANDLES_2026-09-11.md) и [Crew R8 runtime](CREW_R8_RUNTIME_2026-09-11.md). **Не** закрывает continuous skin, articulated fingers, human art acceptance или fun gate.

## Изменения (code + existing assets)

1. **`FCalmyCargoGrip`** (`Public/Core/CargoGrip.h`): имена сокетов `Grip_Near_L/R`, `Grip_Far_L/R`; `ResolveWorld` предпочитает socket на `CargoVisual`, иначе прежний `HandleLocal`.
2. **`ACalmyCargo::GetGripPoint`**: вызывает `ResolveWorld`.
3. **`FCalmyCrewHand`** (`Public/Core/CrewHand.h`): `Wrist`/`Palm`/`Tip` locals и sockets; cosmetic `CarryFingerCurl` на нижней руке при хвате.
4. **`UCalmyCrewPresentation::UpdateCarryArms`**: tip/palm contact через `ContactWorld`; curl при грузе.
5. **Editor stamp scripts** (существующие меши, без нового арта):
   - `Scripts/stamp_cargo_handle_sockets.py` → `/Game/Cargo/R3/SM_Cargo_*`
   - `Scripts/stamp_crew_hand_sockets.py` → `/Game/Characters/CrewR8/SM_*_Forearm*`
   - `Scripts/Stamp-HeroG1Sockets.ps1`
6. Automation: расширен `Cargo.HandleGrips`; добавлен `Character.HandSockets`.

## Continuous skin — ассеты отсутствуют

В Content только 10 rigid Static Mesh частей на героя. **Нет** skeletal mesh со skin weights. Code path не подменяет rig на выдуманный SKM.

Точный список нужд (не сделано):

| Нужда | Путь / имя (предложение) | Зачем |
|---|---|---|
| Skinned body (Deckhand) | `SourceAssets/Generated/CrewR8/SK_Deckhand_R8.blend` → `/Game/Characters/CrewR8/SK_Deckhand_R8` | непрерывная одежда вместо стыков труб |
| Skinned body (Misha) | `SK_Misha_R8` аналогично | то же для NPC |
| Skin weights | плечи/локти/бедра без разрывов | G1 continuous surface |
| Articulated fingers (optional) | отдельные кости или `SM_*_Hand*` + constraints | обхват ручек, не baked grooves |
| Materials | base color + optional AO channel | читаемость в сумраке (MODELS) |

Пока R8 forearm уже содержит **baked glove + finger grooves** в Static Mesh; это не skinned пальцы.

## Проверки

| Проверка | Результат |
|---|---|
| Build Editor Development | PASS (UBT; out-of-scope dirty fly/voyage parked during build) |
| Stamp cargo/hand sockets to Content | **PARTIAL** — scripts run and add sockets in-session; `save_loaded_asset` returns false / uasset mtimes unchanged; cold reload shows sockets MISSING. Scripts kept for interactive Editor re-try. |
| Automation | **2/2** Success — `HandleGrips` + `HandSockets` (`Saved/Logs/Automation-HeroG1-20260912.log`) |
| DLL SHA256 | `FFA664E99254FCAAF706BC06F4C95A9182BB83A86DF5DDEE60B01451B72D23FD` |
| Human fun / art acceptance | **не заявлено** |

Runtime grip points remain reachable `HandleLocal` fallback until Content sockets persist. `ResolveWorld` will prefer sockets automatically once stamped assets save.

Ship/ocean/voyage/net session **не** редактировались этим срезом.

## Ограничения

- Socket locals совпадают с reachable `HandleLocal` (Grip R2a), не с абсолютными вершинами R3 grab rail (Y±47,Z30) — иначе IK снова уходит за reach (~12 см diagnostic).
- Curl — ориентация мотора предплечья, не деформации пальцев.
- Continuous skin / отдельный hand mesh / package / второй живой игрок — следующие срезы.
