// Includes the social component declaration.
#include "Simulation/Social/VillagerSocialComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to on-screen debug logging if desired.
#include "Engine/Engine.h"
#pragma endregion EngineIncludes

// Constructor configuring component defaults.
UVillagerSocialComponent::UVillagerSocialComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking to avoid per-frame overhead.
}

// Initializes affection map from the archetype approvals.
void UVillagerSocialComponent::BeginPlay()
{
	Super::BeginPlay(); // Preserve base initialization.

	RebuildAffectionFromArchetype(); // Build affection map.
}

// Requests a resource amount considering affection and urgency.
float UVillagerSocialComponent::RequestResource(const FGameplayTag& RequesterId, const FGameplayTag& NeedTag, EVillagerNeedUrgency NeedUrgency)
{
	if (!Archetype) // Ensure data exists.
	{
		return 0.0f; // Cannot supply resources without data.
	}

	const float Affection = GetOrAddAffection(RequesterId); // Fetch affection baseline.

	float Quantity = 0.0f; // Initialize quantity.

	if (Archetype->SocialDefinition.AffectionToQuantityCurve) // Check for curve.
	{
		Quantity = Archetype->SocialDefinition.AffectionToQuantityCurve->GetFloatValue(Affection); // Sample curve using affection.
	}
	else // Fallback when no curve is set.
	{
		Quantity = 1.0f + Affection; // Provide linear fallback scaling.
	}

	ApplyTradeAffectionAdjustments(RequesterId, NeedUrgency); // Update social states.

	return Quantity; // Return computed quantity.
}

// Applies a penalty when the buyer misses the seller.
void UVillagerSocialComponent::RegisterMissedTrade(const FGameplayTag& BuyerId)
{
	if (!Archetype) // Validate archetype presence.
	{
		return; // Abort if missing.
	}

	const float Current = GetOrAddAffection(BuyerId); // Fetch existing affection.
	const float NewValue = Current - Archetype->SocialDefinition.AffectionLossOnMiss; // Apply loss.
	AffectionMap.Add(BuyerId, NewValue); // Store updated value.
}

// Allows overriding the archetype asset at runtime.
void UVillagerSocialComponent::SetArchetype(UVillagerArchetypeDataAsset* InArchetype)
{
	Archetype = InArchetype; // Store new archetype.
	RebuildAffectionFromArchetype(); // Recompute affection from new data.
}

// Retrieves affection for a villager, creating an entry if absent.
float UVillagerSocialComponent::GetOrAddAffection(const FGameplayTag& VillagerId)
{
	if (float* Found = AffectionMap.Find(VillagerId)) // Check existing entry.
	{
		return *Found; // Return stored value.
	}

	AffectionMap.Add(VillagerId, 0.0f); // Seed default affection.

	return 0.0f; // Return default.
}

// Updates affection values following a successful trade.
void UVillagerSocialComponent::ApplyTradeAffectionAdjustments(const FGameplayTag& RequesterId, EVillagerNeedUrgency NeedUrgency)
{
	if (!Archetype) // Ensure data exists.
	{
		return; // Abort if missing.
	}

	const float UrgencyMultiplier = NeedUrgency == EVillagerNeedUrgency::Critical ? 2.0f : 1.0f; // Scale for criticality.

	const float CurrentBuyerAffection = GetOrAddAffection(RequesterId); // Get current affection.
	const float NewBuyerAffection = CurrentBuyerAffection + Archetype->SocialDefinition.BuyerAffectionGainOnTrade * UrgencyMultiplier; // Compute buyer gain.
	const float NewSellerAffection = NewBuyerAffection + Archetype->SocialDefinition.SellerAffectionGainPerTrade; // Compute seller gain stacking on buyer gain.
	AffectionMap.Add(RequesterId, NewSellerAffection); // Store combined affection.
}

// Rebuilds the affection map from the archetype approvals.
void UVillagerSocialComponent::RebuildAffectionFromArchetype()
{
	AffectionMap.Reset(); // Clear previous data.

	if (!Archetype) // Validate archetype.
	{
		return; // Abort if missing.
	}

	for (const FApprovalEntry& Approval : Archetype->SocialDefinition.Approvals) // Iterate defaults.
	{
		AffectionMap.Add(Approval.VillagerIdTag, Approval.AffectionValue); // Seed map.
	}
}
