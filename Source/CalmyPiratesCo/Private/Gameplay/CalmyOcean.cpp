#include "Gameplay/CalmyOcean.h"
#include "Core/DryHullProfile.h"
#include "ProceduralMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyCargo.h"
#include "Components/StaticMeshComponent.h"
#include "Gameplay/CalmyCrewPresentation.h"
#include "Core/WaterImpulse.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ACalmyOcean::ACalmyOcean()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 1.f / 30.f;
    bReplicates = true; bAlwaysRelevant = true; SetNetUpdateFrequency(15.f);
    Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OceanSurface"));
    SetRootComponent(Surface);
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Surface->SetCastShadow(false);
    // Separate translucent effects from SingleLayerWater. Rebuilding small
    // droplets must not recreate the entire 37k-vertex ocean scene proxy.
    InteractionMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterInteractions"));
    InteractionMesh->SetupAttachment(Surface);
    InteractionMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    InteractionMesh->SetCastShadow(false);
    InteractionMesh->SetTranslucentSortPriority(1);
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Foam(TEXT("/Game/Materials/M_WaterInteraction_W1.M_WaterInteraction_W1"));
    InteractionMaterial=Foam.Object;
}
void ACalmyOcean::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACalmyOcean, Disturbances);
    DOREPLIFETIME(ACalmyOcean, DisturbanceSerial);
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
    const double T = WaveTime();
    return GetActorLocation().Z + WaveOffset(FVector2D(P - GetActorLocation()), T) + DisturbanceHeightAt(P, T);
}
float ACalmyOcean::DisturbanceHeightAt(const FVector& P, double T) const
{
    if (Disturbances.IsEmpty() || P.ContainsNaN() || !FMath::IsFinite(T) || IsInsideDryHull(P)) return 0;
    float H = 0;
    for (const auto& D : Disturbances)
    {
        const double Age = T - D.Birth;
        if (Age <= 0 || Age >= FCalmyWaterImpulse::Lifetime) continue;
        const double Reach = Age * FCalmyWaterImpulse::Speed + D.Width * 4.;
        if (FMath::Abs(P.X-D.Position.X) > Reach || FMath::Abs(P.Y-D.Position.Y) > Reach) continue;
        H += FCalmyWaterImpulse::Height(FVector::Dist2D(P, D.Position), Age, D.Amplitude, D.Width);
    }
    return FCalmyWaterImpulse::Limit(H);
}
float ACalmyOcean::VerticalVelocityAt(const FVector& P) const
{
    const double T = WaveTime();
    // Derivative of the SAME limited field used by vertices and displacement.
    return WaveVerticalVelocity(FVector2D(P-GetActorLocation()), T)
        + (DisturbanceHeightAt(P,T+.002)-DisturbanceHeightAt(P,T-.002))/.004f;
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
        const float WaterVZ=VerticalVelocityAt(Point);
        const float Lift = FMath::Clamp(980.f * LiftRatio * Wet - (V.Z-WaterVZ) * 3.5f * Wet, -980.f, 3920.f);
#if !UE_BUILD_SHIPPING
        // Diagnostic of the actual applied sample, relative to the same body
        // in the background sea alone; not a second force or fixture impulse.
        const FVector2D Local(Point-GetActorLocation());
        const float BaseWet=Immersion(GetActorLocation().Z+WaveOffset(Local,WaveTime())-Point.Z,Radius);
        const float BaseLift=FMath::Clamp(980.f*LiftRatio*BaseWet-(V.Z-WaveVerticalVelocity(Local,WaveTime()))*3.5f*BaseWet,-980.f,3920.f);
        PeakDisturbanceAcceleration=FMath::Max(PeakDisturbanceAcceleration,FMath::Abs(Lift-BaseLift));
#endif
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
    WaveSin.Reserve(Vertices.Num());WaveCos.Reserve(Vertices.Num());
    for(const FVector& P:Vertices)
    {
        const FVector A(P.X*.0018+P.Y*.0007,P.Y*.0031-P.X*.0012,P.X*.007+P.Y*.004);
        WaveSin.Add(FVector(FMath::Sin(A.X),FMath::Sin(A.Y),FMath::Sin(A.Z)));
        WaveCos.Add(FVector(FMath::Cos(A.X),FMath::Cos(A.Y),FMath::Cos(A.Z)));
    }
    UpdateSurface();
    Surface->CreateMeshSection(0,Vertices,Triangles,Normals,UVs,TArray<FColor>(),TArray<FProcMeshTangent>(),false);
    Surface->SetMaterial(0,SurfaceMaterial);
    InteractionMesh->SetMaterial(0,InteractionMaterial?InteractionMaterial:FoamMaterial);
}
bool ACalmyOcean::IsInsideDryHull(const FVector& Position) const
{
    return FCalmyDryHullProfile::Contains(Position-DryHullOrigin,DryHullStations,DryHullVertical);
}
void ACalmyOcean::UpdateSurface()
{
    const double T=WaveTime();
    SurfaceTime=T;
    // Spatial phases never change on this static grid. Angle subtraction replaces
    // five complete wave evaluations per vertex with cached vector arithmetic.
    const FVector TimeSin(FMath::Sin(T*1.1),FMath::Sin(T*1.65),FMath::Sin(T*2.4));
    const FVector TimeCos(FMath::Cos(T*1.1),FMath::Cos(T*1.65),FMath::Cos(T*2.4));
    const double Strength=SeaStrength(T);
    const FVector Amplitude=FVector(14,8,4)*Strength;
    // Preserve the original +/-5cm finite-difference normals exactly, rather
    // than silently changing the rendered wave slope to another approximation.
    const FVector SlopeX=FVector(14*FMath::Sin(.0018*5)/5,8*FMath::Sin(-.0012*5)/5,4*FMath::Sin(.007*5)/5)*Strength;
    const FVector SlopeY=FVector(14*FMath::Sin(.0007*5)/5,8*FMath::Sin(.0031*5)/5,4*FMath::Sin(.004*5)/5)*Strength;
    const FVector WorldOrigin=GetActorLocation();
    for(int32 I=0;I<Vertices.Num();++I)
    {
        FVector& P=Vertices[I];
        // Fade to a flat far sea only OUTSIDE the 250 m playable safety radius.
        // Otherwise interpolated wave normals across the stretched outer ring
        // produce kilometre-long specular streaks at the horizon.
        const float Fade=FMath::Clamp((31000.f-FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y)))/6000.f,0.f,1.f);
        if(Fade<=0){P.Z=0;Normals[I]=FVector::UpVector;continue;}
        const FVector Sin=WaveSin[I]*TimeCos-WaveCos[I]*TimeSin;
        const FVector Cos=WaveCos[I]*TimeCos+WaveSin[I]*TimeSin;
        P.Z=FVector::DotProduct(Sin,Amplitude)*Fade;
        float DX=FVector::DotProduct(Cos,SlopeX),DY=FVector::DotProduct(Cos,SlopeY);
        if(!Disturbances.IsEmpty())
        {
            const FVector At=WorldOrigin+FVector(P.X,P.Y,0);
            P.Z+=DisturbanceHeightAt(At,T)*Fade;
            DX+=(DisturbanceHeightAt(At+FVector(5,0,0),T)-DisturbanceHeightAt(At-FVector(5,0,0),T))*.1f;
            DY+=(DisturbanceHeightAt(At+FVector(0,5,0),T)-DisturbanceHeightAt(At-FVector(0,5,0),T))*.1f;
        }
        Normals[I]=FVector(-DX*Fade,-DY*Fade,1).GetSafeNormal();
    }
}
void ACalmyOcean::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // Must run on dedicated servers too: gameplay waves cannot depend on a GPU.
    if (HasAuthority()) UpdateDisturbanceSources();
    if(Vertices.IsEmpty()) return;
