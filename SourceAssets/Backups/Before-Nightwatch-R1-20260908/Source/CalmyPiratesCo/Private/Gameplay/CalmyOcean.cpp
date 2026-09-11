#include "Gameplay/CalmyOcean.h"
#include "Core/DryHullProfile.h"
#include "ProceduralMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Components/StaticMeshComponent.h"

ACalmyOcean::ACalmyOcean()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.f / 30.f;
    bReplicates = true; bAlwaysRelevant = true;
    Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OceanSurface"));
    SetRootComponent(Surface);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Surface->SetCastShadow(false);
}
ACalmyOcean* ACalmyOcean::Find(const UWorld* World)
{
    if (World) for (TActorIterator<ACalmyOcean> It(World); It; ++It) return *It;
    return nullptr;
}
float ACalmyOcean::SeaStrength(double T)
{
    const double Packet=.5-.5*FMath::Cos(T*2*PI/80.);
    return .65f+2.f*Packet*Packet*Packet;
}
float ACalmyOcean::WaveOffset(const FVector2D& P, double T)
{
    // Moderate harbour swell. Identical evaluator for all force samples and vertices.
    return SeaStrength(T)*(14.f * FMath::Sin(P.X * .0018 + P.Y * .0007 - T * 1.1)
         + 8.f * FMath::Sin(P.Y * .0031 - P.X * .0012 - T * 1.65)
         + 4.f * FMath::Sin(P.X * .007 + P.Y * .004 - T * 2.4));
}
float ACalmyOcean::Immersion(float D, float R)
{
    if (!FMath::IsFinite(D) || !FMath::IsFinite(R) || R <= 0) return 0;
    const float H = FMath::Clamp(D + R, 0.f, 2.f * R);
    return H * H * (3.f * R - H) / (4.f * R * R * R);
}
float ACalmyOcean::WaveVerticalVelocity(const FVector2D& P,double T)
{
    const double Packet=.5-.5*FMath::Cos(T*2*PI/80.);
    const double Derivative=3*Packet*Packet*FMath::Sin(T*2*PI/80.)*(2*PI/80.);
    return SeaStrength(T)*(-15.4f*FMath::Cos(P.X*.0018+P.Y*.0007-T*1.1)
         -13.2f*FMath::Cos(P.Y*.0031-P.X*.0012-T*1.65)
         -9.6f*FMath::Cos(P.X*.007+P.Y*.004-T*2.4)) + Derivative*WaveOffset(P,T)/SeaStrength(T);
}
FVector ACalmyOcean::FlowAt(const FVector& P) const
{
    const double T=WaveTime();const float Effort=SeaStrength(T)-.65f;
    return FVector(18+Effort*32*FMath::Sin(T*.35+P.Y*.001),-8+Effort*20*FMath::Cos(T*.29+P.X*.001),0);
}
double ACalmyOcean::WaveTime() const
{
    const auto* GS = GetWorld()->GetGameState();
    return GS ? GS->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
}
float ACalmyOcean::HeightAt(const FVector& P) const
{
    return GetActorLocation().Z + WaveOffset(FVector2D(P - GetActorLocation()), WaveTime());
}
bool ACalmyOcean::IsOutsideSafetyArea(const FVector& P) const
{
    return P.ContainsNaN() || FVector2D(P - GetActorLocation()).SizeSquared() > FMath::Square(25000.f)
        || P.Z < GetActorLocation().Z - 1800.f;
}
void ACalmyOcean::ApplyBuoyancy(UPrimitiveComponent* Body, float LiftRatio) const
{
    // Clients forward-simulate the same forces for smooth rigid-body presentation;
    // replicated server body state remains authoritative (no client force RPC).
    if (!Body || !Body->IsSimulatingPhysics() || IsInsideDryHull(Body->GetComponentLocation())) return;
    const float Mass = Body->GetMass();
    // Use scaled local bounds. Rotating a world AABB again inflated sample
    // spacing as a crate rolled, changing its buoyancy with yaw alone.
    const FTransform Transform = Body->GetComponentTransform();
    const FVector Extent=Body->CalcBounds(FTransform(FQuat::Identity,FVector::ZeroVector,Transform.GetScale3D())).BoxExtent;
    const float Radius = FMath::Clamp(Extent.Z, 12.f, 150.f);
    const float RX = FMath::Clamp(Extent.X * .6f, 10.f, 150.f);
    const float RY = FMath::Clamp(Extent.Y * .6f, 10.f, 150.f);
    for (float X : {-1.f, 1.f}) for (float Y : {-1.f, 1.f})
    {
        const FVector Point = Body->GetComponentLocation() + Transform.GetRotation().RotateVector(FVector(X*RX,Y*RY,0));
        const float Wet = IsInsideDryHull(Point)?0.f:Immersion(HeightAt(Point) - Point.Z, Radius);
        if (Wet <= 0) continue;
        const FVector V = Body->GetPhysicsLinearVelocityAtPoint(Point);
        const float WaterVZ=WaveVerticalVelocity(FVector2D(Point-GetActorLocation()),WaveTime());
        const float Lift = FMath::Clamp(980.f * LiftRatio * Wet - (V.Z-WaterVZ) * 3.5f * Wet, -980.f, 3920.f);
        // Mild current plus dissipative horizontal/vertical drag. Forces at four
        // locations also damp angular motion; no transform teleport/bobbing.
        const FVector Flow=FlowAt(Point);
        const FVector Force((Flow.X-V.X)*1.5f*Wet, (Flow.Y-V.Y)*1.5f*Wet, Lift);
        Body->AddForceAtLocation(Force * (Mass * .25f), Point);
    }
}
void ACalmyOcean::BeginPlay()
{
    Super::BeginPlay();
    if (GetNetMode() == NM_DedicatedServer) return;
    constexpr int32 N=192;
    TArray<int32> Triangles;
    for(int32 Y=0;Y<=N;++Y) for(int32 X=0;X<=N;++X)
    {
        // Stretch only the remote boundary ring beyond the playable safety area.
        // This closes the visible horizon without a dense kilometre-scale grid.
        auto Coord=[](int32 I) { const int32 A=FMath::Abs(I-N/2);const float Sign=I<N/2?-1.f:1.f;
            return I==0?-2000000.f:I==N?2000000.f:Sign*(A<=64?A*100.f:6400.f+(A-64)*750.f); };
        Vertices.Add(FVector(Coord(X),Coord(Y),0));
        Normals.Add(FVector::UpVector);UVs.Add(FVector2D(X*.25f,Y*.25f));
        if(X<N && Y<N) { const int32 A=Y*(N+1)+X; Triangles.Append({A,A+N+1,A+1,A+1,A+N+1,A+N+2}); }
    }
    UpdateSurface();
    Surface->CreateMeshSection(0,Vertices,Triangles,Normals,UVs,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
    Surface->SetMaterial(0,SurfaceMaterial);
    Surface->SetMaterial(1,FoamMaterial);
}
bool ACalmyOcean::IsInsideDryHull(const FVector& Position) const
{
    return FCalmyDryHullProfile::Contains(Position-DryHullOrigin,DryHullStations,DryHullVertical);
}
void ACalmyOcean::UpdateSurface()
{
    const double T=WaveTime();
    for(int32 I=0;I<Vertices.Num();++I)
    {
        FVector& P=Vertices[I];
        // Fade to a flat far sea only OUTSIDE the 250 m playable safety radius.
        // Otherwise interpolated wave normals across the stretched outer ring
        // produce kilometre-long specular streaks at the horizon.
        const float Fade=FMath::Clamp((31000.f-FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y)))/6000.f,0.f,1.f);
        P.Z=WaveOffset(FVector2D(P),T)*Fade;
        const float DX=(WaveOffset(FVector2D(P.X+5,P.Y),T)-WaveOffset(FVector2D(P.X-5,P.Y),T))*.1f;
        const float DY=(WaveOffset(FVector2D(P.X,P.Y+5),T)-WaveOffset(FVector2D(P.X,P.Y-5),T))*.1f;
        Normals[I]=FVector(-DX*Fade,-DY*Fade,1).GetSafeNormal();
    }
}
void ACalmyOcean::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(Vertices.IsEmpty()) return;
    UpdateSurface();
    Surface->UpdateMeshSection(0,Vertices,Normals,UVs,TArray<FColor>(),TArray<FProcMeshTangent>());
    UpdateInteractions();
}

