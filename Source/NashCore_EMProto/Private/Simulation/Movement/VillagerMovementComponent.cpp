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
// Enables dispatching callbacks on the game thread without deep recursion.
#include "Async/Async.h"
#pragma endregion EngineIncludes

// Local log category for movement diagnostics.
DEFINE_LOG_CATEGORY_STATIC(LogVillagerMovement, Log, All);

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
		UE_LOG(LogVillagerMovement, Warning, TEXT("Cannot request move: AI controller not resolved for %s"), *GetNameSafe(GetOwner()));
		DispatchMoveFinished(false); // Fail fast when controller is missing.
		return; // Abort request.
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()); // Access nav system.
	if (!NavSystem || !NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)) // Validate nav data.
	{
		UE_LOG(LogVillagerMovement, Warning, TEXT("Cannot request move: navigation data unavailable for %s"), *GetNameSafe(GetOwner()));
		DispatchMoveFinished(false); // Fail fast when nav mesh is missing.
		return; // Abort request.
	}

	ClearMoveDelegate(); // Ensure previous bindings are cleared.

	const float UseAcceptance = AcceptanceRadiusOverride > 0.0f ? AcceptanceRadiusOverride : MovementDefinition.AcceptanceRadius; // Choose radius.

	FAIMoveRequest MoveRequest(TargetTransform.GetLocation()); // Build move request from transform.
	MoveRequest.SetAcceptanceRadius(UseAcceptance); // Apply radius.
	MoveRequest.SetUsePathfinding(true); // Enable pathfinding.

	FNavPathSharedPtr OutPath; // Placeholder for generated path.
	const FPathFollowingRequestResult RequestResult = Controller->MoveTo(MoveRequest, &OutPath); // Issue move request.

	if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal) // Handle immediate success when already at destination.
	{
		DispatchMoveFinished(true); // Immediately signal success to caller.
		return; // Skip binding because nothing will move.
	}

	if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful) // Validate success.
	{
		ActiveRequestId = RequestResult.MoveId; // Cache request id.
		Controller->ReceiveMoveCompleted.AddDynamic(this, &UVillagerMovementComponent::HandleMoveCompleted); // Bind completion delegate.
	}
	else // Handle failure cases.
	{
		ActiveRequestId = FAIRequestID::InvalidRequest; // Reset invalid request id on failure.
		UE_LOG(LogVillagerMovement, Warning, TEXT("Move request failed for %s with result %s"), *GetNameSafe(GetOwner()), *UEnum::GetValueAsString(RequestResult.Code));
		DispatchMoveFinished(false); // Defer failure notification to avoid recursion.
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
			CharacterMovement->bOrientRotationToMovement = true; // Face direction of travel.
			CharacterMovement->RotationRate = FRotator(0.f, 540.f, 0.f); // Smooth turning speed.
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

	DispatchMoveFinished(bSuccess); // Notify caller on completion.
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

// Dispatches the pending delegate on the game thread while preventing re-entrancy.
void UVillagerMovementComponent::DispatchMoveFinished(bool bSuccess)
{
	if (!PendingDelegate.IsBound()) // Nothing to execute.
	{
		return;
	}

	const FOnVillagerMovementFinished DelegateCopy = PendingDelegate; // Copy to local to avoid invalidation.
	PendingDelegate.Unbind(); // Clear stored delegate to prevent duplicate calls.

	AsyncTask(ENamedThreads::GameThread, [DelegateCopy, bSuccess]() mutable // Defer to next tick to avoid recursive call chains.
	{
		if (DelegateCopy.IsBound())
		{
			DelegateCopy.Execute(bSuccess); // Notify caller of move result.
		}
	});
}
