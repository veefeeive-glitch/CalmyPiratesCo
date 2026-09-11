#include "Core/BodycamSpring.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyBodycamTest,"CalmyPiratesCo.Character.BodycamBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyBodycamTest::RunTest(const FString&)
{
    for(float DT:{1.f/30,1.f/60,1.f/120,.5f})
    {
        FCalmyBodycamSpring S;S.Velocity=FVector(0,0,-35);
        for(int32 I=0;I<400;++I)
        {
            S.Step(I<100?FVector(100,-100,40):FVector::ZeroVector,DT);
            TestFalse(TEXT("Finite camera"),S.Position.ContainsNaN());
            TestTrue(TEXT("Bounded roll and pitch"),FMath::Abs(S.Position.X)<=5 && FMath::Abs(S.Position.Y)<=5);
            TestTrue(TEXT("Bounded landing displacement"),FMath::Abs(S.Position.Z)<=6);
        }
        TestTrue(TEXT("Camera settles"),S.Position.Size()<.01);
    }
    return true;
}
#endif
