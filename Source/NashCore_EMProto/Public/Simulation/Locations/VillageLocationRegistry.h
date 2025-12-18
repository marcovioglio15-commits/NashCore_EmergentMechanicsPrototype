// Prevents multiple inclusion of the location registry header.
#pragma once

// Region: Includes.
#pragma region Includes
// Core types and macros.
#include "CoreMinimal.h"
// Base world subsystem for per-world lifetime.
#include "Subsystems/WorldSubsystem.h"
// Gameplay tags for keyed lookup.
#include "GameplayTagContainer.h"
#pragma endregion Includes

// Generated header required for reflection.
#include "VillageLocationRegistry.generated.h"

// Forward declare the tagged location actor.
class ATaggedLocationActor;

// Subsystem that discovers tagged location actors in the world and exposes lookups.
UCLASS()
class UVillageLocationRegistry : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// Ensure creation for all worlds.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// Build the registry when the subsystem initializes.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Rebuilds the registry by scanning the world.
	void RefreshRegistry();

	// Attempts to fetch a transform for the supplied tag; returns false if not found or invalid.
	bool TryGetLocation(const FGameplayTag& LocationTag, FTransform& OutTransform);

	// Returns a copy of all registered locations (used for debugging or UI).
	TMap<FGameplayTag, FTransform> GetRegisteredLocations() const { return RegisteredLocations; }

private:
	// Adds a location to the registry, projecting to the navmesh if possible.
	void AddFromActor(const ATaggedLocationActor* Actor);

	// Cached mapping of tag -> transform.
	UPROPERTY()
	TMap<FGameplayTag, FTransform> RegisteredLocations;
};
