#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/NetSerialization.h"
#include "Gameplay/FlyCognition.h"
#include "Gameplay/FlyNavigation.h"
#include "FlySwarmMob.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;

// Anatomy and constant excitability exist once per loaded module/process.
// Runtime neuron indices are JSON array indices, never FlyWire root IDs.
struct CALMYPIRATESCO_API FFlyConnectome
{
    static constexpr int32 NeuronCount = 668;
    static constexpr int32 EdgeCount = 18968;
    const uint32* RowStart;
    const uint16* Targets;
    const float* Weights;
    const float* Baseline;
    const uint8* Role;       // 1 loom, 2/3 DNa L/R, 4 MDN, 5 forward, 6 groom, 7 wing, 8 GF
    const uint8* SensorSide; // 0 none, 1 left, 2 right
    const uint16* GroupCount;
    static const FFlyConnectome& Get();
};

// No graph copy, synapse state, UObject or per-neuron heap allocation.
// Four-ms inhibition requires history: 5 * 21 uint32 words = 420 bytes.
struct CALMYPIRATESCO_API FFlyLIFState
{
    static constexpr int32 Words = (FFlyConnectome::NeuronCount + 31) / 32;
    float Potential[FFlyConnectome::NeuronCount] = {};
    uint8 RefractoryMs[FFlyConnectome::NeuronCount] = {};
    uint32 SpikeHistory[5][Words] = {};
    float Rates[9] = {}; // Hz/neuron EMA; index 0 is entire population
    FRandomStream Random;
    uint8 HistoryHead = 0;
    bool bEscape = false;
    uint64 SimulatedMs = 0;

    void Initialize(int32 Seed);
    // Exactly one neural millisecond, with source decay/reset/refractory ordering.
    void Step(const FFlyConnectome& Graph, float LoomLeft, float LoomRight, float SensoryGain = 0.30f, bool bNoise = true);
};

struct FFlyAgentState
{
    FFlyLIFState Brain;
    FVector Position = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    float Yaw = 0.f;
    float EscapeSeconds = 0.f;
};

USTRUCT()
struct FFlySwarmPose
{
    GENERATED_BODY()
    UPROPERTY() FVector_NetQuantize10 Position = FVector::ZeroVector;
    UPROPERTY() uint16 Yaw = 0;
};

/** One authoritative actor for 20-200 independently simulated brains. */
UCLASS(Blueprintable)
class CALMYPIRATESCO_API AFlySwarmMob : public AActor
{
    GENERATED_BODY()
public:
    AFlySwarmMob();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void OnRep_ReplicatedMovement() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fly Swarm")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> FlyInstances;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Visuals, Category="Fly Swarm")
    TObjectPtr<UStaticMesh> FlyMesh;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Visuals, Category="Fly Swarm", meta=(ClampMin="0.001"))
    float MeshScale = 0.06f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="20", ClampMax="200"))
    int32 FlyCount = 50;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="20", ClampMax="30", Units="Hz"))
    int32 SimulationHz = 25;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="1", Units="cm"))
    float SpawnRadius = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="1", Units="cm"))
    float RoamRadius = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="1", Units="cm"))
    float DangerRadius = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="0", ClampMax="1"))
    float SensoryGain = 0.30f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="0", Units="cm/s"))
    float CruiseSpeed = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="0", Units="cm/s"))
    float EscapeSpeed = 320.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm", meta=(ClampMin="0", Units="deg/s"))
    float MaxTurnRate = 180.f;
    // Explicit danger actors, sampled alongside all possessed player pawns.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Fly Swarm")
    TArray<TObjectPtr<AActor>> DangerSources;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Swarm")
    int32 RandomSeed = 783;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Fly Mind") bool bEnableCognition = false;
    // One travelling colony; the saved HOLD 1 location is its starting home.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Mind") bool bRoamShip = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Mind", meta=(ClampMin="20", ClampMax="250", Units="cm/s")) float ColonySpeed = 135.f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Mind", meta=(ClampMin="10", ClampMax="75", Units="cm")) float ColonyRadius = 45.f;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Fly Mind") FVector TravelTarget = FVector::ZeroVector;
    bool IsRoamingShip() const { return bEnableCognition && bRoamShip; }
    bool CanFlySegment(const FVector& From, const FVector& To, float Radius) const;
    const TArray<FFlySwarmPose>& GetFlyPoses() const { return Poses; }
    // Unique per colony. Empty disables disk persistence. Server owns the save.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fly Mind") FName PersistentMemoryId;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Fly Mind") EFlyActivity Activity = EFlyActivity::Explore;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Fly Mind") FString Speech;
    UPROPERTY(Replicated, BlueprintReadOnly, Category="Fly Mind") int32 SpeechSerial = 0;
    bool CanConverse(const class APlayerController* PC) const;
    bool ReceiveUtterance(class APlayerController* PC, EFlyUtterance Intent);
    bool AimDistance(const FVector& Origin, const FVector& Direction, float Range, float Width, float& Distance) const;
    static AFlySwarmMob* FindAimedSwarm(const class APlayerController* PC);
    bool ReceiveSwat(class ACalmyCharacter* Attacker);
    UPROPERTY(Replicated, BlueprintReadOnly) int32 SwatCount = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bScattering = false;
    static AFlySwarmMob* FindConversationalSwarm(const class APlayerController* PC);
    const FFlyCognition& GetCognition() const { return Mind; }
    bool SaveMemory();
    bool LoadMemory();

    UFUNCTION(BlueprintPure, Category="Fly Swarm") int32 GetSimulatedFlyCount() const { return Flies.Num(); }
    UFUNCTION(BlueprintPure, Category="Fly Swarm") int32 GetVisibleFlyCount() const;
    uint64 GetNeuralMilliseconds() const { return Flies.IsEmpty() ? 0 : Flies[0].Brain.SimulatedMs; }
    uint64 GetEscapeEvents() const { return EscapeEvents; }
    double GetLastSimulationMilliseconds() const { return LastSimulationMilliseconds; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_Poses) TArray<FFlySwarmPose> Poses;
    UFUNCTION() void OnRep_Poses();
    UFUNCTION() void OnRep_Visuals();
    void SimulateSwarm();
    void UpdateInstances();
    void UpdateCognition(float Dt);
    void UpdateTravel(float Dt);
    bool IsOnShip(const FVector& Position) const;
    FFlyNavigation Navigation;
    TWeakObjectPtr<class ACalmyOcean> ShipOcean;
    FVector ColonyVelocity = FVector::ZeroVector;
    FVector HomePosition = FVector::ZeroVector;
    TArray<FVector> TravelTrail;
    FRandomStream TravelRandom;
    bool bPlanning = false;
    float TravelWait = 0.f;
    EFlyActivity PlannedActivity = EFlyActivity::Explore;
    FString MemorySlot() const;
    static FString PersonKey(const class APlayerController* PC, bool& bPersistent);
    FFlyCognition Mind;
    float ScatterSeconds = 0.f;
    FVector SwatOrigin = FVector::ZeroVector;
    TArray<FFlyObservation> Observations;
    TMap<TWeakObjectPtr<class APlayerController>, double> LastConversations;
    float MindRemainder = 0.f, SaveSeconds = 0.f, SpeechSeconds = 0.f, SilenceSeconds = 0.f;
    bool bMemoryDirty = false;
    TArray<FFlyAgentState> Flies;
    TArray<FTransform> InstanceTransforms;
    FTimerHandle SimulationTimer;
    double NeuralRemainderMs = 0.;
    double FixedSeconds = 0.04;
    double LastSimulationMilliseconds = 0.;
    uint64 EscapeEvents = 0;
};
