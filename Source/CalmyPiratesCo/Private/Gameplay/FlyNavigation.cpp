#include "Gameplay/FlyNavigation.h"

FIntVector FFlyNavigation::Cell(const FVector& P) const
{
    const FVector V = (P - Origin) / CellSize;
    return FIntVector(FMath::RoundToInt(V.X), FMath::RoundToInt(V.Y), FMath::RoundToInt(V.Z));
}
FVector FFlyNavigation::Point(const FIntVector& C) const { return Origin + FVector(C) * CellSize; }
void FFlyNavigation::Visit(const FVector& P, float Time)
{
    if (LastVisit.Num() >= 16000) LastVisit.Reset(); // Bounded exploration memory, separate from people/save.
    LastVisit.Add(Cell(P), Time);
}
void FFlyNavigation::Begin(const FVector& P, const FVector& Desired, bool Directed, float Time)
{
    Start = P; Preference = Desired; bDirected = Directed; Now = Time;
    Path.Reset(); Nodes.Reset(); Seen.Reset(); Cursor = 0;
    Nodes.Add({Cell(P), INDEX_NONE}); Seen.Add(Cell(P));
}
bool FFlyNavigation::Advance(TFunctionRef<bool(const FVector&, const FVector&)> CanTravel, int32 Budget)
{
    if (Nodes.IsEmpty()) return true;
    static const FIntVector Directions[] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    for (int32 Work = 0; Work < Budget && Cursor < Nodes.Num() && Nodes.Num() < SearchLimit; ++Work)
    {
        const int32 Parent = Cursor++;
        const FIntVector C = Nodes[Parent].Cell;
        const FVector From = Parent == 0 ? Start : Point(C);
        for (const auto& D : Directions)
        {
            const FIntVector Next = C + D;
            if (Seen.Contains(Next)) continue;
            // Mark only reachable cells. A wall on one edge need not block a
            // different approach to the same cell through an open doorway.
            if (!CanTravel(From, Point(Next))) continue;
            Seen.Add(Next); Nodes.Add({Next, Parent});
            if (Nodes.Num() >= SearchLimit) break;
        }
    }
    if (Cursor < Nodes.Num() && Nodes.Num() < SearchLimit) return false;
    int32 Best = 0; double BestScore = -DBL_MAX;
    for (int32 I = 1; I < Nodes.Num(); ++I)
    {
        const FVector P = Point(Nodes[I].Cell);
        double Recent = 0;
        // Avoid orbiting the same few cells; neighbourhood visits count too.
        for (const auto& D : Directions)
            if (const float* T = LastVisit.Find(Nodes[I].Cell + D)) Recent += FMath::Max(0.f, 60.f - (Now - *T)) * 8.f;
        if (const float* T = LastVisit.Find(Nodes[I].Cell)) Recent += FMath::Max(0.f, 90.f - (Now - *T)) * 15.f;
        const double Score = bDirected ? -FVector::Dist(P, Preference)
            : FVector::Dist(P, Start) * .35 + FVector::DotProduct(P - Start, Preference) - Recent;
        if (Score > BestScore) { BestScore = Score; Best = I; }
    }
    while (Best > 0) { Path.Insert(Point(Nodes[Best].Cell), 0); Best = Nodes[Best].Parent; }
    Nodes.Reset(); Seen.Reset(); return true;
}
