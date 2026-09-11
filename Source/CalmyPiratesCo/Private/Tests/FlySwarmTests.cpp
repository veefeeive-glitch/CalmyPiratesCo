#include "Gameplay/FlySwarmMob.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyGraphTest, "CalmyPiratesCo.FlySwarm.Connectome", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyGraphTest::RunTest(const FString&)
{
    const FFlyConnectome& G = FFlyConnectome::Get();
    TestTrue(TEXT("Shared graph identity"), &G == &FFlyConnectome::Get());
    TestEqual(TEXT("668 CSR rows"), G.RowStart[668], 18968u);
    int32 Exc = 0, Inh = 0;
    for (int32 I = 0; I < 668; ++I)
    {
        if (G.RowStart[I] > G.RowStart[I + 1]) { AddError(TEXT("Unordered CSR")); return false; }
        for (uint32 K = G.RowStart[I]; K < G.RowStart[I + 1]; ++K)
        {
            if (G.Targets[K] >= 668 || !FMath::IsFinite(G.Weights[K])) { AddError(TEXT("Invalid edge")); return false; }
            Exc += G.Weights[K] > 0; Inh += G.Weights[K] < 0;
        }
    }
    TestEqual(TEXT("Positive edges preserved"), Exc, 11171);
    TestEqual(TEXT("Negative edges preserved"), Inh, 7797);
    TestEqual(TEXT("Loom population"), int32(G.GroupCount[1]), 314);
    TestEqual(TEXT("GF 152"), int32(G.Role[152]), 8);
    TestEqual(TEXT("GF 255"), int32(G.Role[255]), 8);
    TestEqual(TEXT("DNa left"), int32(G.GroupCount[2]), 2);
    TestEqual(TEXT("DNa right"), int32(G.GroupCount[3]), 2);
    AddInfo(FString::Printf(TEXT("Per-fly brain bytes=%llu; agent bytes=%llu"), uint64(sizeof(FFlyLIFState)), uint64(sizeof(FFlyAgentState))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyReferenceTest, "CalmyPiratesCo.FlySwarm.SourceLIFParity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyReferenceTest::RunTest(const FString&)
{
    // Independent reference uses the original 5*N float inhibition queue.
    // Disable stochastic noise in both paths, retain real weights and baseline.
    const FFlyConnectome& G = FFlyConnectome::Get();
    FFlyLIFState Fly; Fly.Initialize(1);
    float V[668] = {}, Refr[668] = {}, Queue[5][668] = {}, Rates[9] = {};
    int32 Head = 0, ReferenceGF = 0, ActualGF = 0;
    for (int32 T = 0; T < 1000; ++T)
    {
        const float Left = T >= 200 && T < 700 ? 0.9f : 0.f;
        const float Right = T >= 400 && T < 800 ? 0.4f : 0.f;
        Fly.bEscape = false; Fly.Step(G, Left, Right, 0.3f, false);
        ActualGF += Fly.bEscape;
        for (int32 I = 0; I < 668; ++I)
        {
            if (Refr[I] > 0) { Refr[I] -= 1; V[I] *= 0.9512f; }
            else V[I] = V[I] * 0.9512f + G.Baseline[I];
            if (G.SensorSide[I]) V[I] += (G.SensorSide[I] == 1 ? Left : Right) * 0.3f;
            if (Queue[Head][I] != 0) { V[I] = FMath::Max(-2.f, V[I] + Queue[Head][I]); Queue[Head][I] = 0; }
        }
        TArray<int32, TInlineAllocator<668>> Spikes;
        int32 Counts[9] = {}; bool GF = false;
        for (int32 I = 0; I < 668; ++I)
            if (Refr[I] <= 0 && V[I] >= 1.f)
            {
                V[I] = 0; Refr[I] = 2; Spikes.Add(I); ++Counts[G.Role[I]]; GF |= G.Role[I] == 8;
            }
        ReferenceGF += GF;
        for (int32 I : Spikes)
            for (uint32 K = G.RowStart[I]; K < G.RowStart[I + 1]; ++K)
            {
                const int32 J = G.Targets[K]; const float W = G.Weights[K];
                if (W >= 0) V[J] = FMath::Max(-2.f, V[J] + W);
                else Queue[(Head + 4) % 5][J] += W;
            }
        Head = (Head + 1) % 5;
        Counts[0] = Spikes.Num();
        for (int32 R = 0; R < 9; ++R)
        {
            Rates[R] += (Counts[R] * 1000.f / (R == 0 ? 668 : FMath::Max(1, int32(G.GroupCount[R]))) - Rates[R]) / 120.f;
            if (!FMath::IsNearlyEqual(Rates[R], Fly.Rates[R], 0.0001f)) { AddError(FString::Printf(TEXT("Rate parity ms=%d group=%d"), T, R)); return false; }
        }
        for (int32 I = 0; I < 668; ++I)
            if (!FMath::IsNearlyEqual(V[I], Fly.Potential[I], 0.0001f) || Refr[I] != Fly.RefractoryMs[I])
            { AddError(FString::Printf(TEXT("LIF parity ms=%d neuron=%d"), T, I)); return false; }
    }
    TestEqual(TEXT("GF latch parity"), ActualGF, ReferenceGF);
    TestTrue(TEXT("Real looming pathway fires GF"), ActualGF > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyDelayTest, "CalmyPiratesCo.FlySwarm.DelayAndRefractory", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyDelayTest::RunTest(const FString&)
{
    // Minimal directed inhibitory edge 0 -> 1, with no noise/baseline.
    uint32 Rows[669]; Rows[0] = 0; for (int32 I = 1; I < 669; ++I) Rows[I] = 1;
    const uint16 Target[] = {1}; const float Weight[] = {-0.5f};
    float Baseline[668] = {}; uint8 Role[668] = {}, Sensor[668] = {}; uint16 Counts[9] = {};
    const FFlyConnectome G{Rows, Target, Weight, Baseline, Role, Sensor, Counts};
    FFlyLIFState A; A.Initialize(1); A.Potential[0] = 2.f;
    A.Step(G, 0, 0, 0, false);
    TestEqual(TEXT("Reset"), A.Potential[0], 0.f);
    TestEqual(TEXT("Two ms refractory"), int32(A.RefractoryMs[0]), 2);
    A.Potential[0] = 1.5f;
    A.Step(G, 0, 0, 0, false);
    TestEqual(TEXT("Refractory prevents spike"), int32(A.RefractoryMs[0]), 1);
    TestTrue(TEXT("Source retains subthreshold membrane during refractory"), A.Potential[0] > 1.f);
    A.Step(G, 0, 0, 0, false);
    TestEqual(TEXT("Can fire as refractory reaches zero"), int32(A.RefractoryMs[0]), 2);
    A.Step(G, 0, 0, 0, false);
    TestEqual(TEXT("No inhibition before 4 ms"), A.Potential[1], 0.f);
    A.Step(G, 0, 0, 0, false);
    TestEqual(TEXT("Inhibition arrives exactly 4 ms later"), A.Potential[1], -0.5f);
    FFlyLIFState B; B.Initialize(1);
    TestEqual(TEXT("Another fly state remains independent"), B.Potential[1], 0.f);
    return true;
}
#endif
