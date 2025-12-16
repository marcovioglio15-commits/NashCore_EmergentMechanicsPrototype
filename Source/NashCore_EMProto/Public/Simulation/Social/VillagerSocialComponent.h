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

	// Reduces affection when a buyer misses the seller at a trade location.
	void RegisterMissedTrade(const FGameplayTag& BuyerId);

	// Sets or overrides the archetype used by this component.
	void SetArchetype(UVillagerArchetypeDataAsset* InArchetype);

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
};