void ACalmyOcean::UpdateInteractions()
{
    // Local visual feedback derived from replicated actor positions. No client
    // ripple/force RPC; transient history is deliberately not replayed on late join.
    const double Now=WaveTime();
    TSet<TWeakObjectPtr<AActor>> Seen;
    auto Touch=[&](AActor* Actor,float Radius)
    {
        const FVector P=Actor->GetActorLocation();if(IsInsideDryHull(P))return;const float Z=HeightAt(P);
        if(P.Z-Radius>Z || P.Z+Radius<Z) return;
        Seen.Add(Actor);double* Last=WetActors.Find(Actor);
        if(!Last || (Actor->GetVelocity().Size2D()>25 && Now-*Last>.23))
        {
            const float Strength=!Last?FMath::Clamp(Actor->GetVelocity().Size()/300.f,.65f,2.f):.3f;
            if(Ripples.Num()>=48) Ripples.RemoveAt(0);
            Ripples.Add({FVector(P.X,P.Y,Z),Now,Strength});WetActors.Add(Actor,Now);
        }
    };
    for(TActorIterator<ACalmyCharacter> It(GetWorld());It;++It) Touch(*It,88);
    for(TActorIterator<ACalmyCargo> It(GetWorld());It;++It) if(!It->Carry.Holder) Touch(*It,It->Mesh->Bounds.BoxExtent.Z);
    for(auto It=WetActors.CreateIterator();It;++It) if(!Seen.Contains(It.Key())) It.RemoveCurrent();
    Ripples.RemoveAll([&](const FRipple& R){return Now-R.Birth>2.2;});
    TArray<FVector> V,N;TArray<int32> Tris;TArray<FVector2D> UV;TArray<FLinearColor> Colors;
    auto Quad=[&](const FVector& A,const FVector& B,const FVector& C,const FVector& D,float Alpha)
    {
        const int32 I=V.Num();V.Append({A,B,C,D});Tris.Append({I,I+1,I+2,I,I+2,I+3});
        for(int32 J=0;J<4;++J){N.Add(FVector::UpVector);UV.Add(FVector2D(J==1||J==2,J>=2));Colors.Add(FLinearColor(.75,.94,1,Alpha));}
    };
    for(const FRipple& R:Ripples)
    {
        const float Age=Now-R.Birth;const float Alpha=FMath::Clamp((2.2f-Age)*.5f,0.f,1.f)*FMath::Min(R.Strength,1.f);
        const FVector Center=R.Position-GetActorLocation();const float Radius=28+Age*100;
        for(int32 J=0;J<32;++J)
        {
            auto Point=[&](float Angle,float Rad){FVector P=Center+FVector(FMath::Cos(Angle)*Rad,FMath::Sin(Angle)*Rad,0);P.Z=WaveOffset(FVector2D(P),Now)+3;return P;};
            const float A=J*2*PI/32,B=(J+1)*2*PI/32;
            Quad(Point(A,Radius),Point(B,Radius),Point(B,Radius+8),Point(A,Radius+8),Alpha);
        }
        if(R.Strength>.6f && Age<.7f) for(int32 J=0;J<12;++J)
        {
            const float A=J*2*PI/12;const FVector Side(-FMath::Sin(A)*3,FMath::Cos(A)*3,0);
            FVector P=Center+FVector(FMath::Cos(A),FMath::Sin(A),0)*(24+Age*90);
            P.Z=WaveOffset(FVector2D(P),Now)+FMath::Max(0.f,180*R.Strength*Age-260*Age*Age);
            Quad(P-Side,P+Side,P+Side+FVector(0,0,12),P-Side+FVector(0,0,12),Alpha);
        }
    }
    if(V.IsEmpty()) Surface->ClearMeshSection(1);
    else Surface->CreateMeshSection_LinearColor(1,V,Tris,N,UV,Colors,TArray<FProcMeshTangent>(),false);
}
