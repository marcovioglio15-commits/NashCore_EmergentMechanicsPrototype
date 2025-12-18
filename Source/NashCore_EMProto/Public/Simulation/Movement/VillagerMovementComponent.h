// Prevents multiple inclusion of this movement component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides core UE types.
#include "CoreMinimal.h"
// Supplies base actor component functionality.
#include "Components/ActorComponent.h"
// Adds AI controller support for navigation.
#include "AIController.h"
// Provides path following result types required for move completion callbacks.
#include "Navigation/PathFollowingComponent.h"
// Imports shared movement configuration structs.
#include "Simulation/Data/VillagerDataAssets.h"
#pragma endregion Includes

// Forward declarations to satisfy UHT without full header dependencies.
struct FAIRequestID; // Declares AI move request identifier.
class AAIController; // Declares AI controller class.

// Generated header required for reflection.
#include "VillagerMovementComponent.generated.h"

// Delegate invoked when a move request finishes.
DECLARE_DELEGATE_OneParam(FOnVillagerMovementFinished, bool /*bSuccess*/);

// Component that wraps NavMesh-driven movement requests.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerMovementComponent : public UActorComponent
{
	GENERATED_BODY() // Enables reflection.

public:
	// Default constructor setting sensible navigation defaults.
	UVillagerMovementComponent();

	// Initializes controller references and applies movement tuning.
	virtual void BeginPlay() override;

	// Requests movement to a target transform and notifies on completion.
	void RequestMoveToLocation(const FTransform& TargetTransform, float AcceptanceRadiusOverride, FOnVillagerMovementFinished CompletionDelegate);

	// Returns the configured acceptance radius.
	float GetAcceptanceRadius() const;

	// Applies new movement tuning parameters.
	void ApplyMovementDefinition(const FMovementDefinition& Definition);

private:
	// Handles move completion events from the AI controller.
	UFUNCTION()
	void HandleMoveCompleted(FAIRequestID RequestId, EPathFollowingResult::Type Result);

	// Safely unsubscribes from the controller delegate.
	void ClearMoveDelegate();

	// Retrieves or caches the AI controller.
	AAIController* ResolveAIController();

	// Executes the pending delegate, deferring to avoid re-entrancy loops.
	void DispatchMoveFinished(bool bSuccess);

	// Cached movement definition settings.
	UPROPERTY(EditAnywhere, Category = "Villager")
	FMovementDefinition MovementDefinition;

	// Cached controller pointer.
	UPROPERTY()
	TObjectPtr<AAIController> CachedAIController;

	// Pending completion callback.
	FOnVillagerMovementFinished PendingDelegate;

	// Active request identifier to validate callbacks.
	FAIRequestID ActiveRequestId;
};
