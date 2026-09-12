# -*- coding: utf-8 -*-
from pathlib import Path

probe = Path(r"E:\Game\CalmyPiratesCoUE\Source\CalmyPiratesCo\Private\Testing\CalmySessionProbe.cpp")
text = probe.read_text(encoding="utf-8")
old = """\tif (Crew >= 1 && Pawns >= 1)
\t{
\t\tUE_LOG(LogTemp, Display, TEXT(\"CALMY_SESSION_TEST %s READY crew=%d pawns=%d\"), *Role, Crew, Pawns);
\t\tGLog->Flush();
\t\tconst bool Enough = !Late || (Crew >= ExpectedPeers && Pawns >= ExpectedPeers);
\t\tif (Enough || (!Late && Crew >= 2 && Pawns >= 2))
\t\t{
\t\t\tFinish(true, FString::Printf(TEXT(\"joined=1 crew=%d pawns=%d expected=%d late=%d\"),
\t\t\t\tCrew, Pawns, ExpectedPeers, Late ? 1 : 0));
\t\t}
\t}
}
"""
new = """\tif (Crew >= 1 && Pawns >= 1)
\t{
\t\tif (Stage == 0)
\t\t{
\t\t\tUE_LOG(LogTemp, Display, TEXT(\"CALMY_SESSION_TEST %s READY crew=%d pawns=%d\"), *Role, Crew, Pawns);
\t\t\tGLog->Flush();
\t\t\tStage = 1;
\t\t}
\t\t// Stay connected until the expected roster is visible so early clients
\t\t// do not tear down the listen session before Client2/LateJoin arrive.
\t\tif (Crew >= ExpectedPeers && Pawns >= ExpectedPeers)
\t\t{
\t\t\tFinish(true, FString::Printf(TEXT(\"joined=1 crew=%d pawns=%d expected=%d late=%d\"),
\t\t\t\tCrew, Pawns, ExpectedPeers, Late ? 1 : 0));
\t\t}
\t}
}
"""
if old not in text:
    raise SystemExit("probe block not found")
probe.write_text(text.replace(old, new), encoding="utf-8", newline="\n")
print("probe", probe.stat().st_size)

smoke = Path(r"E:\Game\CalmyPiratesCoUE\Scripts\run_network_smoke.ps1")
st = smoke.read_text(encoding="utf-8")
old_w = """        if ((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern $Pattern -Quiet)) { return }
        if ($processes | Where-Object { $_.HasExited }) { throw \"A probe exited before $Role / $Pattern. Evidence: $evidence\" }"""
new_w = """        if ((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern $Pattern -Quiet)) { return }
        # Allow peers that already PASS/exit while later joiners are still connecting.
        if (-not ($processes | Where-Object { -not $_.HasExited })) { throw \"All probes exited before $Role / $Pattern. Evidence: $evidence\" }
        foreach ($roleLog in Get-ChildItem -LiteralPath $evidence -Filter '*.log' -ErrorAction SilentlyContinue) {
            if (Select-String -LiteralPath $roleLog.FullName -Pattern 'Fatal error:|Assertion failed:|Ensure condition failed:|Handled ensure:' -Quiet) {
                throw \"A probe failed before $Role / $Pattern. Evidence: $evidence\"
            }
        }"""
if old_w not in st:
    raise SystemExit("wait-log block not found")
smoke.write_text(st.replace(old_w, new_w), encoding="utf-8", newline="\n")
print("smoke", smoke.stat().st_size)
