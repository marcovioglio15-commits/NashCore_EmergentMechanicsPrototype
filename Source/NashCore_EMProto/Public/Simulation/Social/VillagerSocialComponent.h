// Prevents multiple inclusion of the social component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides core engine definitions.
#include "CoreMinimal.h"
// Supplies the actor component base.
#include "Components/ActorComponent.h"
// Imports villager needs urgency enumeration for trade scaling.
#include "Simulation/Needs/VillagerNeedsComponent.h"
// Imports shared data asset definitions.
#include "Simulation/Data/VillagerDataAssets.h"
// Imports logging component declaration for affection logging.
#include "Simulation/Logging/VillagerLogComponent.h"
#pragma endregion Includes

// Generated header required for reflection.
#include "VillagerSocialComponent.generated.h"

// Component managing approvals and resource trades between villagers.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerSocialComponent : public UActorComponent
{
	GENERATED_BODY() // Enables reflection.

public:
	// Constructor establishing defaults.
	UVillagerSocialComponent();

	// Initializes approval maps from archetype data.
	virtual void BeginPlay() override;

	// Requests a resource amount based on affection and need urgency.
	float RequestResource(const FGameplayTag& RequesterId, const FGameplayTag& NeedTag, EVillagerNeedUrgency NeedUrgency);

	// Reduces affection toward another villager when a trade attempt fails to happen.
	void RegisterMissedTrade(const FGameplayTag& OtherVillagerId);

	// Sets or overrides the archetype used by this component.
	void SetArchetype(UVillagerArchetypeDataAsset* InArchetype);

	// Returns the resource this villager provides, if any.
	FGameplayTag GetProvidedResourceTag() const;

	// Returns available trade location tags for this villager.
	TArray<FGameplayTag> GetTradeLocationTags() const;

	// Returns the villager identifier tag from the archetype.
	FGameplayTag GetVillagerIdTag() const;

	// Returns a snapshot of current affection values keyed by villager id.
	TMap<FGameplayTag, float> GetAffectionSnapshot() const;

private:
	// Retrieves affection for a villager, inserting if missing.
	float GetOrAddAffection(const FGameplayTag& VillagerId);

	// Applies social rules after a successful trade.
	void ApplyTradeAffectionAdjustments(const FGameplayTag& RequesterId, EVillagerNeedUrgency NeedUrgency);

	// Rebuilds the affection map from archetype data.
	void RebuildAffectionFromArchetype();

	// Archetype asset containing social definitions.
	UPROPERTY(EditAnywhere, Category = "Villager")
	TObjectPtr<UVillagerArchetypeDataAsset> Archetype;

	// Map storing dynamic affection values keyed by villager id tags.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TMap<FGameplayTag, float> AffectionMap;

	// Optional log component used to emit affection updates.
	UPROPERTY()
	TObjectPtr<UVillagerLogComponent> LogComponent;
};
