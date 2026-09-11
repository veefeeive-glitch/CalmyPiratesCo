#include "Gameplay/FlyNavigation.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlyNavigationTest, "CalmyPiratesCo.FlySwarm.RouteAroundBulkhead", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFlyNavigationTest::RunTest(const FString&)
{
    FFlyNavigation N;
    auto Passage = [](const FVector& A, const FVector& B)
    {
        if (FMath::Abs(B.Y)>300 || B.X < -75 || B.X > 600 || FMath::Abs(B.Z)>1) return false;
        // Wall at x=112.5, opening at y=225. Direct movement is forbidden.
        return !((A.X < 112.5) != (B.X < 112.5)) || (A.Y >= 200 && B.Y >= 200);
    };
    N.Begin(FVector::ZeroVector, FVector(525,0,0), true, 0);
    int32 Calls = 0;
    bool Done = false;
    for (int32 Tick=0; Tick<100 && !Done; ++Tick)
    {
        int32 Work=0;
        Done = N.Advance([&](const FVector& A, const FVector& B){++Work; return Passage(A,B);}, 3);
        TestTrue(TEXT("Bounded per-tick collision work"), Work<=18); ++Calls;
    }
    TestTrue(TEXT("Incremental search completed"), Done && Calls>1);
    if (!TestFalse(TEXT("Found a path"), N.Path.IsEmpty())) return false;
    TestTrue(TEXT("Reached destination beyond wall"), FVector::Dist(N.Path.Last(),FVector(525,0,0))<1);
    FVector Previous=FVector::ZeroVector; bool ThroughOpening=false;
    for (const auto& P:N.Path) {TestTrue(TEXT("Every segment clear"), Passage(Previous,P));ThroughOpening|=P.Y>=225;Previous=P;}
    TestTrue(TEXT("Used opening instead of crossing wall"),ThroughOpening);
    N.Begin(FVector::ZeroVector,FVector(525,0,0),true,0);
    while(!N.Advance([](const FVector&,const FVector&){return false;})) {}
    TestTrue(TEXT("Closed enclosure returns no route"),N.Path.IsEmpty());
    return true;
}
#endif
