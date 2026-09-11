> Историческая запись, сохранённая при аудите документации 2026-09-06. Описывает состояние на дату исходного отчёта, а не сегодняшнюю готовность. Старые проценты, default-карты, версии тестов и команды не считать текущими инструкциями. [Актуальный статус](../Current/STATUS.md) · [Указатель](../INDEX.md). Пути внутри backticks даны от исходного корня проекта. Содержимое Evidence и пакеты не переписывались.

# Shift economy verification

Verified 2026-09-04 on Unreal Engine 5.8.2.

- Server-owned contract duration: 300 seconds.
- Successful payout: `$1200 + 4 * seconds remaining - 125 * rescues - 300 * damaged deliveries`, clamped to a `$250` successful minimum.
- Result rating: one to three stars; failed contracts pay zero.
- Restart accepts only the local host/solo controller, the expected round, and a completed or failed contract.
- Late join receives current time/result in the replicated shift snapshot; clients cannot submit time, payout, rating, or completion.

Evidence:

- `Saved/Logs/Automation-20260904-002119.log` — 13/13 project tests passed.
- `Saved/Verification/20260904-002141-Shift` — editor Host, Client, and LateJoin passed.
- `Saved/Logs/Package-20260904-002302.log` — Windows package succeeded.
- `Saved/Verification/20260904-002444-Shift` — packaged Host, Client, and LateJoin passed.
- The stable `Builds/Windows-Current` executable was launched on FreighterDeck and the countdown/economy HUD was visually verified.
