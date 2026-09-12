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
#include "CollisionQueryParams.h"
#include "Engine/HitResult.h"
#include "FlyConnectomeData.inl"

const FFlyConnectome& FFlyConnectome::Get()
{
    namespace D = FlyConnectomeData;
    static_assert(UE_ARRAY_COUNT(D::RowStart) == NeuronCount + 1);
    static_assert(UE_ARRAY_COUNT(D::Targets) == EdgeCount && UE_ARRAY_COUNT(D::Weights) == EdgeCount);
    static const FFlyConnectome Graph{
        D::RowStart, D::Targets, D::Weights, D::Baseline, D::Role, D::SensorSide, D::GroupCount,
        D::Sensory, D::Ascending,
        static_cast<uint16>(UE_ARRAY_COUNT(D::Sensory)),
        static_cast<uint16>(UE_ARRAY_COUNT(D::Ascending))
    };
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
    AscendPhase0 = Random.FRand() * 2.f * PI;
    BurstUntilMs = 0;
    BurstNextMs = 15000ull + uint64(Random.RandRange(0, 25000));
}

void FFlyLIFState::Step(const FFlyConnectome& G, float LoomLeft, float LoomRight, float SensoryGain, bool bNoise, float AirPuff, float AscendDrive)
{
    FFlyNeuralInputs In;
    In.LoomLeft = LoomLeft; In.LoomRight = LoomRight; In.SensoryGain = SensoryGain;
    In.bNoise = bNoise; In.AirPuff = AirPuff; In.AscendBias = AscendDrive;
    Step(G, In);
}

