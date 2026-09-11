// LIF adapted from desktop-fly Sim.swift/windows/src/sim.js (MIT).
// See SourceAssets/ThirdParty/DesktopFly for upstream notice and data license.
#include "Gameplay/FlySwarmMob.h"
#include "Async/ParallelFor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerState.h"
#include "Gameplay/CalmyCharacter.h"
#include "Gameplay/CalmyOcean.h"
#include "Core/DryHullProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "FlyConnectomeData.inl"

const FFlyConnectome& FFlyConnectome::Get()
{
    namespace D = FlyConnectomeData;
    static_assert(UE_ARRAY_COUNT(D::RowStart) == NeuronCount + 1);
    static_assert(UE_ARRAY_COUNT(D::Targets) == EdgeCount && UE_ARRAY_COUNT(D::Weights) == EdgeCount);
    static const FFlyConnectome Graph{D::RowStart, D::Targets, D::Weights, D::Baseline, D::Role, D::SensorSide, D::GroupCount};
    return Graph;
}

void FFlyLIFState::Initialize(int32 Seed)
{
    FMemory::Memzero(Potential);
    FMemory::Memzero(RefractoryMs);
    FMemory::Memzero(SpikeHistory);
    FMemory::Memzero(Rates);
    HistoryHead = 0;
    bEscape = false;
    SimulatedMs = 0;
    Random.Initialize(Seed);
}

void FFlyLIFState::Step(const FFlyConnectome& G, float LoomLeft, float LoomRight, float SensoryGain, bool bNoise)
{
    constexpr int32 N = FFlyConnectome::NeuronCount;
    constexpr float Decay = 0.9512f;
    const float InputL = FMath::Clamp(LoomLeft, 0.f, 1.f) * SensoryGain;
    const float InputR = FMath::Clamp(LoomRight, 0.f, 1.f) * SensoryGain;
    // Scratch belongs to this call, not each fly. Sum inhibition before clamping,
    // as the source does with its much larger 5*N float queue.
    float Inhibition[N] = {};
    uint16 Spiked[N];
    int32 SpikeCount = 0;
    int32 Counts[9] = {};
    for (int32 I = 0; I < N; ++I)
    {
        if (RefractoryMs[I] > 0)
        {
            --RefractoryMs[I];
            Potential[I] *= Decay;
        }
        else
        {
            Potential[I] = Potential[I] * Decay + G.Baseline[I];
            if (bNoise && Random.FRand() < 0.0022f) Potential[I] += 0.42f;
        }
        // Source also injects input into refractory cells; threshold check is gated.
        if (G.SensorSide[I] == 1) Potential[I] += InputL;
        else if (G.SensorSide[I] == 2) Potential[I] += InputR;
    }
    const uint32* Delayed = SpikeHistory[(HistoryHead + 1) % 5]; // t - 4 ms
    for (int32 Word = 0; Word < Words; ++Word)
    {
        uint32 Bits = Delayed[Word];
        while (Bits)
        {
            const int32 I = Word * 32 + FMath::CountTrailingZeros(Bits);
            Bits &= Bits - 1;
            for (uint32 K = G.RowStart[I]; K < G.RowStart[I + 1]; ++K)
                if (G.Weights[K] < 0.f) Inhibition[G.Targets[K]] += G.Weights[K];
        }
    }
    uint32* Current = SpikeHistory[HistoryHead];
    FMemory::Memzero(Current, Words * sizeof(uint32));
    for (int32 I = 0; I < N; ++I)
    {
        if (Inhibition[I] != 0.f) Potential[I] = FMath::Max(-2.f, Potential[I] + Inhibition[I]);
        if (RefractoryMs[I] == 0 && Potential[I] >= 1.f)
        {
            Potential[I] = 0.f;
            RefractoryMs[I] = 2;
            Spiked[SpikeCount++] = static_cast<uint16>(I);
            Current[I / 32] |= 1u << (I % 32);
            ++Counts[G.Role[I]];
            if (G.Role[I] == 8) bEscape = true;
        }
    }
    // Two-phase spike detection: no same-step cascades/order-dependent firing.
    for (int32 S = 0; S < SpikeCount; ++S)
    {
        const uint16 I = Spiked[S];
        for (uint32 K = G.RowStart[I]; K < G.RowStart[I + 1]; ++K)
            if (G.Weights[K] >= 0.f)
            {
                const uint16 J = G.Targets[K];
                Potential[J] = FMath::Max(-2.f, Potential[J] + G.Weights[K]);
            }
    }
    Counts[0] = SpikeCount;
    for (int32 R = 0; R < 9; ++R)
    {
        const int32 Population = R == 0 ? N : FMath::Max(1, int32(G.GroupCount[R]));
        Rates[R] += (Counts[R] * 1000.f / Population - Rates[R]) / 120.f;
    }
    HistoryHead = (HistoryHead + 1) % 5;
    ++SimulatedMs;
}

