# -*- coding: utf-8 -*-
from pathlib import Path

session = Path(r"E:\Game\CalmyPiratesCoUE\Source\CalmyPiratesCo\Private\Core\CalmySession.cpp")
t = session.read_text(encoding="utf-8")
if 'GameFramework/GameStateBase.h' not in t:
    t = t.replace(
        '#include "GameFramework/PlayerController.h"\n',
        '#include "GameFramework/PlayerController.h"\n#include "GameFramework/GameStateBase.h"\n',
    )
start = t.find("int32 FCalmySession::CountCrew")
end = t.find("FString FCalmySession::RoleLabel")
if start < 0 or end < 0:
    raise SystemExit("CountCrew markers missing")
replacement = '''int32 FCalmySession::CountCrew(const UWorld* World)
{
	if (!World) return 0;
	// GameState PlayerArray is replicated to clients; local PC iterators are not.
	if (const AGameStateBase* GS = World->GetGameState())
	{
		return GS->PlayerArray.Num();
	}
	int32 Count = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->IsValid()) ++Count;
	}
	return Count;
}

'''
t = t[:start] + replacement + t[end:]
session.write_text(t, encoding="utf-8", newline="\n")
print("session", session.stat().st_size)

header = Path(r"E:\Game\CalmyPiratesCoUE\Source\CalmyPiratesCo\Public\Testing\CalmySessionProbe.h")
ht = header.read_text(encoding="utf-8")
if "ReadyAt" not in ht:
    ht = ht.replace(
        "bool bStarted = false;",
        "bool bStarted = false;\n\tfloat ReadyAt = -1.f;",
    )
    header.write_text(ht, encoding="utf-8", newline="\n")
    print("header ReadyAt")

probe = Path(r"E:\Game\CalmyPiratesCoUE\Source\CalmyPiratesCo\Private\Testing\CalmySessionProbe.cpp")
pt = probe.read_text(encoding="utf-8")
needle = """\t\tif (Stage == 1 && Crew >= ExpectedPeers && Pawns >= ExpectedPeers)
\t\t{
\t\t\tFinish(true, FString::Printf(TEXT(\"four_slot_cap=%d crew=%d pawns=%d listen=1 late_join_ok=1\"),
\t\t\t\tFCalmySession::MaxCrew, Crew, Pawns));
\t\t}"""
# File has real quotes, not escaped:
needle = needle.replace('\\"', '"')
repl = """\t\tif (Stage == 1 && Crew >= ExpectedPeers && Pawns >= ExpectedPeers)
\t\t{
\t\t\tif (ReadyAt < 0.f) ReadyAt = Age;
\t\t\tif (Age - ReadyAt >= 2.f)
\t\t\t{
\t\t\t\tFinish(true, FString::Printf(TEXT("four_slot_cap=%d crew=%d pawns=%d listen=1 late_join_ok=1"),
\t\t\t\t\tFCalmySession::MaxCrew, Crew, Pawns));
\t\t\t}
\t\t}"""
if needle not in pt:
    raise SystemExit("host finish needle missing")
probe.write_text(pt.replace(needle, repl), encoding="utf-8", newline="\n")
print("probe", probe.stat().st_size)
