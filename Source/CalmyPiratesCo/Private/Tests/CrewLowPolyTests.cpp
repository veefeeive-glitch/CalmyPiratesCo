#include "Misc/AutomationTest.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCalmyLowPolyAssetsTest,"CalmyPiratesCo.Character.LowPolyAssetContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCalmyLowPolyAssetsTest::RunTest(const FString&)
{
    const TCHAR* Parts[]={TEXT("Torso"),TEXT("Head"),TEXT("UpperArmL"),TEXT("ForearmL"),TEXT("UpperArmR"),TEXT("ForearmR"),TEXT("ThighL"),TEXT("ShinL"),TEXT("ThighR"),TEXT("ShinR")};
    for(const TCHAR* Variant:{TEXT("Deckhand"),TEXT("Misha")})
    {
        int32 Triangles=0;
        for(const TCHAR* Part:Parts)
        {
            const FString Path=FString::Printf(TEXT("/Game/Characters/CrewR8/SM_%s_%s.SM_%s_%s"),Variant,Part,Variant,Part);
            const UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
            if(!TestNotNull(*Path,Mesh))continue;
            TestTrue(TEXT("Physical rig has simple collision"),Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.GetElementCount()>0);
            TestTrue(TEXT("Material slots present"),Mesh->GetStaticMaterials().Num()>0);
            const FBox B=Mesh->GetBoundingBox();
            TestTrue(TEXT("Centimetre scale, no export offset"),B.IsValid && B.GetSize().GetMax()<100 && B.GetCenter().Size()<70);
            if(FString(Part)==TEXT("Head")) TestTrue(TEXT("Neck overlaps collar"),B.Min.Z<=-4.9);
            if(FString(Part)==TEXT("Head")) TestTrue(TEXT("Adult headwear width below 31cm"),B.GetSize().Y<31);
            if(FString(Part).StartsWith(TEXT("Forearm")))TestTrue(TEXT("Slim first-person forearm"),B.GetSize().Y<14);
            Triangles+=Mesh->GetNumTriangles(0);
        }
        TestTrue(TEXT("Authored low-poly budget per complete character"),Triangles>1000 && Triangles<6500);
    }
    return true;
}
#endif
