// Prevents multiple inclusion of the tagged location actor header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides base types.
#include "CoreMinimal.h"
// Supplies the base actor interface.
#include "GameFramework/Actor.h"
// Exposes gameplay tags for semantic lookup.
#include "GameplayTagContainer.h"
#pragma endregion Includes

// Generated header required by Unreal reflection.
#include "TaggedLocationActor.generated.h"

// Simple actor that marks a world-space transform with a gameplay tag for lookup.
UCLASS(Blueprintable)
class ATaggedLocationActor : public AActor
{
	GENERATED_BODY() // Enables reflection and construction.

public:
	// Constructor ensuring tick is disabled.
	ATaggedLocationActor();

	// Returns the tagged transform to use for navigation or activities.
	FTransform GetTaggedTransform() const { return GetActorTransform(); }

	// Public tag exposed in the editor and blueprints.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tagged Location")
	FGameplayTag LocationTag;
};