AFlySwarmMob::AFlySwarmMob()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);
    SetNetUpdateFrequency(25.f);
    FlyInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("FlyInstances"));
    SetRootComponent(FlyInstances);
    FlyInstances->SetMobility(EComponentMobility::Movable);
    FlyInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FlyInstances->SetGenerateOverlapEvents(false);
    FlyInstances->SetCanEverAffectNavigation(false);
    FlyInstances->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Placeholder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    FlyMesh = Placeholder.Object;
}

void AFlySwarmMob::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    FlyInstances->SetStaticMesh(FlyMesh);
    if (!HasActorBegunPlay())
    {
        FlyInstances->ClearInstances();
        FRandomStream SpawnRandom(RandomSeed);
        for (int32 I = 0; I < FMath::Clamp(FlyCount, 20, 200); ++I)
        {
            const FVector Offset = SpawnRandom.VRand() * (FMath::Pow(SpawnRandom.FRand(), 1.f / 3.f) * FMath::Max(1.f, SpawnRadius));
            FlyInstances->AddInstance(FTransform(FRotator(0, SpawnRandom.FRand() * 360.f, 0), Offset, FVector(FMath::Max(0.001f, MeshScale))));
        }
    }
}

void AFlySwarmMob::BeginPlay()
{
    Super::BeginPlay();
    FlyInstances->ClearInstances();
    FlyInstances->SetStaticMesh(FlyMesh);
    if (!HasAuthority()) { UpdateInstances(); return; }
    HomePosition = GetActorLocation(); Navigation.Origin = HomePosition;
    TravelRandom.Initialize(RandomSeed + 193);
    ShipOcean = ACalmyOcean::Find(GetWorld());
    TravelTarget = HomePosition;
    if (bEnableCognition) LoadMemory();
    FixedSeconds = 1. / FMath::Clamp(SimulationHz, 20, 30);
    SetNetUpdateFrequency(1.f / FixedSeconds);
    Flies.SetNum(FMath::Clamp(FlyCount, 20, 200));
    Poses.SetNum(Flies.Num());
    FRandomStream SpawnRandom(RandomSeed);
    const float Radius = IsRoamingShip() ? FMath::Clamp(ColonyRadius, 10.f, 75.f) : FMath::Min(FMath::Max(1.f, SpawnRadius), FMath::Max(1.f, RoamRadius));
    for (int32 I = 0; I < Flies.Num(); ++I)
    {
        FFlyAgentState& Fly = Flies[I];
        Fly.Brain.Initialize(static_cast<int32>(static_cast<uint32>(RandomSeed) + uint32(I) * 7919u));
        Fly.Position = GetActorLocation() + SpawnRandom.VRand() * (FMath::Pow(SpawnRandom.FRand(), 1.f / 3.f) * Radius);
        if (IsRoamingShip() && !CanFlySegment(HomePosition, Fly.Position, 3.f)) Fly.Position = HomePosition;
        Fly.Yaw = SpawnRandom.FRand() * 360.f;
        Poses[I].Position = Fly.Position;
        Poses[I].Yaw = FRotator::CompressAxisToShort(Fly.Yaw);
    }
    UpdateInstances();
    // TimerManager schedules on the game thread; HISM/UObjects are never touched on a worker.
    // Each callback advances fixed time, even if several callbacks occur in a slow frame.
    GetWorldTimerManager().SetTimer(SimulationTimer, this, &AFlySwarmMob::SimulateSwarm, FixedSeconds, true);
}

void AFlySwarmMob::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(SimulationTimer);
    if (HasAuthority() && bEnableCognition && bMemoryDirty) SaveMemory();
    Flies.Reset();
    Super::EndPlay(EndPlayReason);
}

