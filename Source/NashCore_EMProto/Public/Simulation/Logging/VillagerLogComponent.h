// Prevents multiple inclusion of the log component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides engine core types and macros.
#include "CoreMinimal.h"
// Supplies the base actor component functionality.
#include "Components/ActorComponent.h"
#pragma endregion Includes

// Generated header required by Unreal reflection.
#include "VillagerLogComponent.generated.h"

// Component that emits on-screen logs for simulation events.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerLogComponent : public UActorComponent
{
	GENERATED_BODY() // Enables reflection and constructors.

public:
	// Constructor disabling tick.
	UVillagerLogComponent();

	// Emits a formatted log line to the screen.
	void LogMessage(const FString& Message);
};
