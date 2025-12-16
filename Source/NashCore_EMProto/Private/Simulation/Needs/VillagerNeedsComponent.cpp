// Includes the corresponding header for implementation.
#include "Simulation/Needs/VillagerNeedsComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Adds logging utilities for debug output if needed.
#include "Engine/Engine.h"
#pragma endregion EngineIncludes

// Default constructor configuring component defaults.
UVillagerNeedsComponent::UVillagerNeedsComponent()
{
	// Disable ticking to avoid per-frame work as per performance guidelines.
	PrimaryComponentTick.bCanEverTick = false;
}

// BeginPlay initializes runtime needs from the archetype asset.
void UVillagerNeedsComponent::BeginPlay()
{
	Super::BeginPlay(); // Preserve parent initialization.

	BuildRuntimeNeeds(); // Instantiate runtime needs.
}

// Applies a delta to the specified need and clamps it within bounds.
void UVillagerNeedsComponent::ApplyNeedDelta(const FGameplayTag& NeedTag, float Delta)
{
	for (FNeedRuntimeState& NeedState : RuntimeNeeds) // Iterate through runtime needs.
	{
		if (NeedState.NeedTag == NeedTag) // Match by tag.
		{
			const float NewValue = NeedState.CurrentValue + Delta; // Compute unclamped value.
			NeedState.CurrentValue = FMath::Clamp(NewValue, NeedState.Definition.MinValue, NeedState.Definition.MaxValue); // Clamp to configured range.
			return; // Exit after applying the delta.
		}
	}
}

// Returns the highest priority need meeting the required urgency.
bool UVillagerNeedsComponent::GetHighestPriorityNeed(EVillagerNeedUrgency MinimumUrgency, FNeedRuntimeState& OutNeed) const
{
	float BestWeight = -1.0f; // Tracks the highest priority found.
	bool bFound = false; // Tracks whether a candidate exists.

	for (const FNeedRuntimeState& NeedState : RuntimeNeeds) // Iterate through needs.
	{
		const EVillagerNeedUrgency Urgency = EvaluateUrgency(NeedState); // Compute urgency.

		if (Urgency < MinimumUrgency) // Skip needs below the requested urgency.
		{
			continue; // Continue to next need.
		}

		const float Weight = NeedState.Definition.PriorityWeight; // Retrieve priority weight.

		if (Weight > BestWeight) // Select the best weight.
		{
			BestWeight = Weight; // Update best weight.
			OutNeed = NeedState; // Copy the candidate out.
			bFound = true; // Mark as found.
		}
	}

	return bFound; // Indicate whether a suitable need was found.
}

// Provides read-only access to runtime needs.
const TArray<FNeedRuntimeState>& UVillagerNeedsComponent::GetRuntimeNeeds() const
{
	return RuntimeNeeds; // Return the cached array.
}

// Returns the archetype pointer for external use.
UVillagerArchetypeDataAsset* UVillagerNeedsComponent::GetArchetype() const
{
	return Archetype; // Provide stored archetype.
}

// Sets a new archetype and rebuilds runtime data.
void UVillagerNeedsComponent::SetArchetype(UVillagerArchetypeDataAsset* InArchetype)
{
	Archetype = InArchetype; // Store provided asset.
	BuildRuntimeNeeds(); // Recreate runtime state.
}

// Builds runtime need states based on the archetype definitions.
void UVillagerNeedsComponent::BuildRuntimeNeeds()
{
	RuntimeNeeds.Reset(); // Clear previous state.

	if (!Archetype) // Validate the archetype asset.
	{
		return; // Abort if no data present.
	}

	for (const FNeedDefinition& Definition : Archetype->NeedDefinitions) // Iterate definitions.
	{
		FNeedRuntimeState RuntimeState; // Create runtime entry.
		RuntimeState.NeedTag = Definition.NeedTag; // Copy tag.
		RuntimeState.Definition = Definition; // Copy static definition.
		RuntimeState.CurrentValue = FMath::Clamp(Definition.StartingValue, Definition.MinValue, Definition.MaxValue); // Initialize value within bounds.
		RuntimeNeeds.Add(RuntimeState); // Add to array.
	}
}

// Computes need urgency based on normalized value and thresholds.
EVillagerNeedUrgency UVillagerNeedsComponent::EvaluateUrgency(const FNeedRuntimeState& Need) const
{
	const float Range = FMath::Max(KINDA_SMALL_NUMBER, Need.Definition.MaxValue - Need.Definition.MinValue); // Avoid division by zero.
	const float Normalized = (Need.CurrentValue - Need.Definition.MinValue) / Range; // Normalize to 0-1.

	if (Normalized >= Need.Definition.Thresholds.CriticalThreshold) // Check critical band.
	{
		return EVillagerNeedUrgency::Critical; // Mark as critical.
	}

	if (Normalized >= Need.Definition.Thresholds.MildThreshold) // Check mild band.
	{
		return EVillagerNeedUrgency::Mild; // Mark as mild.
	}

	return EVillagerNeedUrgency::Satisfied; // Default to satisfied.
}

// Attempts to get a runtime need by tag.
bool UVillagerNeedsComponent::TryGetRuntimeNeed(const FGameplayTag& NeedTag, FNeedRuntimeState& OutNeed) const
{
	for (const FNeedRuntimeState& NeedState : RuntimeNeeds) // Iterate runtime collection.
	{
		if (NeedState.NeedTag == NeedTag) // Match tag.
		{
			OutNeed = NeedState; // Copy state out.
			return true; // Indicate success.
		}
	}

	return false; // No matching need found.
}