void AFlySwarmMob::SimulateSwarm()
{
    if (!HasAuthority()) return;
    TRACE_CPUPROFILER_EVENT_SCOPE(FlySwarm_Simulate);
    const double Started = FPlatformTime::Seconds();
    if (bEnableCognition)
    {
        MindRemainder += FixedSeconds;
        if (MindRemainder >= .2f) { UpdateCognition(MindRemainder); MindRemainder = 0.f; }
    }
    if (IsRoamingShip()) UpdateTravel(static_cast<float>(FixedSeconds));
    TArray<FVector, TInlineAllocator<16>> Threats;
    TArray<float, TInlineAllocator<16>> ThreatGains;
    if (bEnableCognition)
    {
        for (const auto& O : Observations) { Threats.Add(O.Position); ThreatGains.Add(Mind.SensoryMultiplier(O.Key)); }
    }
    else for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        if (const APlayerController* PC = It->Get())
            if (const APawn* Pawn = PC->GetPawn()) { Threats.Add(Pawn->GetActorLocation()); ThreatGains.Add(1.f); }
    for (const auto& Source : DangerSources)
        if (IsValid(Source)) { Threats.Add(Source->GetActorLocation()); ThreatGains.Add(1.f); }
    if (ScatterSeconds > 0.f) { Threats.Add(SwatOrigin); ThreatGains.Add(2.f); }
    NeuralRemainderMs += FixedSeconds * 1000.;
    const int32 Steps = FMath::FloorToInt(NeuralRemainderMs + 1.e-8);
    NeuralRemainderMs -= Steps; // 30 Hz uses 33/33/34 ms, never loses fractional time.
    const float Dt = static_cast<float>(FixedSeconds);
    const FFlyConnectome& Graph = FFlyConnectome::Get();
    // Snapshot UObject-derived input on the game thread. Workers touch only their
    // own plain state/pose and read-only anatomy, threats and parameters.
    const FVector Center = GetActorLocation();
    const bool Travelling = IsRoamingShip();
    const FVector TravelVelocity = ColonyVelocity;
    TArray<FVector> OldPositions;
    if (Travelling) { OldPositions.Reserve(Flies.Num()); for (const auto& F : Flies) OldPositions.Add(F.Position); }
    const float ThreatRadius = FMath::Max(1.f, DangerRadius);
    const float Gain = FMath::Clamp(SensoryGain, 0.f, 1.f);
    const float TurnLimit = FMath::Max(0.f, MaxTurnRate);
    const float Bounds = Travelling ? FMath::Clamp(ColonyRadius, 10.f, 75.f) : FMath::Max(1.f, RoamRadius);
    const float Cruise = FMath::Max(0.f, CruiseSpeed);
    const float Escape = FMath::Max(0.f, EscapeSpeed);
    FVector IntentTarget = Center;
    float IntentSpeed = 1.f;
    if (bEnableCognition)
    {
        const FVector Attention = Mind.AttentionKey.IsEmpty() ? FVector::ZeroVector : (Mind.AttentionPosition - Center).GetSafeNormal2D();
        if (Activity == EFlyActivity::Withdraw) IntentTarget -= Attention * Bounds * .6f;
        else if (Activity == EFlyActivity::Socialize || Activity == EFlyActivity::Observe) IntentTarget += Attention * Bounds * .5f;
        else if (Activity == EFlyActivity::Explore)
            IntentTarget += FVector(FMath::Cos(Mind.LivedSeconds * .15), FMath::Sin(Mind.LivedSeconds * .15), 0) * Bounds * .5f;
        IntentSpeed = Activity == EFlyActivity::Rest ? .25f : 1.f;
    }
    const bool HasIntent = bEnableCognition;
    const bool Scatter = ScatterSeconds > 0.f;
    const FVector SwatPoint = SwatOrigin;
    FFlyAgentState* States = Flies.GetData();
    FFlySwarmPose* Output = Poses.GetData();
    ParallelFor(TEXT("FlySwarmBrains"), Flies.Num(), 8, [&](int32 I)
    {
        FFlyAgentState& Fly = States[I];
        FVector ToDanger = FVector::ZeroVector;
        float ClosestSq = FMath::Square(ThreatRadius);
        bool bDanger = false;
        float LearnedGain = 1.f;
        for (int32 T = 0; T < Threats.Num(); ++T)
        {
            const FVector Delta = Threats[T] - Fly.Position;
            if (Delta.SizeSquared() < ClosestSq)
            {
                ClosestSq = Delta.SizeSquared(); ToDanger = Delta; bDanger = true; LearnedGain = ThreatGains[T];
            }
        }
        const float Intensity = bDanger ? FMath::Clamp((1.f - FMath::Sqrt(ClosestSq) / ThreatRadius) * LearnedGain, 0.f, 1.f) : 0.f;
        const FVector Right = FRotationMatrix(FRotator(0, Fly.Yaw, 0)).GetUnitAxis(EAxis::Y);
        const float Side = FVector::DotProduct(ToDanger.GetSafeNormal(), Right);
        const float LeftInput = Intensity * (0.5f - 0.5f * Side);
        const float RightInput = Intensity * (0.5f + 0.5f * Side);
        Fly.Brain.bEscape = false;
        bool RunBrain = true;
#if !UE_BUILD_SHIPPING
        static const bool SkipBrain = FString(FCommandLine::Get()).Contains(TEXT("CalmyPerformanceProbe=")) && FParse::Param(FCommandLine::Get(), TEXT("PerfNoBrain"));
        RunBrain = !SkipBrain;
#endif
        if (RunBrain) for (int32 Ms = 0; Ms < Steps; ++Ms) Fly.Brain.Step(Graph, LeftInput, RightInput, Gain);
        if (Fly.Brain.bEscape) Fly.EscapeSeconds = 0.45f;
        else Fly.EscapeSeconds = FMath::Max(0.f, Fly.EscapeSeconds - Dt);

        // Descending population rates are command proxies, not measured muscle output.
        float Turn = FMath::Clamp((Fly.Brain.Rates[3] - Fly.Brain.Rates[2]) * 2.5f, -TurnLimit, TurnLimit);
        if (Fly.EscapeSeconds > 0.f && bDanger)
        {
            const float AwayYaw = ToDanger.IsNearlyZero() ? Fly.Yaw : (-ToDanger).Rotation().Yaw;
            Turn += FMath::Clamp(FMath::FindDeltaAngleDegrees(Fly.Yaw, AwayYaw) * 4.f, -TurnLimit, TurnLimit);
        }
        const double Phase = I * 2.39996323 + Fly.Brain.SimulatedMs * .00013;
        const float FormationRadius = Bounds * (.4f + .6f * (I % 7) / 6.f);
        const FVector Formation = Travelling ? FVector(FMath::Cos(Phase), FMath::Sin(Phase), FMath::Sin(Phase * 1.7) * .5) * FormationRadius : FVector::ZeroVector;
        const FVector Home = Center + Formation - Fly.Position;
        if (HasIntent && Fly.EscapeSeconds <= 0.f)
            Turn += FMath::Clamp(FMath::FindDeltaAngleDegrees(Fly.Yaw, (IntentTarget - Fly.Position).Rotation().Yaw), -TurnLimit * .6f, TurnLimit * .6f);
        if (Home.SizeSquared() > FMath::Square(Bounds * 0.8f))
            Turn += FMath::Clamp(FMath::FindDeltaAngleDegrees(Fly.Yaw, Home.Rotation().Yaw) * 2.f, -TurnLimit, TurnLimit);
        Fly.Yaw = FRotator::NormalizeAxis(Fly.Yaw + FMath::Clamp(Turn, -TurnLimit, TurnLimit) * Dt);
        float Speed = Cruise * IntentSpeed * FMath::Clamp(Fly.Brain.Rates[5] / 30.f, 0.f, 1.5f);
        if (Fly.Brain.Rates[4] > 25.f && Fly.Brain.Rates[4] > Fly.Brain.Rates[5]) Speed = -Speed;
        if (Fly.EscapeSeconds > 0.f) Speed = Escape;
        Fly.Velocity = FRotator(0, Fly.Yaw, 0).Vector() * Speed;
        Fly.Velocity.Z = (Fly.EscapeSeconds > 0.f ? Escape * 0.25f : 0.f) + Home.Z * 0.5f;
        if (Travelling)
        {
            // A common intention moves the whole mob. Individual biological
            // motor output adds flight variation, not 32 unrelated destinations.
            Fly.Velocity = TravelVelocity + Fly.Velocity * .2f + Home.GetClampedToMaxSize(180.f) * 2.5f;
        }
        if (Scatter)
        {
            FVector Direction = ((Fly.Position - Center).GetSafeNormal() + (Fly.Position - SwatPoint).GetSafeNormal() * .5f).GetSafeNormal();
            if (Direction.IsNearlyZero()) Direction = FRotator(0, Fly.Yaw, 0).Vector();
            Fly.Velocity = Direction * Escape * 1.8f;
            Fly.Yaw = Fly.Velocity.Rotation().Yaw;
            Fly.EscapeSeconds = FMath::Max(.6f, Fly.EscapeSeconds);
        }
        Fly.Position += Fly.Velocity * Dt;
        // Stationary laboratory swarms retain their explicit local bounds.
        const FVector Offset = Fly.Position - Center;
        if (!Travelling && Offset.SizeSquared() > FMath::Square(Bounds))
        {
            Fly.Position = Center + Offset.GetSafeNormal() * Bounds;
            Fly.Velocity = FVector::VectorPlaneProject(Fly.Velocity, Offset.GetSafeNormal());
        }
        Output[I].Position = Fly.Position;
        Output[I].Yaw = FRotator::CompressAxisToShort(Fly.Yaw);
    }); // Joins all workers before replication/HISM access or next timer callback.
    if (Travelling)
    {
        for (int32 I = 0; I < Flies.Num(); ++I)
        {
            auto& F = Flies[I]; const FVector Previous = OldPositions[I];
            FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(FlyBody), true, this);
            if (!IsOnShip(F.Position)) F.Position = Previous;
            if (GetWorld()->SweepSingleByChannel(Hit, Previous, F.Position, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(3.f), Q))
            {
                F.Position = Hit.bStartPenetrating ? Previous : Hit.Location + Hit.Normal * .5f;
                F.Velocity = FVector::VectorPlaneProject(F.Velocity, Hit.Normal);
            }
            // Follow the visible part of the colony's trail around corners;
            // never snap a separated fly through a bulkhead to its leader.
            if (!Scatter && FVector::DistSquared(F.Position, Center) > FMath::Square(90.f))
            {
                FVector Follow = Center;
                bool Clear = CanFlySegment(F.Position, Follow, 3.f);
                if (!Clear) for (int32 T = TravelTrail.Num()-1; T >= 0; --T)
                    if (CanFlySegment(F.Position, TravelTrail[T], 3.f)) { Follow = TravelTrail[T]; Clear = true; break; }
                if (Clear)
                {
                    const FVector Next = FMath::VInterpConstantTo(F.Position, Follow, Dt, Escape);
                    if (CanFlySegment(F.Position, Next, 3.f)) F.Position = Next;
                }
            }
            Poses[I].Position = F.Position;
        }
    }
    for (const FFlyAgentState& Fly : Flies) EscapeEvents += Fly.Brain.bEscape;
    ScatterSeconds = FMath::Max(0.f, ScatterSeconds - Dt); bScattering = ScatterSeconds > 0.f;
    UpdateInstances();
    ForceNetUpdate();
    LastSimulationMilliseconds = (FPlatformTime::Seconds() - Started) * 1000.;
}

