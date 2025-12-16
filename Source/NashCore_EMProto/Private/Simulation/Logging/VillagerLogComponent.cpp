// Includes the log component declaration.
#include "Simulation/Logging/VillagerLogComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to engine-wide logging utilities.
#include "Engine/Engine.h"
#pragma endregion EngineIncludes

// Constructor ensuring no per-frame ticking.
UVillagerLogComponent::UVillagerLogComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking to meet performance constraints.
}

// Emits a message to the on-screen debug feed.
void UVillagerLogComponent::LogMessage(const FString& Message)
{
	if (GEngine) // Verify engine availability.
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, Message); // Draw message for a short duration.
	}
}
