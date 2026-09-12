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
    float Gain = P ? FMath::Clamp(.25f + P->ExpectedThreat - P->Trust * .2f, .12f, 1.2f) : .7f;
    // Shared visitor marks bias every fly's looming pathway — colony-wide vigilance or calm.
    Gain *= InscriptionSensoryGain();
    return FMath::Clamp(Gain, .08f, 1.4f);
}
float FFlyCognition::InscriptionSensoryGain() const
{
    return FMath::Clamp(1.f + InscriptionCaution * .55f - InscriptionWelcome * .28f, .55f, 1.7f);
}
float FFlyCognition::InscriptionAirPuff() const
{
    // Uneasy marks stir the wind/JO sensory pathway (desktop-fly airPuff analogue).
    return FMath::Clamp(InscriptionCaution * .55f, 0.f, 1.f);
}
float FFlyCognition::InscriptionAscendDrive() const
{
    // Welcoming marks drive ascending proprioceptive context toward contact/locomotion.
    return FMath::Clamp(InscriptionWelcome * .7f, 0.f, 1.f);
}
void FFlyCognition::Step(float Dt, const TArray<FFlyObservation>& Observations, float NeuralAlarm)
{
    if (!FMath::IsFinite(Dt) || Dt <= 0.f) return;
    Dt = FMath::Min(Dt, 1.f); LivedSeconds += Dt; ActivitySeconds += Dt;
    RefreshInscriptionMood();
    Energy = FMath::Clamp(Energy + Dt * (Activity == EFlyActivity::Rest ? .035f : -.009f), 0.f, 1.f);
    Curiosity = FMath::Clamp(Curiosity + Dt * (Activity == EFlyActivity::Explore ? -.016f : .012f), 0.f, 1.f);
    SocialNeed = FMath::Clamp(SocialNeed + Dt * (.01f + InscriptionWelcome * .008f), 0.f, 1.f);
    Alarm = FMath::Max(0.f, Alarm - Dt * .12f);
    Alarm = FMath::Max(Alarm, FMath::Clamp(NeuralAlarm, 0.f, 1.f) * .35f);
    Alarm = FMath::Max(Alarm, InscriptionCaution * .45f);
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
    if (Alarm > .55f) { Activity = EFlyActivity::Withdraw; ActivitySeconds = 0; return; }
    // Rest can interrupt sooner once energy is low; other commitments stay ~3s.
    const float Commit = (Activity == EFlyActivity::Rest && Energy > .55f) ? 2.f : 3.f;
    if (ActivitySeconds < Commit) return;
    const bool Visitor = !AttentionKey.IsEmpty();
    // Visitor-present scores must beat idle Explore so Observe/Socialize actually appear.
    const float Scores[] = {
        Visitor ? .12f + Curiosity * .35f : .18f + Curiosity * .8f, // Explore
        (1.f - Energy) * 1.2f, // Rest
        Visitor ? .55f + Alarm * .35f + (1.f - SocialNeed) * .2f + InscriptionCaution * .25f : -1.f, // Observe
        Visitor ? .4f + SocialNeed * 1.1f - Alarm * .35f + InscriptionWelcome * .35f : -1.f, // Socialize
        Alarm * 1.15f // Withdraw utility (hard cut above still wins first)
    };
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
    {
        FString Mood = ActivityLabel(Activity).ToLower();
        return FString::Printf(TEXT("We have spoken %d times. Threats remembered: %d. Familiarity %.0f%%. Marks left by visitors: %d (echoes %d). Now I am %s. %s"),
            P.Conversations, P.Threats, P.Familiarity * 100.f, InscriptionCount(), InscriptionEchoTotal(), *Mood,
            InscriptionCaution > .35f ? TEXT("The gathered words warn me.") :
            InscriptionWelcome > .35f ? TEXT("The gathered words invite contact.") :
            P.ExpectedThreat > .5f ? TEXT("I expect danger from you.") : P.Trust > .15f ? TEXT("I am learning to trust you.") : TEXT("I am still learning about you."));
    }
    }
    return FString();
}
void FFlyCognition::RefreshInscriptionMood()
{
    float Wary = 0.f, Welcome = 0.f;
    for (const auto& I : Inscriptions)
    {
        if (uint8(I.Kind) > uint8(EFlyMessageKind::SeekContact)) continue;
        const float Weight = 1.f + FMath::Clamp(I.Echoes, 0, 99) * .35f;
        switch (I.Kind)
        {
        case EFlyMessageKind::BeWary: Wary += Weight; break;
        case EFlyMessageKind::SafeHere: Welcome += Weight * .8f; Wary = FMath::Max(0.f, Wary - Weight * .25f); break;
        case EFlyMessageKind::PraiseColony: Welcome += Weight; break;
        case EFlyMessageKind::SeekContact: Welcome += Weight * 1.1f; break;
        default: checkNoEntry(); break;
        }
    }
    InscriptionCaution = FMath::Clamp(Wary / 5.f, 0.f, 1.f);
    InscriptionWelcome = FMath::Clamp(Welcome / 5.f, 0.f, 1.f);
}
int32 FFlyCognition::InscriptionEchoTotal() const
{
    int32 Total = 0;
    for (const auto& I : Inscriptions) Total += FMath::Max(0, I.Echoes);
    return Total;
}
FString FFlyCognition::Inscribe(const FString& AuthorKey, bool Persistent, EFlyMessageKind Kind, const FVector& Location)
{
    if (uint8(Kind) > uint8(EFlyMessageKind::SeekContact) || AuthorKey.IsEmpty() || Location.ContainsNaN()) return FString();
    Remember(AuthorKey, Persistent).LastSeen = LivedSeconds;
    // Same kind within ~1.8 m strengthens an existing mark (appraisal / echo).
    constexpr float MergeCmSq = 180.f * 180.f;
    int32 Best = INDEX_NONE; float BestDist = MergeCmSq;
    for (int32 I = 0; I < Inscriptions.Num(); ++I)
    {
        if (Inscriptions[I].Kind != Kind) continue;
        const float D = FVector::DistSquared(Inscriptions[I].Location, Location);
        if (D < BestDist) { BestDist = D; Best = I; }
    }
    bool bEcho = false;
    if (Best != INDEX_NONE)
    {
        Inscriptions[Best].Echoes = FMath::Min(99, Inscriptions[Best].Echoes + 1);
        Inscriptions[Best].Time = LivedSeconds;
        bEcho = true;
    }
    else
    {
        if (Inscriptions.Num() >= 24)
        {
            int32 Oldest = 0;
            for (int32 I = 1; I < Inscriptions.Num(); ++I) if (Inscriptions[I].Time < Inscriptions[Oldest].Time) Oldest = I;
            Inscriptions.RemoveAt(Oldest);
        }
        FFlyInscription& Mark = Inscriptions.AddDefaulted_GetRef();
        Mark.Id = NextInscriptionId++;
        if (NextInscriptionId <= 0) NextInscriptionId = 1;
        Mark.Kind = Kind; Mark.Location = Location; Mark.Echoes = 0; Mark.AuthorKey = AuthorKey; Mark.Time = LivedSeconds;
    }
    RefreshInscriptionMood();
    SocialNeed = FMath::Clamp(SocialNeed + (Kind == EFlyMessageKind::BeWary ? -.05f : .12f), 0.f, 1.f);
    if (Kind == EFlyMessageKind::BeWary) Alarm = FMath::Max(Alarm, .35f + InscriptionCaution * .4f);
    if (Kind == EFlyMessageKind::SeekContact || Kind == EFlyMessageKind::PraiseColony)
    { Activity = EFlyActivity::Socialize; ActivitySeconds = 0.f; }
    else if (Kind == EFlyMessageKind::BeWary && InscriptionCaution > .5f)
    { Activity = EFlyActivity::Observe; ActivitySeconds = 0.f; }
    if (bEcho)
        return FString::Printf(TEXT("I feel that word again. \"%s\" grows stronger among us."), *MessageLabel(Kind));
    return FString::Printf(TEXT("I keep this mark with the others. \"%s\" — visitors leave words that become mine."), *MessageLabel(Kind));
}
FString FFlyCognition::MessageLabel(EFlyMessageKind Kind)
{
    switch (Kind)
    {
    case EFlyMessageKind::BeWary: return TEXT("BE WARY");
    case EFlyMessageKind::SafeHere: return TEXT("SAFE HERE");
    case EFlyMessageKind::PraiseColony: return TEXT("PRAISE THE FLIES");
    case EFlyMessageKind::SeekContact: return TEXT("TRY TALKING");
    default: checkNoEntry(); return TEXT("BE WARY");
    }
}
FString FFlyCognition::MessageDetail(EFlyMessageKind Kind)
{
    switch (Kind)
    {
    case EFlyMessageKind::BeWary: return TEXT("Leave a warning the colony will keep");
    case EFlyMessageKind::SafeHere: return TEXT("Mark this place as calm");
    case EFlyMessageKind::PraiseColony: return TEXT("Strengthen goodwill toward the swarm");
    case EFlyMessageKind::SeekContact: return TEXT("Invite the colony to approach");
    default: checkNoEntry(); return TEXT("Leave a mark");
    }
}
FString FFlyCognition::ActivityLabel(EFlyActivity InActivity)
{
    switch (InActivity)
    {
    case EFlyActivity::Rest: return TEXT("RESTING");
    case EFlyActivity::Observe: return TEXT("WATCHING");
    case EFlyActivity::Socialize: return TEXT("APPROACHING");
    case EFlyActivity::Withdraw: return TEXT("WITHDRAWING");
    case EFlyActivity::Explore: return TEXT("EXPLORING");
    default:
    {
        checkNoEntry();
        return TEXT("EXPLORING");
    }
    }
}
FString FFlyCognition::DescribeActivity() const
{
    if (InscriptionCaution > .45f && Activity != EFlyActivity::Withdraw)
        return TEXT("Visitor marks warn of danger. I am cautious while I move.");
    if (InscriptionWelcome > .45f && (Activity == EFlyActivity::Socialize || Activity == EFlyActivity::Observe))
        return TEXT("Many marks ask for contact. I am drawn toward visitors.");
    switch (Activity)
    {
    case EFlyActivity::Rest: return TEXT("I need to rest. I will settle on the deck for a while.");
    case EFlyActivity::Observe: return TEXT("I am watching this visitor before I come closer.");
    case EFlyActivity::Socialize: return TEXT("Someone is near. I want to make contact.");
    case EFlyActivity::Withdraw: return TEXT("I sense danger. I am keeping my distance.");
    case EFlyActivity::Explore: return TEXT("I am exploring the ship. I want to see what is beyond this compartment.");
    default:
    {
        checkNoEntry();
        return TEXT("I am exploring the ship. I want to see what is beyond this compartment.");
    }
    }
}
FString FFlyCognition::DescribeTravelThought(bool bPlanning, bool bTowardMark, bool bFleeMark) const
{
    if (bPlanning)
    {
        if (bFleeMark) return TEXT("I am thinking. The marks feel unsafe — I will choose another way through the ship.");
        if (bTowardMark) return TEXT("I am thinking. Visitor marks pull me toward another place on the ship.");
        if (Curiosity > .55f) return TEXT("I am thinking where to go. Curiosity wants a new compartment.");
        return TEXT("I am thinking. I will roam further when I settle on a path.");
    }
    if (bFleeMark) return TEXT("I have a path. I am leaving the wary marks behind.");
    if (bTowardMark) return TEXT("I have a path. I am moving toward a place visitors marked.");
    if (Activity == EFlyActivity::Withdraw) return TEXT("I have a path. I am withdrawing through the ship.");
    if (Activity == EFlyActivity::Socialize) return TEXT("I have a path. I am closing in to make contact.");
    if (Activity == EFlyActivity::Observe) return TEXT("I have a path. I will watch from a safer standoff.");
    return TEXT("I have a path. I will push deeper into the freighter.");
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
    Inscriptions.RemoveAll([&](const FFlyInscription& I)
    {
        return I.Id <= 0 || uint8(I.Kind) > uint8(EFlyMessageKind::SeekContact) || I.Location.ContainsNaN()
            || !FMath::IsFinite(I.Time) || I.Time < 0. || I.AuthorKey.IsEmpty() || I.AuthorKey.Len() > 256
            || (PersistentOnly && !Find(I.AuthorKey));
    });
    if (Inscriptions.Num() > 24) Inscriptions.RemoveAt(0, Inscriptions.Num() - 24);
    for (auto& I : Inscriptions) I.Echoes = FMath::Clamp(I.Echoes, 0, 99);
    if (NextInscriptionId < 1) NextInscriptionId = 1;
    for (const auto& I : Inscriptions) if (I.Id >= NextInscriptionId) NextInscriptionId = I.Id + 1;
    RefreshInscriptionMood();
}