void AFlySwarmMob::UpdateInstances()
{
    if (GetNetMode() == NM_DedicatedServer || !FlyMesh) return;
    InstanceTransforms.SetNum(Poses.Num());
    const FTransform Inverse = FlyInstances->GetComponentTransform().Inverse();
    for (int32 I = 0; I < Poses.Num(); ++I)
    {
        const FTransform WorldPose(FRotator(0, FRotator::DecompressAxisFromShort(Poses[I].Yaw), 0), Poses[I].Position, FVector(FMath::Max(0.001f, MeshScale)));
        InstanceTransforms[I] = WorldPose * Inverse;
    }
    if (FlyInstances->GetInstanceCount() != Poses.Num())
    {
        FlyInstances->ClearInstances();
        FlyInstances->AddInstances(InstanceTransforms, false, false);
    }
    else if (!Poses.IsEmpty()) FlyInstances->BatchUpdateInstancesTransforms(0, InstanceTransforms, false, true, true);
}

void AFlySwarmMob::OnRep_Poses() { UpdateInstances(); }
void AFlySwarmMob::OnRep_ReplicatedMovement() { Super::OnRep_ReplicatedMovement(); UpdateInstances(); }
void AFlySwarmMob::OnRep_Visuals()
{
    FlyInstances->SetStaticMesh(FlyMesh);
    if (!FlyMesh) FlyInstances->ClearInstances();
    else UpdateInstances();
}
int32 AFlySwarmMob::GetVisibleFlyCount() const { return FlyInstances->GetInstanceCount(); }

