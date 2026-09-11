#include "Core/CrewGait.h"
#include "Core/TeamCarryPace.h"
#include "Core/CrewArmIK.h"
#include "Core/CrewImpact.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyImpactTest,"CalmyPiratesCo.Character.ImpactDamage",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyImpactTest::RunTest(const FString&)
{
    TestEqual(TEXT("Ordinary jump landing is safe"),FCalmyCrewImpact::Damage(650,true),0.f);
    TestEqual(TEXT("Negative impact is ignored"),FCalmyCrewImpact::Damage(-1000,true),0.f);
    TestTrue(TEXT("High fall hurts more"),FCalmyCrewImpact::Damage(1300,true)>FCalmyCrewImpact::Damage(900,true));
    TestTrue(TEXT("Stumble is a minor injury"),FCalmyCrewImpact::Damage(650,false)>0 && FCalmyCrewImpact::Damage(650,false)<3);
    TestEqual(TEXT("Damage bounded"),FCalmyCrewImpact::Damage(100000,true),100.f);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyArmTest,"CalmyPiratesCo.Character.HandContactIK",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyArmTest::RunTest(const FString&)
{
    for(const FVector Target:{FVector(40,0,-25),FVector(52,7,-20),FVector(58,-5,2)})
    {
        const FVector E=FCalmyArmIK::Elbow(FVector::ZeroVector,Target,FVector(0,0,-1));
        TestTrue(TEXT("Upper arm retains 30cm length"),FMath::Abs(E.Size()-30)<.01);
        TestTrue(TEXT("Forearm reaches grip without stretching"),FMath::Abs(FVector::Distance(E,Target)-34)<.01);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyGaitTest,"CalmyPiratesCo.Character.GroundAwareLegTargets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyGaitTest::RunTest(const FString&)
{
    for(float X:{-30.f,0.f,30.f}) for(float Z:{60.f,65.f,75.f})
    {
        const auto P=FCalmyLegPose::Solve(X,Z);
        const float H=FMath::DegreesToRadians(P.Hip),K=FMath::DegreesToRadians(P.Knee);
        const FVector2D Actual(38*FMath::Sin(H)+44*FMath::Sin(H+K),38*FMath::Cos(H)+44*FMath::Cos(H+K));
        TestTrue(TEXT("Leg reaches supported target without hyperextension"),FVector2D::Distance(Actual,FVector2D(X,Z).GetClampedToMaxSize(81))<1);
        TestTrue(TEXT("Knee bends in one anatomical direction"),P.Knee<=0 && P.Knee>=-100);
    }
    const auto Far=FCalmyLegPose::Solve(10000,10000);
    TestTrue(TEXT("Unreachable target stays finite and bounded"),FMath::IsFinite(Far.Hip)&&FMath::Abs(Far.Hip)<=80&&Far.Knee<=0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyTeamPaceTest,"CalmyPiratesCo.Cargo.TeamPace",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyTeamPaceTest::RunTest(const FString&)
{
    TestEqual(TEXT("Close synchronized crew gets full pace"),FCalmyTeamCarryPace::Evaluate(140,0),1.f);
    TestTrue(TEXT("Separation slows the holder"),FCalmyTeamCarryPace::Evaluate(250,0)<.8f);
    TestTrue(TEXT("Opposed movement adds effort"),FCalmyTeamCarryPace::Evaluate(200,450)<FCalmyTeamCarryPace::Evaluate(200,0));
    TestEqual(TEXT("Hard lower bound"),FCalmyTeamCarryPace::Evaluate(10000,10000),.55f);
    TestEqual(TEXT("Negative inputs do not accelerate"),FCalmyTeamCarryPace::Evaluate(-10,-500),1.f);
    return true;
}
#endif
