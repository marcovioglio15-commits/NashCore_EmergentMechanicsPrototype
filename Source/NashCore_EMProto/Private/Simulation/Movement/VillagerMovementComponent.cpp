// Includes the movement component declaration.
#include "Simulation/Movement/VillagerMovementComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to character movement tuning.
#include "GameFramework/Character.h"
// Exposes character movement component for speed and acceleration settings.
#include "GameFramework/CharacterMovementComponent.h"
// Supplies path following result structures for move completion callbacks.
#include "Navigation/PathFollowingComponent.h"
// Provides pathfinding request helpers.
#include "NavigationSystem.h"
#pragma endregion EngineIncludes

// Constructor setting default component properties.
UVillagerMovementComponent::UVillagerMovementComponent()
	: PendingDelegate() // Initialize delegate.
	, ActiveRequestId(FAIRequestID::InvalidRequest) // No active move at start.
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking per performance requirements.
}

// Applies movement tuning and caches controller on begin play.
void UVillagerMovementComponent::BeginPlay()
{
	Super::BeginPlay(); // Preserve base behavior.

	ApplyMovementDefinition(MovementDefinition); // Apply configured movement data.

	ResolveAIController(); // Cache controller for future move requests.
}

// Requests navigation to a specific transform.
void UVillagerMovementComponent::RequestMoveToLocation(const FTransform& TargetTransform, float AcceptanceRadiusOverride, FOnVillagerMovementFinished CompletionDelegate)
{
	PendingDelegate = CompletionDelegate; // Store delegate for later execution.

	AAIController* Controller = ResolveAIController(); // Fetch controller.

	if (!Controller) // Validate controller availability.
	{
		if (PendingDelegate.IsBound()) // Check for a bound delegate.
		{
			PendingDelegate.Execute(false); // Signal failure due to missing controller.
		}
		return; // Abort request.
	}

	ClearMoveDelegate(); // Ensure previous bindings are cleared.

	const float UseAcceptance = AcceptanceRadiusOverride > 0.0f ? AcceptanceRadiusOverride : MovementDefinition.AcceptanceRadius; // Choose radius.

	FAIMoveRequest MoveRequest(TargetTransform.GetLocation()); // Build move request from transform.
	MoveRequest.SetAcceptanceRadius(UseAcceptance); // Apply radius.
	MoveRequest.SetUsePathfinding(true); // Enable pathfinding.

	FNavPathSharedPtr OutPath; // Placeholder for generated path.
	const FPathFollowingRequestResult RequestResult = Controller->MoveTo(MoveRequest, &OutPath); // Issue move request.

	if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful) // Validate success.
	{
		ActiveRequestId = RequestResult.MoveId; // Cache request id.
		Controller->ReceiveMoveCompleted.AddDynamic(this, &UVillagerMovementComponent::HandleMoveCompleted); // Bind completion delegate.
	}
	else // Handle failure cases.
	{
		if (PendingDelegate.IsBound()) // Check bound delegate.
		{
			PendingDelegate.Execute(false); // Signal failure.
		}
	}
}

// Returns the configured acceptance radius.
float UVillagerMovementComponent::GetAcceptanceRadius() const
{
	return MovementDefinition.AcceptanceRadius; // Provide cached radius.
}

// Applies new movement definition settings to the owning pawn.
void UVillagerMovementComponent::ApplyMovementDefinition(const FMovementDefinition& Definition)
{
	MovementDefinition = Definition; // Store definition.

	if (ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner())) // Check for character owner.
	{
		if (UCharacterMovementComponent* CharacterMovement = CharacterOwner->GetCharacterMovement()) // Get movement component.
		{
			CharacterMovement->MaxWalkSpeed = MovementDefinition.WalkSpeed; // Apply speed.
			CharacterMovement->MaxAcceleration = MovementDefinition.MaxAcceleration; // Apply acceleration.
		}
	}
}

// Handles move completion from the AI controller.
void UVillagerMovementComponent::HandleMoveCompleted(FAIRequestID RequestId, EPathFollowingResult::Type Result)
{
	if (RequestId != ActiveRequestId) // Ignore unrelated requests.
	{
		return; // Early exit for mismatched ids.
	}

	const bool bSuccess = Result == EPathFollowingResult::Success; // Determine success flag.

	ClearMoveDelegate(); // Remove binding to avoid duplicate notifications.

	ActiveRequestId = FAIRequestID::InvalidRequest; // Reset active id.

	if (PendingDelegate.IsBound()) // Check for bound callback.
	{
		PendingDelegate.Execute(bSuccess); // Execute callback with success flag.
	}
}

// Removes the move completed delegate binding safely.
void UVillagerMovementComponent::ClearMoveDelegate()
{
	if (CachedAIController) // Ensure controller exists.
	{
		CachedAIController->ReceiveMoveCompleted.RemoveDynamic(this, &UVillagerMovementComponent::HandleMoveCompleted); // Remove binding.
	}
}

// Resolves and caches the AI controller for the owning pawn.
AAIController* UVillagerMovementComponent::ResolveAIController()
{
	if (CachedAIController) // Return cached controller if valid.
	{
		return CachedAIController;
	}

	APawn* PawnOwner = Cast<APawn>(GetOwner()); // Cast owner to pawn.

	if (!PawnOwner) // Validate pawn.
	{
		return nullptr; // Cannot resolve controller.
	}

	CachedAIController = Cast<AAIController>(PawnOwner->GetController()); // Cache AI controller.

	return CachedAIController; // Return controller (may be null).
}
