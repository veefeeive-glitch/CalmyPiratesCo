#include "Core/WaterImpulse.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyWaterImpulseTest,"CalmyPiratesCo.Water.PhysicalDisturbanceField",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyWaterImpulseTest::RunTest(const FString&)
{
    using W=FCalmyWaterImpulse;
    TestEqual(TEXT("No wave before impact"),W::Height(0,-1,10,70),0.f);
    TestEqual(TEXT("No birth pop"),W::Height(0,0,10,70),0.f);
    TestEqual(TEXT("Expired wave"),W::Height(900,5,10,70),0.f);
    TestEqual(TEXT("Invalid width"),W::Height(100,1,10,0),0.f);
    TestEqual(TEXT("Distant point is unaffected"),W::Height(10000,1,10,70),0.f);
    TestEqual(TEXT("No energy at zero relative speed"),W::ImpactAmplitude(70,0),0.f);
    TestTrue(TEXT("Heavier impact is stronger"),W::ImpactAmplitude(32,300)>W::ImpactAmplitude(12,300));
    TestEqual(TEXT("Extreme impact is bounded"),W::ImpactAmplitude(10000,10000),16.f);
    TestTrue(TEXT("Swept entry detected even when entirely underwater now"),W::CrossedSurface(50,-200,0,0));
    TestFalse(TEXT("Already underwater is not a new entry"),W::CrossedSurface(-40,-50,0,0));
    TestFalse(TEXT("Leaving water is not an impact"),W::CrossedSurface(-40,50,0,0));
    TestFalse(TEXT("Teleport suppressed"),W::ContinuousMotion(1./30.,1000));
    TestFalse(TEXT("Hitch suppressed"),W::ContinuousMotion(.5,100));
    TestTrue(TEXT("Fast legitimate entry accepted"),W::ContinuousMotion(1./30.,40));
    float MaxHeight=0,MaxSpeed=0;
    for(int32 I=0;I<=500;++I)
    {
        const double T=I*.01;
        for(float R:{0.f,80.f,200.f,500.f,950.f})
        {
            const float H=W::Height(R,T,16,80);
            const float V=(W::Height(R,T+.002,16,80)-W::Height(R,T-.002,16,80))/.004f;
            MaxHeight=FMath::Max(MaxHeight,FMath::Abs(H));MaxSpeed=FMath::Max(MaxSpeed,FMath::Abs(V));
            TestTrue(TEXT("Finite bounded response"),FMath::IsFinite(H)&&FMath::IsFinite(V)&&FMath::Abs(H)<=16);
            TestEqual(TEXT("Reconstructed late-join event is deterministic"),H,W::Height(R,T,16,80));
        }
    }
    TestTrue(TEXT("Actual geometric displacement, not just an effect count"),MaxHeight>8);
    TestTrue(TEXT("Nonzero bounded velocity for drag coupling"),MaxSpeed>10&&MaxSpeed<180);
    TestTrue(TEXT("Many coincident waves remain below 24cm"),W::Limit(48*16)<24);
    TestTrue(TEXT("Limit is odd"),FMath::IsNearlyEqual(W::Limit(100),-W::Limit(-100)));
    TestTrue(TEXT("Wave propagates outward"),W::Height(180,1,10,70)>W::Height(0,1,10,70));
    AddInfo(FString::Printf(TEXT("peak_height_cm=%.3f peak_vertical_speed_cm_s=%.3f"),MaxHeight,MaxSpeed));
    return true;
}
#endif
