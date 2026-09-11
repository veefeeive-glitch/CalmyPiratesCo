#pragma once
#include "CoreMinimal.h"

// One bounded, incremental free-flight search per colony. Collision queries are
// supplied by the owning actor on the game thread, never by neural workers.
struct CALMYPIRATESCO_API FFlyNavigation
{
    static constexpr float CellSize = 75.f;
    static constexpr int32 SearchLimit = 768;
    TArray<FVector> Path;
    TMap<FIntVector, float> LastVisit;
    FVector Origin = FVector::ZeroVector;
    FVector Preference = FVector::ZeroVector;
    FVector Start = FVector::ZeroVector;
    bool bDirected = false;
    float Now = 0.f;
    int32 Cursor = 0;
    struct FNode { FIntVector Cell; int32 Parent; };
    TArray<FNode> Nodes;
    TSet<FIntVector> Seen;
    FIntVector Cell(const FVector& P) const;
    FVector Point(const FIntVector& C) const;
    void Begin(const FVector& Position, const FVector& Desired, bool Directed, float Time);
    // Returns true when the search has finished. At most Budget*6 edge tests.
    bool Advance(TFunctionRef<bool(const FVector&, const FVector&)> CanTravel, int32 Budget = 24);
    void Visit(const FVector& Position, float Time);
};
