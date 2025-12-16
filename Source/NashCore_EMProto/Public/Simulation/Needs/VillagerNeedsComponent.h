// Prevents multiple inclusions of the needs component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides core Unreal types and macros.
#include "CoreMinimal.h"
// Supplies actor component base functionality.
#include "Components/ActorComponent.h"
// Grants access to shared villager data definitions.
#include "Simulation/Data/VillagerDataAssets.h"
#pragma endregion Includes

// Generated header include for reflection support.
#include "VillagerNeedsComponent.generated.h"

// Enumerates need urgency bands derived from configured thresholds.
UENUM(BlueprintType)
enum class EVillagerNeedUrgency : uint8
{
	Satisfied, // Value is below mild threshold.
	Mild, // Value is above mild but below critical threshold.
	Critical // Value is above critical threshold.
};

// Captures runtime state for a single need with its static definition.
USTRUCT(BlueprintType)
struct FNeedRuntimeState
{
	GENERATED_BODY() // Adds reflection helpers.

public:
	// Tag copied from the definition for quick lookups.
	UPROPERTY(BlueprintReadOnly, Category = "Need")
	FGameplayTag NeedTag;

	// Current numeric value clamped within allowed bounds.
	UPROPERTY(BlueprintReadOnly, Category = "Need")
	float CurrentValue = 0.0f;

	// Copy of the static definition for access to thresholds and weight.
	UPROPERTY(BlueprintReadOnly, Category = "Need")
	FNeedDefinition Definition;
};

// Component responsible for tracking and evaluating villager needs.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerNeedsComponent : public UActorComponent
{
	GENERATED_BODY() // Enables UE reflection and constructors.

public:
	// Standard constructor enabling defaults.
	UVillagerNeedsComponent();

	// Initializes runtime state from the archetype asset at BeginPlay.
	virtual void BeginPlay() override;

	// Applies a delta to a need identified by tag, clamping to bounds.
	void ApplyNeedDelta(const FGameplayTag& NeedTag, float Delta);

	// Fetches the highest priority need meeting or exceeding the urgency threshold.
	bool GetHighestPriorityNeed(EVillagerNeedUrgency MinimumUrgency, FNeedRuntimeState& OutNeed) const;

	// Exposes the current need states.
	const TArray<FNeedRuntimeState>& GetRuntimeNeeds() const;

	// Allows external systems to read the archetype asset.
	UVillagerArchetypeDataAsset* GetArchetype() const;

	// Sets the archetype asset and rebuilds runtime needs.
	void SetArchetype(UVillagerArchetypeDataAsset* InArchetype);

private:
	// Creates runtime states from the configured archetype.
	void BuildRuntimeNeeds();

	// Converts a raw value into an urgency state using thresholds.
	EVillagerNeedUrgency EvaluateUrgency(const FNeedRuntimeState& Need) const;

	// Retrieves a runtime state by need tag.
	bool TryGetRuntimeNeed(const FGameplayTag& NeedTag, FNeedRuntimeState& OutNeed) const;

	// Archetype asset that defines needs and activities for this villager.
	UPROPERTY(EditAnywhere, Category = "Villager")
	TObjectPtr<UVillagerArchetypeDataAsset> Archetype;

	// Runtime list of needs with mutable values.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TArray<FNeedRuntimeState> RuntimeNeeds;
};