void FFlyLIFState::Step(const FFlyConnectome& G, const FFlyNeuralInputs& In)
{
    constexpr int32 N = FFlyConnectome::NeuronCount;
    constexpr float Decay = 0.9512f;
    const float Gate = FMath::Clamp(In.SensoryGate, 0.f, 1.5f);
    const float Scale = FMath::Clamp(In.ActivityScale, 0.05f, 1.5f);
    const float LoomGain = FMath::Clamp(In.SensoryGain, 0.f, 1.f) * Gate;
    const float InputL = FMath::Clamp(In.LoomLeft, 0.f, 1.f) * LoomGain;
    const float InputR = FMath::Clamp(In.LoomRight, 0.f, 1.f) * LoomGain;
    const float Puff = FMath::Clamp(In.AirPuff, 0.f, 1.f) * 0.12f * Gate;
    const float Gait = FMath::Clamp(In.GaitDrive, 0.f, 1.5f);
    const float Bias = FMath::Clamp(In.AscendBias, 0.f, 1.f) * 0.09f;
    const float Phase = In.GaitPhase * 2.f * PI;
    // Scratch belongs to this call, not each fly. Sum inhibition before clamping,
    // as the source does with its much larger 5*N float queue.
    float Inhibition[N] = {};
    uint16 Spiked[N];
    int32 SpikeCount = 0;
    int32 Counts[9] = {};
    if (In.bNoise && SimulatedMs >= BurstNextMs)
    {
        BurstUntilMs = SimulatedMs + 400;
        BurstNextMs = SimulatedMs + 15000ull + uint64(Random.RandRange(0, 25000));
    }
    const float NoiseP = In.bNoise
        ? ((SimulatedMs < BurstUntilMs) ? 0.0022f * 6.f : 0.0022f) * Scale
        : 0.f;
    for (int32 I = 0; I < N; ++I)
    {
        if (RefractoryMs[I] > 0)
        {
            --RefractoryMs[I];
            Potential[I] *= Decay;
        }
        else
        {
            Potential[I] = Potential[I] * Decay + G.Baseline[I] * Scale;
            if (NoiseP > 0.f && Random.FRand() < NoiseP) Potential[I] += 0.42f;
        }
        // Source also injects input into refractory cells; threshold check is gated.
        if (G.SensorSide[I] == 1) Potential[I] += InputL;
        else if (G.SensorSide[I] == 2) Potential[I] += InputR;
    }
    if (Puff > 0.001f && G.Sensory && G.SensoryCount > 0)
        for (uint16 S = 0; S < G.SensoryCount; ++S) Potential[G.Sensory[S]] += Puff;
    if ((Gait > 0.001f || Bias > 0.001f) && G.Ascending && G.AscendingCount > 0)
    {
        for (uint16 S = 0; S < G.AscendingCount; ++S)
        {
            const float Rhythm = Gait > 0.001f
                ? Gait * 0.09f * (0.5f + 0.5f * FMath::Sin(Phase + AscendPhase0 + float(S) * 2.39996323f))
                : 0.f;
            Potential[G.Ascending[S]] += Rhythm + Bias;
        }
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
    const float Cruise = FMath::Max(0.f, CruiseSpeed);
    const float Escape = FMath::Max(0.f, EscapeSpeed);
    float SensoryGate = 1.f;
    float ActivityScale = 1.f;
    float GaitDrive = 0.f;
    float MotionPuff = 0.f;
    float MarkAir = 0.f;
    float MarkAscend = 0.f;
    float MarkGain = 1.f;
    if (bEnableCognition)
    {
        MarkGain = Mind.InscriptionSensoryGain();
        MarkAir = Mind.InscriptionAirPuff();
        MarkAscend = Mind.InscriptionAscendDrive();
        for (const auto& O : Observations) MotionPuff = FMath::Max(MotionPuff, O.Startle);
        // desktop-fly sleep/arousal gates mapped from colony activity.
        switch (Activity)
        {
        case EFlyActivity::Rest: SensoryGate = .35f; ActivityScale = .5f; break;
        case EFlyActivity::Withdraw: SensoryGate = 1.15f; ActivityScale = 1.1f; break;
        case EFlyActivity::Observe: SensoryGate = 1.05f; ActivityScale = 1.f; break;
        case EFlyActivity::Socialize: SensoryGate = 1.f; ActivityScale = 1.05f; break;
        case EFlyActivity::Explore: SensoryGate = 1.f; ActivityScale = 1.f; break;
        default: checkNoEntry(); break;
        }
        GaitDrive = FMath::Clamp(ColonyVelocity.Size2D() / FMath::Max(1.f, Cruise), 0.f, 1.25f);
        if (Activity == EFlyActivity::Rest) GaitDrive *= .12f;
        else if (Activity == EFlyActivity::Socialize) GaitDrive = FMath::Max(GaitDrive, .4f);
        else if (Activity == EFlyActivity::Withdraw) GaitDrive = FMath::Max(GaitDrive, .7f);
        ColonyGaitPhase = FMath::Fmod(ColonyGaitPhase + Dt * (.75f + GaitDrive), 1.f);
        if (ColonyGaitPhase < 0.f) ColonyGaitPhase += 1.f;
    }
    const float ContextGain = FMath::Clamp(Gain * MarkGain, 0.05f, 1.f);
    const float Vigilance = bEnableCognition ? Mind.InscriptionCaution * .2f : 0.f;
    const float AirPuff = FMath::Clamp(FMath::Max(MarkAir, MotionPuff), 0.f, 1.f);
    const float GaitPhase = ColonyGaitPhase;
    const float TurnLimit = FMath::Max(0.f, MaxTurnRate);
    const float Bounds = Travelling ? FMath::Clamp(ColonyRadius, 10.f, 75.f) : FMath::Max(1.f, RoamRadius);
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
    const bool Resting = WantsDeckRest();
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
        const float LeftInput = FMath::Clamp(Intensity * (0.5f - 0.5f * Side) + Vigilance, 0.f, 1.f);
        const float RightInput = FMath::Clamp(Intensity * (0.5f + 0.5f * Side) + Vigilance, 0.f, 1.f);
        Fly.Brain.bEscape = false;
        bool RunBrain = true;
#if !UE_BUILD_SHIPPING
        static const bool SkipBrain = FString(FCommandLine::Get()).Contains(TEXT("CalmyPerformanceProbe=")) && FParse::Param(FCommandLine::Get(), TEXT("PerfNoBrain"));
        RunBrain = !SkipBrain;
#endif
        if (RunBrain)
        {
            FFlyNeuralInputs Drive;
            Drive.LoomLeft = LeftInput;
            Drive.LoomRight = RightInput;
            Drive.SensoryGain = ContextGain;
            Drive.AirPuff = AirPuff;
            Drive.GaitDrive = GaitDrive;
            Drive.GaitPhase = GaitPhase;
            Drive.AscendBias = MarkAscend;
            Drive.SensoryGate = SensoryGate;
            Drive.ActivityScale = ActivityScale;
            Drive.bNoise = true;
            for (int32 Ms = 0; Ms < Steps; ++Ms) Fly.Brain.Step(Graph, Drive);
        }
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
        // Rest flattens the cloud onto the deck; flight keeps the proven R6 vertical band.
        const float Vertical = Resting ? .08f : .5f;
        const FVector Formation = Travelling ? FVector(FMath::Cos(Phase), FMath::Sin(Phase), FMath::Sin(Phase * 1.7) * Vertical) * FormationRadius : FVector::ZeroVector;
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
        const float Climb = Fly.EscapeSeconds > 0.f ? Escape * 0.25f : 0.f;
        const float HomeLift = Resting && Fly.EscapeSeconds <= 0.f ? .15f : .5f;
        Fly.Velocity.Z = Climb + Home.Z * HomeLift;
        if (Travelling)
        {
            // A common intention moves the whole mob. Individual biological
            // motor output adds flight variation, not 32 unrelated destinations.
            Fly.Velocity = TravelVelocity + Fly.Velocity * .35f + Home.GetClampedToMaxSize(180.f) * 2.5f;
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
            // Deck pin only while resting; flight altitude is owned by the colony path.
            if (Resting) PinFlyAltitude(F, I, Scatter);
            Poses[I].Position = F.Position;
        }
    }
    else
    {
        for (int32 I = 0; I < Flies.Num(); ++I)
        {
            PinFlyAltitude(Flies[I], I, Scatter);
            Poses[I].Position = Flies[I].Position;
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
    DOREPLIFETIME(AFlySwarmMob, bTravelPlanning);
    DOREPLIFETIME(AFlySwarmMob, Speech);
    DOREPLIFETIME(AFlySwarmMob, SpeechSerial);
    DOREPLIFETIME(AFlySwarmMob, VisibleInscriptions);
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
    // Match perception range so spontaneous speech stays readable on approach.
    if (FVector::DistSquared(PC->GetPawn()->GetActorLocation(), GetActorLocation()) > FMath::Square(650.f)) return false;
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
void AFlySwarmMob::PublishInscriptions()
{
    VisibleInscriptions.Reset();
    VisibleInscriptions.Reserve(Mind.Inscriptions.Num());
    for (const auto& Mark : Mind.Inscriptions)
    {
        FFlyInscriptionView& View = VisibleInscriptions.AddDefaulted_GetRef();
        View.Id = Mark.Id; View.Kind = Mark.Kind; View.Location = Mark.Location; View.Echoes = Mark.Echoes;
    }
}
bool AFlySwarmMob::ReceiveInscription(APlayerController* PC, EFlyMessageKind Kind)
{
    if (!HasAuthority() || uint8(Kind) > uint8(EFlyMessageKind::SeekContact) || !CanConverse(PC)) return false;
    const double Now = GetWorld()->GetTimeSeconds();
    if (const double* Previous = LastConversations.Find(PC)) if (Now - *Previous < 4.) return false;
    LastConversations.Add(PC, Now);
    bool Persistent; const FString Key = PersonKey(PC, Persistent);
    const FVector MarkAt = PC->GetPawn()->GetActorLocation();
    Speech = Mind.Inscribe(Key, Persistent, Kind, MarkAt); ++SpeechSerial; SpeechSeconds = 12.f; SilenceSeconds = 0.f;
    Mind.AttentionPosition = MarkAt; Mind.AttentionKey = Key;
    Activity = Mind.Activity; PublishInscriptions(); bMemoryDirty = true; ForceNetUpdate(); return !Speech.IsEmpty();
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
    const EFlyActivity Previous = Mind.Activity;
    Mind.Step(Dt, Observations, NeuralAlarm); Activity = Mind.Activity; bMemoryDirty = true;
    SpeechSeconds -= Dt; SilenceSeconds += Dt;
    if (SpeechSeconds <= 0.f) Speech.Reset();
    // Announce activity changes immediately so presence is readable; repeats stay on the long cadence.
    if (Activity != Previous)
    {
        Speech = Mind.DescribeActivity(); ++SpeechSerial; SpeechSeconds = 10.f; SilenceSeconds = 0.f;
    }
    else if (!Observations.IsEmpty() && SilenceSeconds >= 20.f)
    {
        Speech = Mind.DescribeActivity(); ++SpeechSerial; SpeechSeconds = 10.f; SilenceSeconds = 0.f;
    }
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
    const FVector Local = Ocean->WorldToDryHullLocal(Position);
    const double Beam = FCalmyDryHullProfile::Sample(Ocean->DryHullStations, Local.X);
    const double Bottom = Ocean->DryHullVertical[0].X, Top = Ocean->DryHullVertical.Last().X;
    if (Local.Z < Bottom + 30 || Local.Z > Top + 1100 || Beam < 80 || FMath::Abs(Local.Y) > Beam - 50) return false;
    return Local.Z >= Top - 30 || Ocean->IsInsideDryHull(Position);
}
bool AFlySwarmMob::FindDeckBelow(const FVector& Point, float& OutFloorZ) const
{
    if (!GetWorld() || Point.ContainsNaN()) return false;
    FHitResult Hit;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(FlyDeck), false, this);
    const FVector Start(Point.X, Point.Y, Point.Z + 150.f);
    const FVector End(Point.X, Point.Y, Point.Z - 1400.f);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q) || !Hit.bBlockingHit) return false;
    // Need a walkable deck/top surface, not a bulkhead.
    if (Hit.ImpactNormal.Z < .65f) return false;
    OutFloorZ = Hit.ImpactPoint.Z;
    return true;
}
bool AFlySwarmMob::WantsDeckRest() const
{
    return bEnableCognition && Activity == EFlyActivity::Rest && ScatterSeconds <= 0.f;
}
void AFlySwarmMob::PinFlyAltitude(FFlyAgentState& Fly, int32 Index, bool bScatter) const
{
    (void)Index;
    // Escape/scatter keep full 3D motor output.
    if (bScatter || Fly.EscapeSeconds > 0.f) return;
    float FloorZ = 0.f;
    // Colony leader samples the deck once so individuals do not dive through grate holes
    // onto a lower deck found under their own XY.
    const FVector Sample = IsRoamingShip() ? GetActorLocation() : Fly.Position;
    if (!FindDeckBelow(Sample, FloorZ)) return;
    const bool OnDeck = WantsDeckRest();
    // During Rest the colony sits on deck regardless of residual wing rates.
    float TargetZ = FloorZ + (OnDeck ? WalkClearanceCm : FlightHeightCm);
    // Reject absurd floor samples (openings that see a deck far below).
    if (FMath::Abs(TargetZ - Fly.Position.Z) > FlightHeightCm + 80.f) return;
    constexpr float MaxStepCm = 6.f;
    float SoftZ = FMath::FInterpTo(Fly.Position.Z, TargetZ, .033f, 6.f);
    SoftZ = FMath::Clamp(SoftZ, Fly.Position.Z - MaxStepCm, Fly.Position.Z + MaxStepCm);
    FVector Next = Fly.Position;
    Next.Z = SoftZ;
    if (FMath::Abs(Next.Z - Fly.Position.Z) < .05f) return;
    if (CanFlySegment(Fly.Position, Next, 3.f))
    {
        Fly.Position = Next;
        Fly.Velocity.Z *= .15f;
    }
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
        Navigation.Path.Reset(); bPlanning = false; bTravelPlanning = false; TravelWait = 0; PlannedActivity = Activity;
    }
    float FloorZ = Center.Z - FlightHeightCm;
    const bool HaveFloor = FindDeckBelow(Center, FloorZ);
    float AirZ = HaveFloor ? FloorZ + FlightHeightCm : Center.Z;
    if (HaveFloor)
    {
        float HomeFloor = 0.f;
        // On the home deck keep the authored spawn band so HOLD corridors stay reachable.
        if (FindDeckBelow(HomePosition, HomeFloor) && FMath::Abs(FloorZ - HomeFloor) < 50.f)
            AirZ = FMath::Max(AirZ, HomePosition.Z);
    }
    const float ClimbZ = AirZ + 35.f;
    const float RestZ = HaveFloor ? FloorZ + WalkClearanceCm : Center.Z;
    if (Activity == EFlyActivity::Rest || ScatterSeconds > 0.f)
    {
        Navigation.Path.Reset(); bPlanning = false; bTravelPlanning = false; TravelTarget = Center;
        if (HaveFloor && Activity == EFlyActivity::Rest)
        {
            FVector Settled = Center;
            Settled.Z = FMath::Clamp(FMath::FInterpTo(Center.Z, RestZ, Dt, 8.f), Center.Z - 6.f, Center.Z + 6.f);
            if (CanFlySegment(Center, Settled, 18.f) && IsOnShip(Settled))
            {
                SetActorLocation(Settled);
                ColonyVelocity = (Settled - Center) / Dt;
            }
        }
        return;
    }
    if (TravelWait > 0.f) return;
    // Briefly stay available while being addressed. A threat overrides courtesy.
    // Only E-wheel / swat replies should park the colony; activity announcements do not.
    if (Activity != EFlyActivity::Withdraw && SpeechSeconds > 11.f && !Observations.IsEmpty()) return;
    if (Navigation.Path.IsEmpty() && !bPlanning)
    {
        bool Directed = false, TowardMark = false, FleeMark = false;
        // Hold station when already at observe/socialize standoff.
        if (!Mind.AttentionKey.IsEmpty() && (Activity == EFlyActivity::Observe || Activity == EFlyActivity::Socialize))
        {
            const float Standoff = Activity == EFlyActivity::Observe ? 300.f : 140.f;
            const float Hold = Activity == EFlyActivity::Observe ? 90.f : 55.f;
            const FVector Away = (Center - Mind.AttentionPosition).GetSafeNormal();
            const FVector HoldAt = Mind.AttentionPosition + Away * Standoff;
            if (FVector::Dist(Center, HoldAt) < Hold)
            { TravelTarget = Center; TravelWait = Activity == EFlyActivity::Observe ? 1.6f : .7f; return; }
        }
        const FVector Desired = ChooseTravelDesire(Center, AirZ, ClimbZ, Directed, TowardMark, FleeMark);
        if (SilenceSeconds > 5.f)
        {
            Speech = Mind.DescribeTravelThought(true, TowardMark, FleeMark);
            ++SpeechSerial; SpeechSeconds = 8.f; SilenceSeconds = 0.f;
        }
        // Incremental Advance is the visible “thinking”; do not stall travel with an extra wait.
        Navigation.Begin(Center, Desired, Directed, Mind.LivedSeconds);
        bPlanning = true; bTravelPlanning = true; ForceNetUpdate();
    }
    if (bPlanning)
    {
        if (!Navigation.Advance([this](const FVector& A, const FVector& B) { return CanFlySegment(A, B, 18.f); })) return;
        bPlanning = false; bTravelPlanning = false;
        if (Navigation.Path.IsEmpty()) { TravelWait = 1.f; TravelTarget = Center; ForceNetUpdate(); return; }
        TravelTarget = Navigation.Path.Last();
        if (SilenceSeconds > 4.f)
        {
            const bool TowardMark = Mind.InscriptionWelcome > .35f;
            const bool FleeMark = Mind.InscriptionCaution > .35f && Activity != EFlyActivity::Socialize;
            Speech = Mind.DescribeTravelThought(false, TowardMark, FleeMark);
            ++SpeechSerial; SpeechSeconds = 9.f; SilenceSeconds = 0.f;
        }
        ForceNetUpdate();
    }
    // Proven R6 motion: follow the path in 3D. Soft altitude is encoded in path goals,
    // not a per-tick Z snap that can tunnel deck openings.
    const FVector Next = FMath::VInterpConstantTo(Center, Navigation.Path[0], Dt,
        FMath::Clamp(ColonySpeed, 20.f, 250.f)
            * (Activity == EFlyActivity::Withdraw ? 1.65f : Activity == EFlyActivity::Observe ? .55f : Activity == EFlyActivity::Socialize ? .85f : 1.f));
    if (!CanFlySegment(Center, Next, 18.f))
    { Navigation.Path.Reset(); TravelWait = .4f; bTravelPlanning = false; return; }
    SetActorLocation(Next); ColonyVelocity = (Next - Center) / Dt;
    if (TravelTrail.IsEmpty() || FVector::Dist(TravelTrail.Last(), Next) >= 20.f)
    { if (TravelTrail.Num() >= 96) TravelTrail.RemoveAt(0); TravelTrail.Add(Next); }
    if (FVector::DistSquared(Next, Navigation.Path[0]) < 1.f) Navigation.Path.RemoveAt(0);
}
FVector AFlySwarmMob::ChooseTravelDesire(const FVector& Center, float AirZ, float ClimbZ, bool& bDirected, bool& bTowardMark, bool& bFleeMark)
{
    bDirected = false; bTowardMark = false; bFleeMark = false;
    if (!Mind.AttentionKey.IsEmpty())
    {
        const FVector Away = (Center - Mind.AttentionPosition).GetSafeNormal();
        if (Activity == EFlyActivity::Withdraw)
        {
            bDirected = true;
            FVector Desired = Center + Away * 900.f; Desired.Z = ClimbZ; return Desired;
        }
        if (Activity == EFlyActivity::Socialize || Activity == EFlyActivity::Observe)
        {
            const float Standoff = Activity == EFlyActivity::Observe ? 300.f : 140.f;
            FVector Desired = Mind.AttentionPosition + Away * Standoff;
            Desired.Z = AirZ; bDirected = true; return Desired;
        }
    }
    // Proven R6 roam length; mind only nudges heading (marks / novelty), soft.
    FVector Dir = TravelRandom.VRand(); Dir.Z = FMath::Abs(Dir.Z) * .65f;
    if (Center.Z > HomePosition.Z + 650.f) Dir.Z = TravelRandom.FRandRange(-.7f, .4f);
    Dir.Normalize();
    FVector Nudge = FVector::ZeroVector;
    for (const auto& Mark : Mind.Inscriptions)
    {
        const FVector To = Mark.Location - Center;
        const float Dist = To.Size();
        if (Dist < 1.f || Dist > 1400.f) continue;
        const FVector N = To / Dist;
        const float W = 1.f + Mark.Echoes * .25f;
        if (Mark.Kind == EFlyMessageKind::BeWary && Mind.InscriptionCaution > .2f)
        { Nudge -= N * (55.f * W * Mind.InscriptionCaution); bFleeMark = true; }
        if ((Mark.Kind == EFlyMessageKind::SafeHere || Mark.Kind == EFlyMessageKind::SeekContact || Mark.Kind == EFlyMessageKind::PraiseColony)
            && Mind.InscriptionWelcome > .2f)
        { Nudge += N * (45.f * W * Mind.InscriptionWelcome); bTowardMark = true; }
    }
    if (const float* Seen = Navigation.LastVisit.Find(Navigation.Cell(Center + Dir * 900.f)))
        if (Mind.LivedSeconds - *Seen < 40.f) Dir = (Dir + TravelRandom.VRand() * .35f).GetSafeNormal();
    FVector Desired = Center + Dir * 900.f + Nudge.GetClampedToMaxSize(180.f);
    Desired.Z = AirZ;
    return Desired;
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
    Save->Version = 2; Save->Mind = Mind; Save->Mind.Sanitize(true);
    const bool OK = UGameplayStatics::SaveGameToSlot(Save, MemorySlot(), 0);
    if (OK) bMemoryDirty = false;
    else UE_LOG(LogTemp, Warning, TEXT("Fly mind save failed: %s"), *MemorySlot());
    return OK;
}
bool AFlySwarmMob::LoadMemory()
{
    if (!HasAuthority() || !bEnableCognition || MemorySlot().IsEmpty() || !UGameplayStatics::DoesSaveGameExist(MemorySlot(), 0)) return false;
    if (const auto* Save = Cast<UFlyCognitionSave>(UGameplayStatics::LoadGameFromSlot(MemorySlot(), 0)))
        if (Save->Version >= 1 && Save->Version <= 2) { Mind = Save->Mind; Mind.Sanitize(true); PublishInscriptions(); return true; }
    UE_LOG(LogTemp, Warning, TEXT("Fly mind save could not be loaded: %s"), *MemorySlot()); return false;
}
void AFlySwarmMob::ApplyShipMotion(const FTransform& Delta)
{
    if (!HasAuthority() || Delta.Equals(FTransform::Identity, 0.05f)) return;
    HomePosition = Delta.TransformPosition(HomePosition);
    TravelTarget = Delta.TransformPosition(TravelTarget);
    SetActorLocation(Delta.TransformPosition(GetActorLocation()), false, nullptr, ETeleportType::None);
    for (FFlyAgentState& Fly : Flies) Fly.Position = Delta.TransformPosition(Fly.Position);
    for (FFlySwarmPose& Pose : Poses) Pose.Position = Delta.TransformPosition(FVector(Pose.Position));
    for (FVector& Point : TravelTrail) Point = Delta.TransformPosition(Point);
    Navigation.Origin = HomePosition;
    UpdateInstances();
}
