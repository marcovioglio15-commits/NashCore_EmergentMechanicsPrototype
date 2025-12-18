// Prevents multiple inclusion of the HUD actor header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides base types and macros.
#include "CoreMinimal.h"
// Supplies the base HUD class so this can be assigned in GameMode HUD slots.
#include "GameFramework/HUD.h"
#pragma endregion Includes

// Forward declarations to reduce coupling.
class UUserWidget; // Forward-declared base widget class.
class UVillageStatusWidget; // Forward-declared status widget class.
class UVillagerLogComponent; // Forward-declared log component class.

// Generated header required by Unreal reflection.
#include "VillageHUDActor.generated.h"

// HUD that spawns and wires up the village status widget at begin play.
UCLASS(Blueprintable)
class AVillageHUDActor : public AHUD
{
	GENERATED_BODY() // Enables reflection.

public:
	// Constructor disabling ticking.
	AVillageHUDActor();

protected:
	// Spawns the widget and wires up data sources.
	virtual void BeginPlay() override;

	// Cleans up the widget when the actor is destroyed.
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Widget class to instantiate; defaults to UVillageStatusWidget if unset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village UI", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UUserWidget> WidgetClass; // Stores the widget class to spawn at runtime.

	// Optional villager actor to pull the log component from.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> PreferredVillager;

	// Whether to automatically find the first villager log component if PreferredVillager is unset.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Village UI", meta = (AllowPrivateAccess = "true"))
	bool bAutoFindVillager = true;

private:
	// Attempts to locate a villager log component from the preferred actor or the level.
	UVillagerLogComponent* ResolveLogComponent() const;

	// Stored widget instance for cleanup.
	UPROPERTY()
	TObjectPtr<UUserWidget> ActiveWidget; // Holds the spawned widget instance for cleanup.
};
