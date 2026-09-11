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
    UPROPERTY() float Energy = .8f;
    UPROPERTY() float Curiosity = .7f;
    UPROPERTY() float SocialNeed = .4f;
    UPROPERTY() float Alarm = 0.f;
    UPROPERTY() double LivedSeconds = 0.;
    UPROPERTY() EFlyActivity Activity = EFlyActivity::Explore;
    float ActivitySeconds = 0.f;
    FString AttentionKey;
    FVector AttentionPosition = FVector::ZeroVector;

    FFlyPersonMemory& Remember(const FString& Key, bool bPersistent);
    const FFlyPersonMemory* Find(const FString& Key) const;
    void Step(float Dt, const TArray<FFlyObservation>& Observations, float NeuralAlarm);
    FString Converse(const FString& Key, bool bPersistent, EFlyUtterance Intent);
    float SensoryMultiplier(const FString& Key) const;
    FString DescribeActivity() const;
    // Bounds and invalid values are checked on disk load. Session-only identities
    // must not become false recognition after restarting a server.
    void Sanitize(bool bPersistentOnly);
};

UCLASS()
class UFlyCognitionSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Version = 1;
    UPROPERTY() FFlyCognition Mind;
};
