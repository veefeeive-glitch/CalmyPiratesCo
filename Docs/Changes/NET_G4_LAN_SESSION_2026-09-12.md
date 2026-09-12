# Net G4 — LAN session UI + 4-peer smoke — 2026-09-12

## Goal

Push A04/A11/G4 toward product net: four peers, join UI, late-join, stress where automation exists. No Steam/internet invites without a concrete integration path.

## Implemented

- `FCalmySession` helpers: max crew 4, sanitize join address, listen host (`UWorld::Listen`), join travel, endpoint describe, crew count (controller + replicated `PlayerArray`), status line.
- Bridge terminal **CREW SESSION / LAN** page: Host LAN Watch, Join LAN (Slate address dialog), Copy Address, Back.
- `ACalmyGameMode::InitGame` sets `GameSession->MaxPlayers = 4`; `PreLogin` uses `CanAdmit`.
- HUD shows `FCalmySession::StatusLine`.
- Automation: `CalmyPiratesCo.Core.SessionLanHelpers` + Bridge menu hit-regions for 5 main rows.
- `run_network_smoke.ps1`: `Session` suite, `-Peers 4`, `-NetProfile None|Lag100|Lossy`.
- `UCalmySessionProbe`: Host/Client/Client2/LateJoin with peak-crew latch.

## Verified (this machine)

| Check | Result |
|---|---|
| Editor build | PASS |
| Automation | **52/52** `Saved/Logs/Automation-20260912-184352.log` |
| Session 4-peer | **NETWORK_SMOKE_PASS** `Saved/Verification/20260912-184225-Session` |
| Session 4-peer + Lag100 | **NETWORK_SMOKE_PASS** `Saved/Verification/20260912-184306-Session` |
| DLL SHA256 | `6DEB726144BD1ADE4695226DA1666C8F18ADCEC1472AD06BF1D76FA26579B307` |

## Steam / internet gap

No OnlineSubsystem Steam/EOS plugin, credentials, or invite API are present in this project. Loopback/LAN listen + IP:port join is the supported path. Firewall rules were not changed. Do not call this internet multiplayer.

## Not done / boundaries

- Lossy (`PktLoss`) profile not re-run in this pass (Lag100 was).
- Host migration / dedicated server not in scope.
- Human fun gate not claimed.
- Concurrent G3 voyage WIP that failed to compile was parked under `Saved/PendingOtherStreams/*.pending` so Net G4 could build; restore there for voyage work.
- Ship ocean motion / character art / voyage narrative GameMode were not edited beyond session admission hooks already on GameMode.

## How to use

1. Solo on ship → Esc → **CREW SESSION** → **HOST LAN WATCH** → Copy Address.
2. Friend on same LAN: Esc → CREW SESSION → Join → paste `IP:17777`.
3. Scripts: `Scripts/start_game.ps1 -Mode Host` / `-Mode Client -JoinAddress host:port`.
4. Smoke: `Scripts/run_network_smoke.ps1 -Suite Session -Peers 4` (+ `-NetProfile Lag100`).
