#include "Core/DryHullProfile.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyDryHullTest,"CalmyPiratesCo.Water.DryHullExclusion",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyDryHullTest::RunTest(const FString&)
{
    const TArray<FVector2D> S={{-6100,640},{-2000,1020},{0,1020},{5200,820},{6100,45}};
    const TArray<FVector2D> V={{0,.46},{190,.9},{320,.97},{680,1},{900,.995},{1050,.97}};
    TestTrue(TEXT("Engine below waterline stays dry"),FCalmyDryHullProfile::Contains(FVector(-4000,400,300),S,V));
    TestTrue(TEXT("Lower hold stays dry"),FCalmyDryHullProfile::Contains(FVector(0,800,450),S,V));
    TestFalse(TEXT("Outside bow not dry bounding box"),FCalmyDryHullProfile::Contains(FVector(6000,500,700),S,V));
    TestFalse(TEXT("Beside hull remains ocean"),FCalmyDryHullProfile::Contains(FVector(0,1025,700),S,V));
    TestFalse(TEXT("Under keel remains ocean"),FCalmyDryHullProfile::Contains(FVector(0,0,-100),S,V));
    TestFalse(TEXT("Above hull not an infinite dry column"),FCalmyDryHullProfile::Contains(FVector(0,0,1200),S,V));
    TestFalse(TEXT("Narrow lower bilge respects cross-section"),FCalmyDryHullProfile::Contains(FVector(0,800,10),S,V));
    TestFalse(TEXT("WaterLab has no exclusion"),FCalmyDryHullProfile::Contains(FVector::ZeroVector,{},{}));
    return true;
}
#endif
