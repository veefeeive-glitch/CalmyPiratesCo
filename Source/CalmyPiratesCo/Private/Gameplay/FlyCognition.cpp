#include "Gameplay/FlyCognition.h"

const FFlyPersonMemory* FFlyCognition::Find(const FString& Key) const
{
    return People.FindByPredicate([&](const FFlyPersonMemory& P) { return P.Key == Key; });
}
FFlyPersonMemory& FFlyCognition::Remember(const FString& Key, bool Persistent)
{
    if (auto* P = People.FindByPredicate([&](const FFlyPersonMemory& M) { return M.Key == Key; })) return *P;
    if (People.Num() >= 64)
    {
        int32 Oldest = 0;
        for (int32 I = 1; I < People.Num(); ++I) if (People[I].LastSeen < People[Oldest].LastSeen) Oldest = I;
        People.RemoveAt(Oldest);
    }
    FFlyPersonMemory& P = People.AddDefaulted_GetRef(); P.Key = Key; P.bPersistent = Persistent;
    return P;
}
float FFlyCognition::SensoryMultiplier(const FString& Key) const
{
    const auto* P = Find(Key);
    return P ? FMath::Clamp(.25f + P->ExpectedThreat - P->Trust * .2f, .12f, 1.2f) : .7f;
}
void FFlyCognition::Step(float Dt, const TArray<FFlyObservation>& Observations, float NeuralAlarm)
{
    if (!FMath::IsFinite(Dt) || Dt <= 0.f) return;
    Dt = FMath::Min(Dt, 1.f); LivedSeconds += Dt; ActivitySeconds += Dt;
    Energy = FMath::Clamp(Energy + Dt * (Activity == EFlyActivity::Rest ? .035f : -.008f), 0.f, 1.f);
    Curiosity = FMath::Clamp(Curiosity + Dt * (Activity == EFlyActivity::Explore ? -.016f : .012f), 0.f, 1.f);
    SocialNeed = FMath::Clamp(SocialNeed + Dt * .006f, 0.f, 1.f);
    Alarm = FMath::Max(0.f, Alarm - Dt * .12f);
    Alarm = FMath::Max(Alarm, FMath::Clamp(NeuralAlarm, 0.f, 1.f) * .35f);
    AttentionKey.Reset(); float Salience = -1.f;
    for (const auto& O : Observations)
    {
        auto& P = Remember(O.Key, O.bPersistent); P.LastSeen = LivedSeconds;
        P.Familiarity = FMath::Min(1.f, P.Familiarity + Dt / 45.f);
        // Prediction-error learning: repeated harmless proximity habituates;
        // sudden close movement conditions a stronger future response.
        const float Evidence = FMath::Clamp(O.Startle, 0.f, 1.f);
        P.ExpectedThreat += (Evidence - P.ExpectedThreat) * (1.f - FMath::Exp(-Dt * (Evidence > .5f ? .8f : .025f)));
        P.Trust = FMath::Clamp(P.Trust + Dt * (Evidence > .5f ? -.06f : .003f), -1.f, 1.f);
        const float Threat = FMath::Max(Evidence, P.ExpectedThreat) * O.Proximity;
        Alarm = FMath::Max(Alarm, Threat);
        const float Score = O.Proximity + Threat + (1.f - P.Familiarity) * .15f;
        if (Score > Salience) { Salience = Score; AttentionKey = O.Key; AttentionPosition = O.Position; }
    }
    if (Alarm > .65f) { Activity = EFlyActivity::Withdraw; ActivitySeconds = 0; return; }
    if (ActivitySeconds < 3.f) return; // Commitment prevents timer-rate thrashing.
    const float Scores[] = { .18f + Curiosity * .8f, (1.f - Energy) * 1.2f,
        AttentionKey.IsEmpty() ? -1.f : .25f + Alarm * .5f,
        AttentionKey.IsEmpty() ? -1.f : SocialNeed * .85f - Alarm * .5f, Alarm * .6f };
    int32 Best = 0;
    for (int32 I = 1; I < 5; ++I) if (Scores[I] > Scores[Best]) Best = I;
    const auto Next = static_cast<EFlyActivity>(Best);
    if (Next != Activity) { Activity = Next; ActivitySeconds = 0; }
}
FString FFlyCognition::Converse(const FString& Key, bool Persistent, EFlyUtterance Intent)
{
    if (uint8(Intent) > uint8(EFlyUtterance::Threaten)) return FString();
    auto& P = Remember(Key, Persistent);
    const bool Known = P.Familiarity > .15f || P.Conversations > 0;
    P.LastSeen = LivedSeconds; P.Conversations = FMath::Min(1000000, P.Conversations + 1); SocialNeed = FMath::Max(0.f, SocialNeed - .3f);
    if (Episodes.Num() >= 32) Episodes.RemoveAt(0);
    FFlyEpisode& Episode = Episodes.AddDefaulted_GetRef(); Episode.Person = Key; Episode.Event = Intent; Episode.Time = LivedSeconds;
    switch (Intent)
    {
    case EFlyUtterance::Greet:
        P.Trust = FMath::Min(1.f, P.Trust + .08f);
        return P.Trust < -.15f ? TEXT("I remember you. Keep your distance.") : Known ? TEXT("You have returned. I remember our encounters.") : TEXT("A new visitor. I am watching how you move.");
    case EFlyUtterance::Reassure:
        P.Trust = FMath::Min(1.f, P.Trust + .18f); P.ExpectedThreat *= .8f;
        return P.ExpectedThreat > .5f ? TEXT("I remember danger. Give me time and move slowly.") : TEXT("I will stay nearby. Your quiet visits are becoming familiar.");
    case EFlyUtterance::Threaten:
        P.Threats = FMath::Min(1000000, P.Threats + 1); P.Trust = FMath::Max(-1.f, P.Trust - .45f); P.ExpectedThreat = FMath::Min(1.f, P.ExpectedThreat + .5f);
        Alarm = 1.f; Activity = EFlyActivity::Withdraw; ActivitySeconds = 0;
        return TEXT("Stay back. I will remember this threat.");
    case EFlyUtterance::AskMemory:
        return FString::Printf(TEXT("We have spoken %d times. Threats remembered: %d. %s"), P.Conversations, P.Threats,
            P.ExpectedThreat > .5f ? TEXT("I expect danger from you.") : P.Trust > .15f ? TEXT("I am learning to trust you.") : TEXT("I am still learning about you."));
    }
    return FString();
}
FString FFlyCognition::DescribeActivity() const
{
    switch (Activity)
    {
    case EFlyActivity::Rest: return TEXT("I need to rest. I will stop here for a while.");
    case EFlyActivity::Observe: return TEXT("I am watching this visitor before approaching.");
    case EFlyActivity::Socialize: return TEXT("Someone is near. I want to make contact.");
    case EFlyActivity::Withdraw: return TEXT("I sense danger. I am keeping my distance.");
    default: return TEXT("I am exploring the ship. I want to see what is beyond this compartment.");
    }
}
void FFlyCognition::Sanitize(bool PersistentOnly)
{
    auto Unit = [](float X, float Default) { return FMath::IsFinite(X) ? FMath::Clamp(X, 0.f, 1.f) : Default; };
    Energy = Unit(Energy, .8f); Curiosity = Unit(Curiosity, .7f); SocialNeed = Unit(SocialNeed, .4f); Alarm = 0.f;
    if (!FMath::IsFinite(LivedSeconds) || LivedSeconds < 0.) LivedSeconds = 0.;
    Activity = EFlyActivity::Explore; ActivitySeconds = 0.f; AttentionKey.Reset();
    People.RemoveAll([&](const FFlyPersonMemory& P) { return P.Key.IsEmpty() || P.Key.Len() > 256 || (PersistentOnly && !P.bPersistent); });
    if (People.Num() > 64) People.SetNum(64);
    for (auto& P : People)
    {
        P.Familiarity = Unit(P.Familiarity, 0.f); P.ExpectedThreat = Unit(P.ExpectedThreat, .45f);
        P.Trust = FMath::IsFinite(P.Trust) ? FMath::Clamp(P.Trust, -1.f, 1.f) : 0.f;
        P.Conversations = FMath::Clamp(P.Conversations, 0, 1000000); P.Threats = FMath::Clamp(P.Threats, 0, 1000000);
        if (!FMath::IsFinite(P.LastSeen) || P.LastSeen < 0.) P.LastSeen = 0.;
    }
    Episodes.RemoveAll([&](const FFlyEpisode& E) { return !Find(E.Person) || !FMath::IsFinite(E.Time) || uint8(E.Event) > 3; });
    if (Episodes.Num() > 32) Episodes.RemoveAt(0, Episodes.Num() - 32);
}