#if !UE_BUILD_SHIPPING
    static const bool SkipVisual = FString(FCommandLine::Get()).Contains(TEXT("CalmyPerformanceProbe=")) && FParse::Param(FCommandLine::Get(), TEXT("PerfNoWaterMesh"));
    if (SkipVisual) return;
#endif
    UpdateSurface();
    Surface->UpdateMeshSection(0,Vertices,Normals,UVs,TArray<FColor>(),TArray<FProcMeshTangent>());
    UpdateInteractions();
}

void ACalmyOcean::UpdateDisturbanceSources()
{
    const double Now = WaveTime();
    Disturbances.RemoveAll([&](const auto& D) { return Now-D.Birth >= FCalmyWaterImpulse::Lifetime; });
    TSet<TWeakObjectPtr<AActor>> Seen;
    auto Observe = [&](AActor* Actor, FVector P, FVector Velocity, float Radius, float Mass)
    {
        if (!Actor || P.ContainsNaN() || Velocity.ContainsNaN() || IsInsideDryHull(P) || IsOutsideSafetyArea(P)) return;
        Seen.Add(Actor);
        const float SurfaceZ = GetActorLocation().Z + WaveOffset(FVector2D(P-GetActorLocation()), Now);
        const float Bottom = P.Z - Radius;
        auto* Previous = WaterContacts.Find(Actor);
        if (Previous)
        {
            const double Dt = Now - Previous->Time;
            const float Distance = FVector::Distance(P, Previous->Previous);
            // Spawn, rescue/teleport and long hitches aren't physical impacts.
            const bool Continuous = FCalmyWaterImpulse::ContinuousMotion(Dt,Distance);
            const float VZ = Continuous ? (P.Z-Previous->Previous.Z)/Dt : 0.f;
            const float WaterVZ = WaveVerticalVelocity(FVector2D(P-GetActorLocation()), Now);
            const bool Entry = Continuous && FCalmyWaterImpulse::CrossedSurface(Previous->Bottom, Bottom, SurfaceZ, Previous->Surface);
            const bool AtSurface = Bottom <= SurfaceZ && P.Z+Radius >= SurfaceZ;
            const float SwimSpeed = (Velocity-FlowAt(P)).Size2D();
            const bool Wake = Continuous && AtSurface && SwimSpeed > 45 && Distance > 1 && Now-Previous->LastEmission > .45;
            const float Impact = FCalmyWaterImpulse::ImpactAmplitude(Mass,FMath::Max(0.f,WaterVZ-VZ));
            if (((Entry && Impact > .6f) || Wake) && Now-Previous->LastEmission > .35
                && Disturbances.Num() < FCalmyWaterImpulse::Capacity)
            {
                FCalmyWaterDisturbance D;
                D.Position = FVector(P.X,P.Y,SurfaceZ); D.Birth = Now;
                D.Width = FMath::Clamp(Radius*1.25f,55.f,150.f);
                D.Amplitude = Entry ? Impact : FMath::Clamp(SwimSpeed*.009f,.6f,2.8f);
                D.Serial = ++DisturbanceSerial; Disturbances.Add(D);
                Previous->LastEmission = Now;
                ForceNetUpdate();
            }
        }
        auto& Contact = WaterContacts.FindOrAdd(Actor);
        Contact.Previous=P; Contact.Time=Now; Contact.Bottom=Bottom; Contact.Surface=SurfaceZ;
    };
    for (TActorIterator<ACalmyCharacter> It(GetWorld()); It; ++It)
    {
        FVector P=It->GetActorLocation(); float Radius=88;
        if (const auto* Rig=It->GetCrewPresentation(); Rig && Rig->IsPhysicalFall())
        { P=Rig->GetChestTransform().GetLocation(); Radius=35; }
        Observe(*It,P,It->GetVelocity(),Radius,80);
    }
    for (TActorIterator<ACalmyCargo> It(GetWorld()); It; ++It)
        if (It->Mesh && !It->Carry.Holder && It->Mesh->IsSimulatingPhysics())
            Observe(*It,It->Mesh->Bounds.Origin,It->Mesh->GetPhysicsLinearVelocity(),It->Mesh->Bounds.BoxExtent.Z,It->Mesh->GetMass());
    for (auto It=WaterContacts.CreateIterator(); It; ++It)
        if (!Seen.Contains(It.Key())) It.RemoveCurrent();
}

