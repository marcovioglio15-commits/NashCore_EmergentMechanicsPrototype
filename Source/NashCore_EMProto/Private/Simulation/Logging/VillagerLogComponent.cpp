// Includes the log component declaration.
#include "Simulation/Logging/VillagerLogComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to engine-wide logging utilities.
#include "Engine/Engine.h"
#pragma endregion EngineIncludes
// Region: Gameplay tags.
#pragma region GameplayTagIncludes
#include "GameplayTagContainer.h"
#pragma endregion GameplayTagIncludes

// Constructor ensuring no per-frame ticking.
UVillagerLogComponent::UVillagerLogComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking to meet performance constraints.
}

// Global toggle for on-screen debug rendering.
bool UVillagerLogComponent::bOnScreenDebugEnabled = true;

// Emits a message to the on-screen debug feed.
void UVillagerLogComponent::LogMessage(const FString& Message)
{
	LogAction(Message, FGameplayTag()); // Delegate to tagged logging.
}

// Enables or disables on-screen debug rendering for all villager log components.
void UVillagerLogComponent::SetOnScreenDebugEnabled(bool bEnabled)
{
	bOnScreenDebugEnabled = bEnabled; // Cache the global flag for subsequent log calls.
}

// Returns whether on-screen debug rendering is currently enabled.
bool UVillagerLogComponent::IsOnScreenDebugEnabled()
{
	return bOnScreenDebugEnabled; // Provide the current global flag value.
}

// Emits a log line, prefixing it with the villager identifier and an optional target.
void UVillagerLogComponent::LogAction(const FString& ActionDescription, const FGameplayTag& TargetVillagerTag)
{
	const FString ActorLabel = GetShortTagString(VillagerIdTag);
	const FString TargetSuffix = TargetVillagerTag.IsValid() ? FString::Printf(TEXT(" -> [%s]"), *GetShortTagString(TargetVillagerTag)) : FString();
	const FString ComposedMessage = FString::Printf(TEXT("[%s]%s %s"), *ActorLabel, *TargetSuffix, *ActionDescription);

	if (GEngine && bOnScreenDebugEnabled) // Verify engine availability and debug setting.
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, ComposedMessage); // Draw message for a short duration.
	}

	RecentMessages.Add(ComposedMessage); // Buffer the message for UI.

	if (RecentMessages.Num() > MaxStoredMessages) // Trim old entries.
	{
		RecentMessages.RemoveAt(0);
	}

	OnLogLineAdded.Broadcast(this, ComposedMessage); // Notify UI listeners with source context.
}

// Returns the resolved log text color for this villager.
FLinearColor UVillagerLogComponent::GetResolvedLogTextColor() const
{
	if (!bAutoAssignColorFromId) // Respect explicit color configuration.
	{
		return LogTextColor; // Return the configured color value.
	}

	if (!bHasCachedAutoColor) // Compute and cache the auto color once.
	{
		const FString SourceId = GetShortTagString(VillagerIdTag); // Choose a stable identifier string.
		const uint32 Hash = GetTypeHash(SourceId); // Hash the identifier for deterministic color generation.
		const uint8 Hue = static_cast<uint8>(Hash % 255); // Derive hue from the hash.
		CachedAutoColor = FLinearColor::MakeFromHSV8(Hue, 200, 255); // Build a vivid HSV color from the hue.
		bHasCachedAutoColor = true; // Mark the auto color as cached.
	}

	return CachedAutoColor; // Return the cached auto-assigned color.
}

// Returns the configured log font size for this villager.
int32 UVillagerLogComponent::GetLogFontSize() const
{
	return LogFontSize; // Return the configured font size.
}

// Sets the villager identifier used to prefix log messages.
void UVillagerLogComponent::SetVillagerIdTag(const FGameplayTag& InVillagerId)
{
	VillagerIdTag = InVillagerId; // Store the new villager identifier tag. 
	bHasCachedAutoColor = false; // Reset cached color to reflect the new identifier. 
}

// Returns a shortened tag string without parent prefixes.
FString UVillagerLogComponent::GetShortTagString(const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
	{
		return TEXT("Unknown");
	}

	const FString FullString = Tag.ToString();
	int32 LastDotIndex = INDEX_NONE;
	if (FullString.FindLastChar(TEXT('.'), LastDotIndex))
	{
		return FullString.Mid(LastDotIndex + 1);
	}

	return FullString;
}
