#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalmyOcean.generated.h"
class UProceduralMeshComponent;
class UPrimitiveComponent;
class UMaterialInterface;

/** Shared visual/physical wave field, in Unreal centimetres. No client outcome RPC. */
UCLASS()
class CALMYPIRATESCO_API ACalmyOcean : public AActor
{
    GENERATED_BODY()
public:
    ACalmyOcean();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static ACalmyOcean* Find(const UWorld* World);
    static float WaveOffset(const FVector2D& XY, double Seconds);
    static float WaveVerticalVelocity(const FVector2D& XY,double Seconds);
    static float SeaStrength(double Seconds);
    FVector FlowAt(const FVector& Position) const;
    int32 ActiveDisturbances() const { return Ripples.Num(); }
    static float Immersion(float SurfaceMinusCenter, float Radius);
    float HeightAt(const FVector& Position) const;
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
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Surface;
private:
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    void UpdateSurface();
    void UpdateInteractions();
    struct FRipple { FVector Position; double Birth; float Strength; };
    TArray<FRipple> Ripples;
    TMap<TWeakObjectPtr<AActor>,double> WetActors;
};
