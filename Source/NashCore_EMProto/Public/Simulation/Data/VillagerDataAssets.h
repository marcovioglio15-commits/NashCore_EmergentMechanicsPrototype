// Ensures the file is included only once during compilation to prevent duplicate symbol errors.
#pragma once

// Region: Engine and gameplay includes.
#pragma region Includes
// Provides fundamental Unreal types and macros.
#include "CoreMinimal.h"
// Supplies the base class for data-driven assets editable in the editor.
#include "Engine/DataAsset.h"
// Gives access to gameplay tags used for lightweight identifiers.
#include "GameplayTagContainer.h"
// Adds curve asset support for time-based value sampling.
#include "Curves/CurveFloat.h"
#pragma endregion Includes

// Generated header include required for Unreal's reflection system.
#include "VillagerDataAssets.generated.h"

// Defines the day phase used by the clock to drive day-night dependent logic.
UENUM(BlueprintType)
enum class EVillageDayPhase : uint8
{
	Day, // Represents the daytime interval.
	Night // Represents the nighttime interval.
};

// Encapsulates thresholds for mild and critical need states for a single need.
USTRUCT(BlueprintType)
struct FNeedThresholds
{
	GENERATED_BODY() // Macro that generates boilerplate constructors and reflection data.

public:
	// Lower bound above which the need is considered mild.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float MildThreshold = 0.8f;

	// Lower bound above which the need is considered critical.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float CriticalThreshold = 0.5f;
};

// Declares a designer-editable definition for a villager need.
USTRUCT(BlueprintType)
struct FNeedDefinition
{
	GENERATED_BODY() // Macro generating reflection helpers.

public:
	// Tag uniquely identifying the need for lookup and curve binding.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	FGameplayTag NeedTag;

	// Value from which the need starts when the villager is spawned or reset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float StartingValue = 0.25f;

	// Minimum allowed value for the need to avoid negative overflow.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float MinValue = 0.0f;

	// Maximum allowed value for the need to avoid runaway accumulation.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float MaxValue = 1.0f;

	// Struct holding the mild and critical thresholds defining state bands.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	FNeedThresholds Thresholds;

	// Priority weight used to resolve ties when multiple needs are critical.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	float PriorityWeight = 1.0f;

	// Curve mapping normalized need value (0-1) to probability of forcing a satisfying activity.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	UCurveFloat* ForceActivityProbabilityCurve = nullptr;

	// Activity tag that satisfies this need when executed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Need")
	FGameplayTag SatisfyingActivityTag;
};

// Records a time window in hours that bounds when a PartOfDay activity may run.
USTRUCT(BlueprintType)
struct FActivityTimeWindow
{
	GENERATED_BODY() // Generates reflection details.

public:
	// Inclusive hour when the activity may begin.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	int32 AllowedStartHour = 6;

	// Inclusive hour when the activity must end.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	int32 AllowedEndHour = 18;
};

// Defines a tagged location the villager can move toward to perform actions.
USTRUCT(BlueprintType)
struct FTaggedLocation
{
	GENERATED_BODY() // Adds default constructor and reflection.

public:
	// Tag describing the logical place, enabling indirection in design tools.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Location")
	FGameplayTag LocationTag;

	// World-space transform representing the target position and facing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Location")
	FTransform LocationTransform;
};

// Represents a single activity definition with scheduling and need curves.
USTRUCT(BlueprintType)
struct FActivityDefinition
{
	GENERATED_BODY() // Generates required reflection metadata.

public:
	// Tag uniquely identifying the activity for scheduling and references.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	FGameplayTag ActivityTag;

	// Flag indicating whether the activity is part of the daily routine.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	bool bIsPartOfDay = true;

	// Order index for PartOfDay activities to define the default schedule.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (EditCondition = "bIsPartOfDay"))
	int32 DayOrder = 0;

	// Per-need curves that apply deltas over activity time while active.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	TMap<FGameplayTag, UCurveFloat*> NeedCurves;

	// Indicates whether the villager must move to a specific transform.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	bool bRequiresSpecificLocation = false;

	// Location tag used to resolve the activity position through tagged actors in the world.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (EditCondition = "bRequiresSpecificLocation"))
	FGameplayTag ActivityLocationTag;

	// Resource tag that must be present before the activity can start.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
	FGameplayTag RequiredResourceTag;

	// Time window limiting when PartOfDay activities may execute.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (EditCondition = "bIsPartOfDay"))
	FActivityTimeWindow PartOfDayWindow;

	// Duration in in-game minutes for non PartOfDay activities.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (EditCondition = "!bIsPartOfDay"))
	float NonDailyDurationMinutes = 10.0f;
};

// Holds a pair mapping a villager identifier to an affection value.
USTRUCT(BlueprintType)
struct FApprovalEntry
{
	GENERATED_BODY() // Enables editing and serialization.

public:
	// Identifier tag representing the other villager.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	FGameplayTag VillagerIdTag;

	// Affection value where higher means more willing to trade generously.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	float AffectionValue = 0.0f;
};

// Encapsulates the social and trade-related setup for a villager.
USTRUCT(BlueprintType)
struct FSocialDefinition
{
	GENERATED_BODY() // Generates constructors and reflection data.

public:
	// Tag describing which resource this villager provides to others.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	FGameplayTag ProvidedResourceTag;

	// Curve mapping affection to quantity delivered during a trade.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	UCurveFloat* AffectionToQuantityCurve = nullptr;

	// List storing baseline affection toward other villagers.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	TArray<FApprovalEntry> Approvals;

	// Locations where this villager is available for trading.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	TArray<FTaggedLocation> TradeLocations;

	// Cooldown in seconds after completing a trade before new activities can begin.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	float PostTradeCooldownSeconds = 0.25f;

	// Amount by which the buyer's affection increases when trade succeeds.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	float BuyerAffectionGainOnTrade = 0.05f;

	// Amount by which the seller's affection toward the buyer increases.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	float SellerAffectionGainPerTrade = 0.025f;

	// Amount by which affection drops when a buyer misses the seller at a location.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Social")
	float AffectionLossOnMiss = 0.1f;
};

// Configurable movement settings per villager archetype.
USTRUCT(BlueprintType)
struct FMovementDefinition
{
	GENERATED_BODY() // Supplies reflection boilerplate.

public:
	// Desired walking speed in cm/s applied to the movement component.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float WalkSpeed = 200.0f;

	// Maximum acceleration applied to movement.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float MaxAcceleration = 1024.0f;

	// Acceptance radius for reaching destinations.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float AcceptanceRadius = 75.0f;
};

// Bundles all villager authoring data for easy reuse across instances.
UCLASS(BlueprintType)
class UVillagerArchetypeDataAsset : public UDataAsset
{
	GENERATED_BODY() // Adds UObject constructors and reflection metadata.

public:
	// Unique identifier for this villager instance used in logging and social interactions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager|Identity")
	FGameplayTag VillagerIdTag;

	// Collection of needs that define this villager's internal drives.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager|Needs")
	TArray<FNeedDefinition> NeedDefinitions;

	// Activities the villager can perform, including daily and reactive ones.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager|Activities")
	TArray<FActivityDefinition> ActivityDefinitions;

	// Social definition controlling approvals and resources.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager|Social")
	FSocialDefinition SocialDefinition;

	// Movement tuning parameters.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager|Movement")
	FMovementDefinition MovementDefinition;
};
