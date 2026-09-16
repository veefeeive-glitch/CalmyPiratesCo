#pragma once

#include "CoreMinimal.h"
#include "Core/GameplayPolicies.h"
#include "Core/BodycamSpring.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CalmyCharacter.generated.h"

class ACalmyCargo;
class ACalmyDoor;
class ACalmyCrewHelper;
class ACalmyLeak;
class UCalmyCrewPresentation;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UENUM()
enum class ECalmyGrabFeedback : uint8 { Ready, NoTarget, TooFar, Obstructed, Occupied, Delivered, HandsFull, Recovering, Swimming, NeedHelper, Unstable, Retry };

UCLASS()
class CALMYPIRATESCO_API UCalmyCharacterMovement : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
    virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
    virtual void PhysCustom(float DeltaSeconds, int32 Iterations) override;
    bool IsSurfaceSwimming() const { return MovementMode == MOVE_Custom && CustomMovementMode == 1; }
};

UCLASS(Config=Game)
class CALMYPIRATESCO_API ACalmyCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	ACalmyCharacter(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    virtual void CalcCamera(float DeltaSeconds,FMinimalViewInfo& OutResult) override;
    virtual void Landed(const FHitResult& Hit) override;
    void RequestStumble();
    void ToggleSteadyCamera();
    bool IsSteadyCamera() const { return bSteadyCamera; }
    void RequestHelper();
    ACalmyCrewHelper* FindHelperTarget() const;
    void CycleCameraMotion();
    float GetCameraMotionScale() const { return CameraMotionScale; }
    void AdjustLookSensitivity(float Delta);
    float GetLookSensitivity() const { return LookSensitivity; }
    // Server-owned interaction tuning; client values are presentation only until replication.
    UPROPERTY(EditDefaultsOnly,Config,Replicated,Category="Interaction",meta=(ClampMin="100",ClampMax="250")) float GrabRangeCm=220.f;
    FVector GetInteractionOrigin() const;
    ECalmyGrabFeedback GetGrabFeedback(const ACalmyCargo* Cargo) const;
    FString GetInteractionHint() const;
    static FString GrabFeedbackText(ECalmyGrabFeedback Reason);
    UPROPERTY(Replicated,BlueprintReadOnly) TObjectPtr<ACalmyCrewHelper> CrewHelper;
    UCalmyCrewPresentation* GetCrewPresentation() const { return CrewPresentation; }

	void MoveIntent(const FVector2D& Intent);
    bool IsInOcean() const;
    void BeginJumpInput();
    void EndJumpInput();
	void RequestGrab();
	void RequestDrop();
	void RequestThrow();
    void RequestPunch();
    bool PerformPunch(); // Authority-only; resolves target, damage and force locally.
    AActor* FindMeleeTarget() const;
    virtual bool ReceiveMeleeHit(ACalmyCharacter* Attacker);
    float GetPunchBlend() const;
    float GetHitFlash() const;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 PunchCount = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 MeleeHitsReceived = 0;
    UPROPERTY(Replicated) float PunchStartedAt = -100.f;
    UPROPERTY(Replicated) float LastMeleeHitAt = -100.f;
	void RequestRestart();
	void RequestHandoff();
	void BeginRepairInput();
	void EndRepairInput();
	void EndInteractInput();
	bool IsHoldingDoor() const { return HeldDoor.IsValid(); }
	bool ReturnToDeck(bool bIncident = true);
	bool CanGrab(const ACalmyCargo* Cargo) const;
	bool ReceiveCargoImpact(float Speed,FVector Direction=FVector::ZeroVector);
    // Authority-only landing input from CharacterMovement, never a client RPC.
    void ApplyLandingImpact(float Speed);
    UPROPERTY(Replicated,BlueprintReadOnly) float Health=100.f;
	void ReleaseCargo();
	bool IsControlled() const { return Lifecycle == ECalmyPlayerLifecycleState::Controlled; }
	FVector GetCarryLocation() const;
	ACalmyCargo* FindGrabTarget() const;

	UPROPERTY(Replicated, BlueprintReadOnly) TObjectPtr<ACalmyCargo> HeldCargo;
	UPROPERTY(ReplicatedUsing=OnRep_Lifecycle, BlueprintReadOnly)
	ECalmyPlayerLifecycleState Lifecycle = ECalmyPlayerLifecycleState::Controlled;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 KnockdownCount = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 RecoveryCount = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 RejectedCommands = 0;
	UPROPERTY(Replicated, BlueprintReadOnly) int32 ResetCount = 0;

private:
    float NextPunchAt = 0.f;
    // Owning client -> owned server pawn. Shared monotonic sequence/rate gate,
    // controlled/dry/empty hands and server cooldown. Server recomputes short
    // aimed first hit and wall occlusion. Replay cannot cause another strike.
    // Late join receives counters/timestamps, never old hit RPCs; disconnect
    // destroys the pawn and invalidates helper provocation references.
    UFUNCTION(Server, Reliable) void ServerPunch(uint32 Sequence);
    friend class UCalmyMeleeProbe;
    // Owning client -> server pawn. Shared monotonic command gate, current door revision,
    // controlled/dry/empty-handed state and first 220cm aim/LOS hit are recomputed server-side.
    // Duplicate/stale commands do nothing; late join receives door angle/state/revision.
    // Door operation has no user lease: disconnect leaves the server-driven door in its current motion.
    UFUNCTION(Server,Reliable) void ServerDoor(ACalmyDoor* Door,uint32 ExpectedRevision,uint32 Sequence);
    // Owning client -> server pawn when E is held on a door (bActive) and when it is released. Monotonic sequence;
    // the server re-checks controlled/dry/empty-handed state and the 220cm aim at the leaf on begin, keeps one holder
    // per door and drops the hold when the pawn walks more than 3m away, loses control, picks up cargo or disconnects.
    // Duplicate or out-of-order begin/end are ignored; late joiners see the replicated angle, velocity and holder.
    UFUNCTION(Server,Reliable) void ServerDoorGrab(ACalmyDoor* Door,bool bActive,uint32 Sequence);
    // Owning client -> server, unreliable ~20 Hz while holding. Push is quantised to [-127,127] and only the door's
    // current holder moves it; it expires after 0.25 s, so lost or delayed packets let the heavy leaf coast instead
    // of running away. Client angles or velocities are never sent. Disconnect clears the holder on the server.
    UFUNCTION(Server,Unreliable) void ServerDoorDrag(ACalmyDoor* Door,int8 Push);
    friend class UCalmyInteractionProbe;
    friend class UCalmyDoorProbe;
    friend class UCalmyCharacterProbe;
    friend class UCalmyHelperProbe;
	friend class UCalmyNetworkProbe;
	friend class UCalmyShiftProbe;
	friend class UCalmyCargoProbe;
    friend class UCalmyWaterProbe;
	friend class UCalmyLeakProbe;
	friend class UCalmyPlaythroughProbe;
	// Owning client -> server pawn. Monotonic sequence, heartbeat rate, current round, target world,
	// empty hands, controlled + stationary grounded pawn, 180cm and visibility validated server-side.
	// No client progress/time. Release consumes sequence but bypasses rate limit; replay is a no-op.
	// Late join gets the leak snapshot. Lease expires in .65s; Logout/EndPlay also clears repairer.
	UFUNCTION(Server, Reliable) void ServerRepair(ACalmyLeak* Leak, int32 ExpectedRound, bool bActive, uint32 Sequence);
	// Owning client -> server. Kept for probes; gameplay input uses ServerHoldInteract.
	UFUNCTION(Server, Reliable) void ServerFuel(bool bActive, uint32 Sequence);
	// Owning client -> server while F is held (0.2 s pulses) or released. Server picks leak valve or fuel pump from its own
	// state (range, phase, empty hands, lease). Stale/duplicate sequences and rate-gate misses are rejected and counted.
	// Late join needs nothing (progress replicates on the leak/director); disconnect clears via Logout/EndPlay lease expiry.
	UFUNCTION(Server, Reliable) void ServerHoldInteract(bool bActive, uint32 Sequence);
	// RPC contract (all three below): caller = owning client; receiver = server on owned pawn.
	// Gate validates monotonic sequence and server-time rate. Grab additionally validates lifecycle,
	// target class/world, first aimed proxy, short surface range, visibility and exclusive reservation;
	// aim derives from server-owned pawn + control rotation, never a client-supplied ray. Drop/throw validate holder.
	// Replayed sequences are no-ops. Late join receives replicated current state, not RPC history.
	// Logout/EndPlay release held cargo; queued calls on a destroyed/disowned pawn cannot act.
	UFUNCTION(Server, Reliable) void ServerGrab(ACalmyCargo* Cargo, uint32 Sequence);
	UFUNCTION(Server, Reliable) void ServerDrop(uint32 Sequence);
	UFUNCTION(Server, Reliable) void ServerThrow(uint32 Sequence);
    // Server -> owning client, cosmetic reply after an accepted-rate grab attempt. No gameplay writes.
    // Replays/floods receive no extra reply; late join does not replay feedback; pawn destruction discards it.
    UFUNCTION(Client,Reliable) void ClientGrabFeedback(ECalmyGrabFeedback Reason);
	// Owning client -> server. Command gate + completed round + local host controller required.
	// ExpectedRound rejects stale/duplicate restart after a new round. Late join uses Shift snapshot.
	// Disconnect destroys the owned pawn; no persistent client request can restart a future round.
	UFUNCTION(Server, Reliable) void ServerRestart(int32 ExpectedRound, uint32 Sequence);
	// Owning client -> server. Shared sequence/rate gate; held unstable cargo, receiver world,
	// controlled/empty hands, human receiver (NPC has no handoff task yet), distance/visibility,
	// and cargo-wide 2s cooldown validated on server.
	// Duplicate/late requests are no-ops; late join receives holder+condition snapshots.
	// Disconnect releases whichever pawn currently holds the cargo through the existing path.
	UFUNCTION(Server, Reliable) void ServerHandoff(ACalmyCharacter* Receiver, uint32 Sequence);
	UFUNCTION() void OnRep_Lifecycle();
    // Owning client -> owned server pawn. Shared monotonic sequence/rate gate,
    // controlled + grounded + not swimming + 5 second server cooldown required.
    // Replay consumes no extra stumble. Server supplies force/duration; no client
    // vectors. Late join gets Lifecycle/counters; disconnect destroys the pawn.
    UFUNCTION(Server,Reliable) void ServerStumble(uint32 Sequence);
    // Owning player -> server owned pawn; shared monotonic/rate gate, grounded
    // controlled dry player, 2s cooldown. Recruit an existing free NPC within
    // 220cm and line of sight, or release own NPC. Never spawn via this RPC.
    // One leader per helper; replay cannot reassign. Late join receives the
    // same persistent actor/leader/state; disconnect releases it in place.
    UFUNCTION(Server,Reliable) void ServerHelper(uint32 Sequence);
    void SaveCameraPreferences();
public:
    void ApplyLocalPreferences();
private:
    bool BeginKnockdown(float Speed,FVector Direction);
	void SetLifecycle(ECalmyPlayerLifecycleState Next);
	void BeginRecovery();
	void FinishRecovery();
	void Forward(const FInputActionValue& Value);
	void Backward(const FInputActionValue& Value);
	void Right(const FInputActionValue& Value);
	void Left(const FInputActionValue& Value);
	void LookYaw(const FInputActionValue& Value);
	void LookPitch(const FInputActionValue& Value);
	void MoveRelativeIntent(const FVector2D& Intent);
	UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UCalmyCrewPresentation> CrewPresentation;
    FCalmyBodycamSpring BodycamMotion;
    FVector CameraPreviousVelocity=FVector::ZeroVector;
    float CameraPhase=0,CameraFall=0,NextStumbleTime=0;
    bool bSteadyCamera=false;
    float CameraMotionScale=1.f,NextHelperCommand=0;
    float LookSensitivity=1.f,FeedbackUntil=0.f;
    ECalmyGrabFeedback LastGrabFeedback=ECalmyGrabFeedback::Ready;
	UPROPERTY() TObjectPtr<UInputMappingContext> InputContext;
	UPROPERTY() TArray<TObjectPtr<UInputAction>> InputActions;
    UPROPERTY() TArray<FKey> InputDefaultKeys;
	FCalmyCommandGate CommandGate;
	FCalmyCommandGate RepairGate;
	uint32 LastRepairIssued = 0;
	uint32 LastRepairReceived = 0;
	bool bRepairInput = false;
	// E on a door: a short tap toggles it, holding grabs the leaf and mouse motion swings it.
	TWeakObjectPtr<ACalmyDoor> PendingDoor;
	TWeakObjectPtr<ACalmyDoor> HeldDoor;
	bool bInteractHeld = false;
	float InteractHeldSeconds = 0, DoorMouseX = 0, DoorMouseY = 0, DoorMouseTotal = 0, DoorSendCountdown = 0;
	uint32 LastDoorGrabIssued = 0;
	uint32 LastDoorGrabReceived = 0;
	int32 HeardPunchCount = -1;
	float RepairPulseCountdown = 0;
	uint32 LastIssuedSequence = 0;
	FTimerHandle RecoveryTimer;
	FVector HomeLocation = FVector::ZeroVector;
	FRotator HomeRotation = FRotator::ZeroRotator;
};
