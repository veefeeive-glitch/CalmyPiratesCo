# Crew R8 runtime switch — recovery motor — 2026-09-11

После [импорта R8](CREW_R8_IMPORT_2026-09-11.md): pivots в манифесте совпадают с `CrewRig::Bind`; Character зависал на `CanFinishRecovery` из‑за NDOP контактов более крупных мешей.

## Change (careful)

В `CalmyCrewPresentation.cpp`:

1. Runtime mesh path → `/Game/Characters/CrewR8` (и тест `CrewLowPolyTests.cpp`).
2. При `PrepareRecovery`: `WorldStatic` → Overlap на частях + `WakeRigidBody` (мотор не борется с полом).
3. Чуть сильнее recovery force/torque.
4. Soft finish после 2 с при валидной стойке (`dist<28`, `deg<40`); прежние пороги 5 см / 12° сохранены первыми.
5. После выхода из physical fall — `ResetBodies()` для snap к bind.
6. `FindRecoveryPosition`: порог скорости 180 после `RecoveryAge>2`.

R6B Content не удалялся.

## Verification

| Check | Result |
|---|---|
| Build | Succeeded |
| Automation | **42/42** `Saved/Logs/Automation-20260911-194950.log` |
| Character Host/Client/LateJoin | **NETWORK_SMOKE_PASS** `Saved/Verification/20260911-195105-Character` |
| Helper | **NETWORK_SMOKE_PASS** |
| DLL SHA256 | `fbe951fa8ba660b72537bf40e11b746b468bdd78c27419b69473ff75044cf807` |
| Host PASS detail | `root_drop_cm=79.4` `max_joint_gap_cm=0.09` powered_recovery=1 |

Ship/Water на этой DLL отдельно не гонялись в этом прогоне (ранее PASS на R8 meshes до recovery fix).

## Not done

- Grips / fingers / skin
- Water floating proxy
- New Windows package
- Human art acceptance of R8

## Next

Обычная игра / GPU взгляд; затем grips как отдельный этап. При регрессии recovery — откат путей на R6B + эти моторные правки можно оставить (помогают и R6B).
