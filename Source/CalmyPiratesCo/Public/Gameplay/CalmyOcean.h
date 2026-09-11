#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyOcean.generated.h"
class UProceduralMeshComponent;
class UPrimitiveComponent;
class UMaterialInterface;

USTRUCT()
struct FCalmyWaterDisturbance
{
    GENERATED_BODY()
    UPROPERTY() FVector Position = FVector::ZeroVector;
    UPROPERTY() double Birth = 0;
    UPROPERTY() float Amplitude = 0;
    UPROPERTY() float Width = 65;
    UPROPERTY() int32 Serial = 0;
};

/** Shared visual/physical wave field, in Unreal centimetres. No client outcome RPC. */
UCLASS()
class CALMYPIRATESCO_API ACalmyOcean : public AActor
{
    GENERATED_BODY()
public:
    ACalmyOcean();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    static ACalmyOcean* Find(const UWorld* World);
    static float WaveOffset(const FVector2D& XY, double Seconds);
    static float WaveVerticalVelocity(const FVector2D& XY,double Seconds);
    static float SeaStrength(double Seconds);
    FVector FlowAt(const FVector& Position) const;
    int32 ActiveDisturbances() const { return Disturbances.Num(); }
    static float Immersion(float SurfaceMinusCenter, float Radius);
    float HeightAt(const FVector& Position) const;
    float VerticalVelocityAt(const FVector& Position) const;
    float DisturbanceHeightAt(const FVector& Position, double Seconds) const;
    const TArray<FCalmyWaterDisturbance>& GetDisturbances() const { return Disturbances; }
    int32 GetDisturbanceSerial() const { return DisturbanceSerial; }
    double GetSurfaceSampleTime() const { return SurfaceTime; }
    float GetPeakDisturbanceAcceleration() const { return PeakDisturbanceAcceleration; }
    double WaveTime() const;
    bool IsOutsideSafetyArea(const FVector& Position) const;
    bool IsInsideDryHull(const FVector& Position) const;
    // Static map-authored profile loaded identically by server/clients. No runtime/client mutation.
    UPROPERTY(EditAnywhere) FVector DryHullOrigin=FVector::ZeroVector;
    UPROPERTY(EditAnywhere) TArray<FVector2D> DryHullStations;
    UPROPERTY(EditAnywhere) TArray<FVector2D> DryHullVertical;
    void ApplyBuoyancy(UPrimitiveComponent* Body, float LiftRatio = 1.8f) const;
    UPROPERTY(EditAnywhere) TObjectPtr<UMaterialInterface> SurfaceMaterial;
    UPROPERTY(EditAnywhere) TObjectPtr<UMaterialInterface> FoamMaterial;
    UPROPERTY(EditAnywhere) TObjectPtr<UMaterialInterface> InteractionMaterial;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Surface;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> InteractionMesh;
private:
    TArray<FVector> Vertices;
    TArray<FVector> WaveSin, WaveCos;
    double SurfaceTime=0;
    mutable float PeakDisturbanceAcceleration=0;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    void UpdateSurface();
    void UpdateInteractions();
    void UpdateDisturbanceSources();
    // Server detects crossings/wakes from real body positions, never client intent.
    // Server -> all clients bounded snapshot; serials identify events, no replay RPC.
    // Late join receives living waves at their original server time (not restarted).
    // Disconnect removes source tracking; already emitted waves decay normally.
    UPROPERTY(Replicated) TArray<FCalmyWaterDisturbance> Disturbances;
    UPROPERTY(Replicated) int32 DisturbanceSerial = 0;
    struct FWaterContact
    {
        FVector Previous = FVector::ZeroVector;
        double Time = 0, LastEmission = -10;
        float Bottom = 0, Surface = 0;
    };
    TMap<TWeakObjectPtr<AActor>, FWaterContact> WaterContacts;
};
