#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FlyCognition.generated.h"

// Engineered cognitive model, not a claim of phenomenal consciousness or a
// reconstruction of additional biological circuitry. One state per colony.
UENUM(BlueprintType)
enum class EFlyActivity : uint8 { Explore, Rest, Observe, Socialize, Withdraw };
UENUM(BlueprintType)
enum class EFlyUtterance : uint8 { Greet, Reassure, AskMemory, Threaten };
// Closed vocabulary left by visitors — Dark Souls-style marks the colony keeps.
UENUM(BlueprintType)
enum class EFlyMessageKind : uint8 { BeWary, SafeHere, PraiseColony, SeekContact };

USTRUCT()
struct FFlyPersonMemory
{
    GENERATED_BODY()
    UPROPERTY() FString Key;
    UPROPERTY() bool bPersistent = false;
    UPROPERTY() float Familiarity = 0.f;
    UPROPERTY() float ExpectedThreat = .45f;
    UPROPERTY() float Trust = 0.f;
    UPROPERTY() int32 Conversations = 0;
    UPROPERTY() int32 Threats = 0;
    UPROPERTY() double LastSeen = 0.;
};

USTRUCT()
struct FFlyEpisode
{
    GENERATED_BODY()
    UPROPERTY() FString Person;
    UPROPERTY() EFlyUtterance Event = EFlyUtterance::Greet;
    UPROPERTY() double Time = 0.;
};

USTRUCT()
struct FFlyInscription
{
    GENERATED_BODY()
    UPROPERTY() int32 Id = 0;
    UPROPERTY() EFlyMessageKind Kind = EFlyMessageKind::BeWary;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() int32 Echoes = 0;
    UPROPERTY() FString AuthorKey;
    UPROPERTY() double Time = 0.;
};

// Replicated presentation only — no author identity.
USTRUCT()
struct FFlyInscriptionView
{
    GENERATED_BODY()
    UPROPERTY() int32 Id = 0;
    UPROPERTY() EFlyMessageKind Kind = EFlyMessageKind::BeWary;
    UPROPERTY() FVector Location = FVector::ZeroVector;
    UPROPERTY() int32 Echoes = 0;
};

struct FFlyObservation
{
    FString Key;
    FVector Position = FVector::ZeroVector;
    bool bPersistent = false;
    float Proximity = 0.f;
    float Startle = 0.f;
};

USTRUCT()
struct CALMYPIRATESCO_API FFlyCognition
{
    GENERATED_BODY()
    UPROPERTY() TArray<FFlyPersonMemory> People;
    UPROPERTY() TArray<FFlyEpisode> Episodes;
    UPROPERTY() TArray<FFlyInscription> Inscriptions;
    UPROPERTY() int32 NextInscriptionId = 1;
    UPROPERTY() float Energy = .8f;
    UPROPERTY() float Curiosity = .7f;
    UPROPERTY() float SocialNeed = .4f;
    UPROPERTY() float Alarm = 0.f;
    UPROPERTY() double LivedSeconds = 0.;
    UPROPERTY() EFlyActivity Activity = EFlyActivity::Explore;
    float ActivitySeconds = 0.f;
    FString AttentionKey;
    FVector AttentionPosition = FVector::ZeroVector;
    // Derived from Inscriptions; recomputed, not authoritative across saves alone.
    float InscriptionCaution = 0.f;
    float InscriptionWelcome = 0.f;

    FFlyPersonMemory& Remember(const FString& Key, bool bPersistent);
    const FFlyPersonMemory* Find(const FString& Key) const;
    void Step(float Dt, const TArray<FFlyObservation>& Observations, float NeuralAlarm);
    FString Converse(const FString& Key, bool bPersistent, EFlyUtterance Intent);
    // Leave a mark or strengthen a nearby same-kind mark (echo). Returns reply speech.
    FString Inscribe(const FString& AuthorKey, bool bPersistent, EFlyMessageKind Kind, const FVector& Location);
    void RefreshInscriptionMood();
    int32 InscriptionCount() const { return Inscriptions.Num(); }
    int32 InscriptionEchoTotal() const;
    float SensoryMultiplier(const FString& Key) const;
    // Colony-wide loom gain from shared visitor marks (1 = neutral).
    float InscriptionSensoryGain() const;
    // Ambient wind / ascending context from marks, mapped like desktop-fly airPuff/gait.
    float InscriptionAirPuff() const;
    float InscriptionAscendDrive() const;
    FString DescribeActivity() const;
    // Spoken while the colony plans or commits to a ship roam goal.
    FString DescribeTravelThought(bool bPlanning, bool bTowardMark, bool bFleeMark) const;
    static FString ActivityLabel(EFlyActivity Activity);
    static FString MessageLabel(EFlyMessageKind Kind);
    static FString MessageDetail(EFlyMessageKind Kind);
    // Bounds and invalid values are checked on disk load. Session-only identities
    // must not become false recognition after restarting a server.
    void Sanitize(bool bPersistentOnly);
};

UCLASS()
class UFlyCognitionSave : public USaveGame
{
    GENERATED_BODY()
public:
    // 1 = people/episodes; 2 = + visitor inscriptions / echoes.
    UPROPERTY() int32 Version = 2;
    UPROPERTY() FFlyCognition Mind;
};