void ACalmyOcean::UpdateInteractions()
{
    // Foam and ballistic droplets share the server wave's birth/location/energy.
    // They remain presentation; the height field above is what moves floaters.
    const double Now=WaveTime();
    TArray<FVector> V,N;TArray<int32> Tris;TArray<FVector2D> UV;TArray<FLinearColor> Colors;
    auto Quad=[&](const FVector& A,const FVector& B,const FVector& C,const FVector& D,float Alpha)
    {
        const int32 I=V.Num();V.Append({A,B,C,D});Tris.Append({I,I+1,I+2,I,I+2,I+3});
        for(int32 J=0;J<4;++J){N.Add(FVector::UpVector);UV.Add(FVector2D(J==1||J==2,J>=2));Colors.Add(FLinearColor(.75f,.94f,1,Alpha));}
    };
    auto Triangle=[&](FVector A,FVector B,FVector C,float Alpha)
    {
        const int32 I=V.Num();V.Append({A,B,C});Tris.Append({I,I+1,I+2});
        const FVector Normal=FVector::CrossProduct(B-A,C-A).GetSafeNormal();
        for(int32 J=0;J<3;++J){N.Add(Normal);UV.Add(FVector2D(.5,.5));Colors.Add(FLinearColor(.7f,.9f,1,Alpha));}
    };
    for(const auto& R:Disturbances)
    {
        const float Age=Now-R.Birth;
        if(Age<=0 || Age>=FCalmyWaterImpulse::Lifetime)continue;
        const float Alpha=FMath::Clamp(Age/.15f,0.f,1.f)*FMath::Exp(-Age*2.f)*FMath::Clamp(R.Amplitude/16.f,.08f,.6f);
        const FVector Center=R.Position-GetActorLocation();const float Radius=Age*FCalmyWaterImpulse::Speed;
        for(int32 J=0;Age<1.8f && J<40;++J)
        {
            // Broken foam arcs instead of perfectly continuous white hoops.
            if((J+R.Serial)%7<2)continue;
            auto Point=[&](float Angle,float Rad){Rad+=FMath::Sin(Angle*5+R.Serial)*FMath::Min(12.f,Age*12);FVector P=Center+FVector(FMath::Cos(Angle)*Rad,FMath::Sin(Angle)*Rad,0);P.Z=HeightAt(P+GetActorLocation())-GetActorLocation().Z+3;return P;};
            const float A=J*2*PI/40,B=(J+.85f)*2*PI/40;
            const float W=1.5f+FMath::Sin(J*2.3f+R.Serial);
            Quad(Point(A,Radius),Point(B,Radius),Point(B,Radius+W),Point(A,Radius+W),Alpha);
        }
        if(R.Amplitude>3 && Age<.85f) for(int32 J=0;J<18;++J)
        {
            const float A=J*2*PI/18+R.Serial*.37f;
            const float Jitter=.75f+.25f*FMath::Sin(J*17.f+R.Serial);
            FVector P=Center+FVector(FMath::Cos(A),FMath::Sin(A),0)*Age*(60+R.Amplitude*7)*Jitter;
            P.Z=Center.Z+(110+R.Amplitude*18)*Jitter*Age-490*Age*Age;
            if(P.Z<HeightAt(P+GetActorLocation())-GetActorLocation().Z)continue;
            const float Size=1.3f+R.Amplitude*.12f;
            const FVector Ring[]={FVector(Size,0,0),FVector(0,Size,0),FVector(-Size,0,0),FVector(0,-Size,0)};
            for(int32 K=0;K<4;++K)
            {
                Triangle(P+FVector(0,0,Size*2),P+Ring[K],P+Ring[(K+1)%4],.8f);
                Triangle(P-FVector(0,0,Size),P+Ring[(K+1)%4],P+Ring[K],.8f);
            }
        }
    }
    if(V.IsEmpty())
    {
        if(InteractionMesh->GetNumSections()>0)InteractionMesh->ClearAllMeshSections();
    }
    else InteractionMesh->CreateMeshSection_LinearColor(0,V,Tris,N,UV,Colors,TArray<FProcMeshTangent>(),false);
}
