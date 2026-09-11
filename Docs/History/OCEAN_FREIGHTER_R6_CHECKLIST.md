> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# R6 — iteration toward the user's 30% review

Starting point: R5, user assessment 22%. Percent complete is a human judgement,
not derived from passing collision tests. No production-complete claim.

## Execution order

- [x] T01: connect both Boat exits to exterior stair towers without crossing old hull rails.
- [x] T02: connect tower landings to forward weather deck; keep exposed-edge guards.
- [x] T03: connect aft weather passage; route around bollards and exhaust, not through them.
- [x] T04: build supported aft engine-gallery crosswalks on lower and tween decks.
- [x] T05: open a usable bow-thruster service bypass to the forepeak doorway.
- [x] Add distinct medical-room fittings and radio equipment, preserving room access.
- [x] Run actual UE CharacterMovement forward/reverse on base and new continuous routes.
- [x] Review close-up images of every changed connection and new room assembly.
- [x] Preserve R5 and promote only a verified candidate; provide walking review last.
- [ ] User walkthrough and assessment: does this iteration reach the requested quality?

Execution result: see `OCEAN_FREIGHTER_R6_HANDOVER.md`. The required-route gate is
260/260 directed physical walks; broad exploratory suite is 293/336 including
four successful controls. 43 exploratory failures are retained and documented.

## Acceptance boundaries

Reference: OCEAN_FREIGHTER_TRAVERSAL_AUDIT.md and its immutable R5 evidence.
Old exploratory routes through furniture or engine machinery remain in the audit;
they are not legitimate requirements to remove functional equipment. New service
routes must use continuous supported floor, ordinary walking, and no teleportation.
Base routes and usable-room routes must not regress. Physics negative controls
must still detect a blocking wall. Isolated Chaos tests are not multiplayer proof.

Water stays absent. Unreal gameplay-map asset replacement, multiplayer ship
collision, art completion of every compartment and user fun acceptance remain
separate unfinished work. No purchased/downloaded assets planned for this pass.
