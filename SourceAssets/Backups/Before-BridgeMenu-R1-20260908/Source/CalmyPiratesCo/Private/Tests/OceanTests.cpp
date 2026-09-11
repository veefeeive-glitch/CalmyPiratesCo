#include "Gameplay/CalmyOcean.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyOceanTest,"CalmyPiratesCo.Water.WavesAndDisplacement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyOceanTest::RunTest(const FString&)
{
    TestEqual(TEXT("Dry sphere"),ACalmyOcean::Immersion(-50,35),0.f);
    TestEqual(TEXT("Full sphere"),ACalmyOcean::Immersion(50,35),1.f);
    TestEqual(TEXT("Half volume at centre"),ACalmyOcean::Immersion(0,35),.5f);
    TestEqual(TEXT("Invalid radius does not divide"),ACalmyOcean::Immersion(1,0),0.f);
    for(int32 I=0;I<1000;++I)
    {
        const FVector2D P(I*79,I*-33);const double T=I*.031;
        const float A=ACalmyOcean::WaveOffset(P,T);
        TestTrue(TEXT("Bounded wave packets: sum amplitudes 26cm times maximum envelope 2.65"),FMath::IsFinite(A)&&FMath::Abs(A)<=68.901f);
        TestEqual(TEXT("Same network time gives same field"),A,ACalmyOcean::WaveOffset(P,T));
        const float Difference=(ACalmyOcean::WaveOffset(P,T+.001)-ACalmyOcean::WaveOffset(P,T-.001))/.002f;
        TestTrue(TEXT("Water drag follows analytic wave velocity"),FMath::Abs(Difference-ACalmyOcean::WaveVerticalVelocity(P,T))<.015f);
    }
    TestTrue(TEXT("Quiet phase"),FMath::IsNearlyEqual(ACalmyOcean::SeaStrength(0),.65f));
    TestTrue(TEXT("Strong packet every 80 seconds"),FMath::IsNearlyEqual(ACalmyOcean::SeaStrength(40),2.65f,1.e-5f));
    TestTrue(TEXT("Returns to quiet"),FMath::IsNearlyEqual(ACalmyOcean::SeaStrength(80),.65f));
    return true;
}
#endif