void AFlySwarmMob::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AFlySwarmMob, Poses);
    DOREPLIFETIME(AFlySwarmMob, FlyMesh);
    DOREPLIFETIME(AFlySwarmMob, MeshScale);
    DOREPLIFETIME(AFlySwarmMob, bEnableCognition);
    DOREPLIFETIME(AFlySwarmMob, Activity);
    DOREPLIFETIME(AFlySwarmMob, TravelTarget);
    DOREPLIFETIME(AFlySwarmMob, Speech);
    DOREPLIFETIME(AFlySwarmMob, SpeechSerial);
    DOREPLIFETIME(AFlySwarmMob, SwatCount); DOREPLIFETIME(AFlySwarmMob, bScattering);
    // Server is sole writer; clients/late join receive current poses without brain copies.
    // No RPCs or client intent. Disconnect removes its pawn from the next sensory sample.
}

FString AFlySwarmMob::PersonKey(const APlayerController* PC, bool& Persistent)
{
    Persistent = false;
    if (!PC) return FString();
    const APlayerState* PS = PC->GetPlayerState<APlayerState>();
    if (PS && PS->GetUniqueId().IsValid() && !PS->GetUniqueId().GetType().IsNone() && PS->GetUniqueId().GetType() != FName(TEXT("NULL")))
    { Persistent = true; return TEXT("online:") + PS->GetUniqueId().GetType().ToString() + TEXT(":") + PS->GetUniqueId().ToString(); }
    if (PC->IsLocalController() && PC == PC->GetWorld()->GetFirstPlayerController())
    { Persistent = true; return TEXT("local-host-profile"); }
    // Never use a display name or reusable PlayerId as a persistent identity.
    return FString::Printf(TEXT("session:%u"), PC->GetUniqueID());
}
bool AFlySwarmMob::AimDistance(const FVector& Origin, const FVector& Direction, float Range, float Width, float& Distance) const
{
    if (Range <= 0.f || !Direction.IsNormalized() || Origin.ContainsNaN()) return false;
    bool Found = false; Distance = Range;
    for (const auto& Pose : Poses)
    {
        const FVector Delta = FVector(Pose.Position) - Origin;
        const float Along = FVector::DotProduct(Delta, Direction);
        if (Along < 0.f || Along > Distance) continue;
        if ((Delta - Direction * Along).SizeSquared() <= FMath::Square(Width)) { Distance = Along; Found = true; }
    }
    return Found;
}
AFlySwarmMob* AFlySwarmMob::FindAimedSwarm(const APlayerController* PC)
{
    const auto* P = PC ? Cast<ACalmyCharacter>(PC->GetPawn()) : nullptr;
    if (!P || !P->IsControlled() || P->HeldCargo) return nullptr;
    const FVector Origin = P->GetInteractionOrigin(), Direction = P->GetBaseAimRotation().Vector();
    float Range = 400.f; FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(FlyAim), true, P);
    if (PC->GetWorld()->LineTraceSingleByChannel(Hit, Origin, Origin + Direction * Range, ECC_Visibility, Q)) Range = Hit.Distance;
    AFlySwarmMob* Best = nullptr;
    for (TActorIterator<AFlySwarmMob> It(PC->GetWorld()); It; ++It)
    {
        float Distance;
        if (It->CanConverse(PC) && It->AimDistance(Origin, Direction, Range, 55.f, Distance)) { Best = *It; Range = Distance; }
    }
    return Best;
}
bool AFlySwarmMob::ReceiveSwat(ACalmyCharacter* Attacker)
{
    if (!HasAuthority() || !IsValid(Attacker) || Attacker->GetWorld() != GetWorld() || !Attacker->IsControlled()) return false;
    SwatOrigin = Attacker->GetInteractionOrigin(); ScatterSeconds = .85f; bScattering = true; ++SwatCount;
    if (bEnableCognition)
    {
        if (const auto* PC = Cast<APlayerController>(Attacker->GetController()))
        {
            bool Persistent; const FString Key = PersonKey(PC, Persistent);
            auto& Person = Mind.Remember(Key, Persistent);
            Person.LastSeen = Mind.LivedSeconds; Person.Threats = FMath::Min(1000000, Person.Threats + 1);
            Person.ExpectedThreat = 1.f; Person.Trust = FMath::Max(-1.f, Person.Trust - .45f);
            if (Mind.Episodes.Num() >= 32) Mind.Episodes.RemoveAt(0);
            auto& Event = Mind.Episodes.AddDefaulted_GetRef(); Event.Person = Key; Event.Event = EFlyUtterance::Threaten; Event.Time = Mind.LivedSeconds;
            Mind.AttentionKey = Key; Mind.AttentionPosition = SwatOrigin;
        }
        Mind.Alarm = 1.f; Mind.Activity = EFlyActivity::Withdraw; Mind.ActivitySeconds = 0.f; Activity = Mind.Activity;
        Speech = TEXT("Stay back. I remember what you did."); ++SpeechSerial; SpeechSeconds = 12.f; SilenceSeconds = 0.f; bMemoryDirty = true;
    }
    ForceNetUpdate(); return true;
}
bool AFlySwarmMob::CanConverse(const APlayerController* PC) const
{
    if (!bEnableCognition || !IsValid(PC) || !IsValid(PC->GetPawn()) || PC->GetWorld() != GetWorld()) return false;
    if (const auto* C = Cast<ACalmyCharacter>(PC->GetPawn()))
        if (C->Health <= 0 || C->Lifecycle != ECalmyPlayerLifecycleState::Controlled) return false;
    if (FVector::DistSquared(PC->GetPawn()->GetActorLocation(), GetActorLocation()) > FMath::Square(400.f)) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(FlyTalk), true, this); Params.AddIgnoredActor(PC->GetPawn());
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByChannel(Hit, PC->GetPawn()->GetPawnViewLocation(), GetActorLocation(), ECC_Visibility, Params);
}
AFlySwarmMob* AFlySwarmMob::FindConversationalSwarm(const APlayerController* PC)
{
    if (!PC || !PC->GetWorld() || !PC->GetPawn()) return nullptr;
    AFlySwarmMob* Best = nullptr; double Distance = DBL_MAX;
    for (TActorIterator<AFlySwarmMob> It(PC->GetWorld()); It; ++It)
        if (It->CanConverse(PC))
        {
            const double D = FVector::DistSquared(It->GetActorLocation(), PC->GetPawn()->GetActorLocation());
            if (D < Distance) { Distance = D; Best = *It; }
        }
    return Best;
}
bool AFlySwarmMob::ReceiveUtterance(APlayerController* PC, EFlyUtterance Intent)
{
    if (!HasAuthority() || uint8(Intent) > 3 || !CanConverse(PC)) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (const double* Previous = LastConversations.Find(PC)) if (Now - *Previous < 4.) return false;
    LastConversations.Add(PC, Now);
    bool Persistent; const FString Key = PersonKey(PC, Persistent);
    Speech = Mind.Converse(Key, Persistent, Intent); ++SpeechSerial; SpeechSeconds = 12.f; SilenceSeconds = 0.f;
    Mind.AttentionPosition = PC->GetPawn()->GetActorLocation(); Mind.AttentionKey = Key;
    Activity = Mind.Activity; bMemoryDirty = true; ForceNetUpdate(); return true;
}
void AFlySwarmMob::UpdateCognition(float Dt)
{
    Observations.Reset();
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get(); if (!PC || !PC->GetPawn()) continue;
        const APawn* Pawn = PC->GetPawn();
        const float Distance = FVector::Dist(Pawn->GetActorLocation(), GetActorLocation());
        if (Distance > 650.f) continue;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(FlyPerception), true, this); Params.AddIgnoredActor(Pawn);
        FHitResult Hit;
        if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), Pawn->GetPawnViewLocation(), ECC_Visibility, Params)) continue;
        FFlyObservation& O = Observations.AddDefaulted_GetRef();
        O.Key = PersonKey(PC, O.bPersistent); O.Position = Pawn->GetActorLocation();
        O.Proximity = 1.f - Distance / 650.f;
        const FVector TowardNest = (GetActorLocation() - O.Position).GetSafeNormal();
        O.Startle = Distance < 300.f ? FMath::Clamp(FVector::DotProduct(Pawn->GetVelocity(), TowardNest) / 280.f, 0.f, 1.f) : 0.f;
    }
    float NeuralAlarm = 0.f;
    for (const auto& Fly : Flies) NeuralAlarm += Fly.EscapeSeconds > 0.f ? 1.f : 0.f;
    NeuralAlarm /= FMath::Max(1, Flies.Num());
    Mind.Step(Dt, Observations, NeuralAlarm); Activity = Mind.Activity; bMemoryDirty = true;
    SpeechSeconds -= Dt; SilenceSeconds += Dt;
    if (SpeechSeconds <= 0.f) Speech.Reset();
    if (!Observations.IsEmpty() && SilenceSeconds >= 20.f)
    { Speech = Mind.DescribeActivity(); ++SpeechSerial; SpeechSeconds = 10.f; SilenceSeconds = 0.f; }
    for (auto It = LastConversations.CreateIterator(); It; ++It) if (!It.Key().IsValid()) It.RemoveCurrent();
    SaveSeconds += Dt;
    if (SaveSeconds >= 30.f) { SaveSeconds = 0.f; if (!MemorySlot().IsEmpty()) SaveMemory(); }
}
bool AFlySwarmMob::IsOnShip(const FVector& Position) const
{
    if (Position.ContainsNaN()) return false;
    const auto* Ocean = ShipOcean.Get();
    if (!Ocean || Ocean->DryHullStations.Num() < 2 || Ocean->DryHullVertical.Num() < 2)
        return FVector::DistSquared(Position, HomePosition) <= FMath::Square(FMath::Max(1.f, RoamRadius));
    const FVector Local = Position - Ocean->DryHullOrigin;
    const double Beam = FCalmyDryHullProfile::Sample(Ocean->DryHullStations, Local.X);
    const double Bottom = Ocean->DryHullVertical[0].X, Top = Ocean->DryHullVertical.Last().X;
    if (Local.Z < Bottom + 30 || Local.Z > Top + 1100 || Beam < 80 || FMath::Abs(Local.Y) > Beam - 50) return false;
    return Local.Z >= Top - 30 || Ocean->IsInsideDryHull(Position);
}
bool AFlySwarmMob::CanFlySegment(const FVector& From, const FVector& To, float Radius) const
{
    if (!GetWorld() || !IsOnShip(To) || From.ContainsNaN()) return false;
    FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(FlyTravel), true, this);
    return !GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(Radius), Q);
}
void AFlySwarmMob::UpdateTravel(float Dt)
{
    ColonyVelocity = FVector::ZeroVector;
    const FVector Center = GetActorLocation();
    TravelWait = FMath::Max(0.f, TravelWait - Dt);
    Navigation.Visit(Center, Mind.LivedSeconds);
    if (Activity != PlannedActivity)
    {
        Navigation.Path.Reset(); bPlanning = false; TravelWait = 0; PlannedActivity = Activity;
    }
    if (Activity == EFlyActivity::Rest || ScatterSeconds > 0.f)
    { Navigation.Path.Reset(); bPlanning = false; TravelTarget = Center; return; }
    if (TravelWait > 0.f) return;
    // Briefly stay available while being addressed. A threat overrides courtesy.
    if (Activity != EFlyActivity::Withdraw && SpeechSeconds > 8.f && !Observations.IsEmpty()) return;
    if (Navigation.Path.IsEmpty() && !bPlanning)
    {
        FVector Desired = TravelRandom.VRand(); Desired.Z = FMath::Abs(Desired.Z) * .65f;
        // Above the main deck, explore both higher and lower decks.
        if (Center.Z > HomePosition.Z + 650.f) Desired.Z = TravelRandom.FRandRange(-.7f, .4f);
        Desired.Normalize(); bool Directed = false;
        if (!Mind.AttentionKey.IsEmpty())
        {
            const FVector Away = (Center - Mind.AttentionPosition).GetSafeNormal();
            if (Activity == EFlyActivity::Withdraw) { Desired = Center + Away * 900.f + FVector(0,0,120); Directed = true; }
            else if (Activity == EFlyActivity::Socialize || Activity == EFlyActivity::Observe)
            {
                Desired = Mind.AttentionPosition + Away * (Activity == EFlyActivity::Observe ? 260.f : 170.f) + FVector(0,0,60);
                if (FVector::Dist(Center, Desired) < 75.f) { TravelTarget = Center; TravelWait = 1.f; return; }
                Directed = true;
            }
        }
        Navigation.Begin(Center, Desired, Directed, Mind.LivedSeconds); bPlanning = true;
    }
    if (bPlanning)
    {
        if (!Navigation.Advance([this](const FVector& A, const FVector& B) { return CanFlySegment(A, B, 18.f); })) return;
        bPlanning = false;
        if (Navigation.Path.IsEmpty()) { TravelWait = 1.f; TravelTarget = Center; return; }
        TravelTarget = Navigation.Path.Last();
    }
    const FVector Next = FMath::VInterpConstantTo(Center, Navigation.Path[0], Dt,
        FMath::Clamp(ColonySpeed, 20.f, 250.f) * (Activity == EFlyActivity::Withdraw ? 1.65f : 1.f));
    if (!CanFlySegment(Center, Next, 18.f))
    { Navigation.Path.Reset(); TravelWait = .4f; return; } // Closing door: stop, then search current geometry again.
    SetActorLocation(Next); ColonyVelocity = (Next - Center) / Dt;
    if (TravelTrail.IsEmpty() || FVector::Dist(TravelTrail.Last(), Next) >= 20.f)
    { if (TravelTrail.Num() >= 96) TravelTrail.RemoveAt(0); TravelTrail.Add(Next); }
    if (FVector::DistSquared(Next, Navigation.Path[0]) < 1.f) Navigation.Path.RemoveAt(0);
}
FString AFlySwarmMob::MemorySlot() const
{
    if (PersistentMemoryId.IsNone()) return FString();
    FString Prefix = TEXT("FlyMind_");
#if !UE_BUILD_SHIPPING
    // All probes are isolated from the user's saved resident.
    if (FString(FCommandLine::Get()).Contains(TEXT("Probe=")))
    {
        FString TestSlot;
        if (!FParse::Value(FCommandLine::Get(), TEXT("FlyMindTestSlot="), TestSlot)) return FString();
        if (TestSlot.Len() > 64 || TestSlot.Contains(TEXT("/")) || TestSlot.Contains(TEXT("\\")) || TestSlot.Contains(TEXT(".."))) return FString();
        Prefix = TEXT("FlyMindTest_") + TestSlot + TEXT("_");
    }
#endif
    return Prefix + PersistentMemoryId.ToString();
}
bool AFlySwarmMob::SaveMemory()
{
    if (!HasAuthority() || !bEnableCognition || MemorySlot().IsEmpty()) return false;
    auto* Save = Cast<UFlyCognitionSave>(UGameplayStatics::CreateSaveGameObject(UFlyCognitionSave::StaticClass()));
    Save->Mind = Mind; Save->Mind.Sanitize(true);
    const bool OK = UGameplayStatics::SaveGameToSlot(Save, MemorySlot(), 0);
    if (OK) bMemoryDirty = false;
    else UE_LOG(LogTemp, Warning, TEXT("Fly mind save failed: %s"), *MemorySlot());
    return OK;
}
bool AFlySwarmMob::LoadMemory()
{
    if (!HasAuthority() || !bEnableCognition || MemorySlot().IsEmpty() || !UGameplayStatics::DoesSaveGameExist(MemorySlot(), 0)) return false;
    if (const auto* Save = Cast<UFlyCognitionSave>(UGameplayStatics::LoadGameFromSlot(MemorySlot(), 0)))
        if (Save->Version == 1) { Mind = Save->Mind; Mind.Sanitize(true); return true; }
    UE_LOG(LogTemp, Warning, TEXT("Fly mind save could not be loaded: %s"), *MemorySlot()); return false;
}
