// Prevent multiple inclusions of this activity component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides base engine utilities.
#include "CoreMinimal.h"
// Supplies the actor component base.
#include "Components/ActorComponent.h"
// Grants access to villager needs component declarations.
#include "Simulation/Needs/VillagerNeedsComponent.h"
// Imports movement component declarations for navigation requests.
#include "Simulation/Movement/VillagerMovementComponent.h"
// Imports social component declarations for resource negotiation.
#include "Simulation/Social/VillagerSocialComponent.h"
// Imports logging component declarations for on-screen logging.
#include "Simulation/Logging/VillagerLogComponent.h"
// Imports the clock subsystem declaration.
#include "Simulation/Time/VillageClockSubsystem.h"
// Imports location registry for resolving tagged destinations.
#include "Simulation/Locations/VillageLocationRegistry.h"
#pragma endregion Includes

// Generated header include required by Unreal.
#include "VillagerActivityComponent.generated.h"

// Forward declaration for actor pointers used in provider context.
class AActor;

// Represents the current activity runtime state.
USTRUCT(BlueprintType)
struct FActivityRuntimeState
{
	GENERATED_BODY() // Adds constructors and reflection data.

public:
	// Cached definition of the active activity.
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	FActivityDefinition Definition;

	// Tracks elapsed in-game minutes for curve sampling and durations.
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	float ElapsedMinutes = 0.0f;

	// Indicates whether the component is waiting for movement completion.
	UPROPERTY(BlueprintReadOnly, Category = "Activity")
	bool bWaitingForMovement = false;
};

// Captures runtime data about the provider used during resource acquisition.
struct FResourceProviderContext
{
	// Identifier tag for the provider villager.
	FGameplayTag ProviderIdTag;

	// Tag for the trade location where the provider can be met.
	FGameplayTag TradeLocationTag;

	// Transform of the trade location selected for the provider.
	FTransform TradeLocationTransform;

	// Provider social component used to negotiate resources.
	TWeakObjectPtr<UVillagerSocialComponent> ProviderSocialComponent;

	// Provider actor used to validate spatial presence.
	TWeakObjectPtr<AActor> ProviderActor;

	// Indicates whether the provider was present when selected.
	bool bWasPresentAtSelection = false;
};

// Component responsible for scheduling and executing villager activities.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerActivityComponent : public UActorComponent
{
	GENERATED_BODY() // Enables reflection and constructors.

public:
	// Constructor establishing safe defaults.
	UVillagerActivityComponent();

	// Initializes component references and subscribes to the clock.
	virtual void BeginPlay() override;

	// Starts the next planned activity based on day order and current time.
	void StartNextPlannedActivity();

	// Forces an activity by tag, used when satisfying urgent needs.
	void ForceActivityByTag(const FGameplayTag& ActivityTag);

	// Returns whether an activity is currently running.
	bool IsActivityActive() const;

	// Returns current runtime state.
	const FActivityRuntimeState& GetCurrentRuntime() const;

	// Sets the archetype asset to drive activity selection.
	void SetArchetype(UVillagerArchetypeDataAsset* InArchetype);

private:
	// Callback for the village clock minute tick to advance activity time.
	UFUNCTION()
	void OnMinuteTick(int32 Hour, int32 Minute);

	// Starts execution of a specific activity definition.
	void BeginActivity(const FActivityDefinition& Definition);

	// Handles movement completion before starting the activity.
	void HandleMovementFinished(bool bSuccess);

	// Handles movement completion for resource acquisition prior to activity execution.
	void HandleResourceMovementFinished(bool bSuccess);

	// Applies per-minute need deltas based on the activity curves.
	void ApplyNeedDeltasForMinute();

	// Determines whether the provider is present at the expected trade location.
	bool IsProviderAtTradeLocation(const FResourceProviderContext& ProviderContext) const;

	// Checks for urgent needs during PartOfDay activities to allow interruption.
	void RunNeedInterruptionCheck();

	// Determines the probability of forcing the satisfying activity for a need.
	float GetNeedForceProbability(const FNeedRuntimeState& NeededNeed) const;

	// Determines whether the current check should force a need-driven activity.
	bool ShouldForceNeedActivity(const FNeedRuntimeState& NeededNeed) const;

	// Completes the current activity and transitions back to scheduling.
	void CompleteCurrentActivity();

	// Attempts to pick an activity that satisfies the highest priority need, respecting force probability.
	bool TryStartNeedSatisfyingActivity(EVillagerNeedUrgency UrgencyThreshold);

	// Attempts to start the activity that satisfies the specified need.
	bool TryStartNeedSatisfyingActivity(const FNeedRuntimeState& NeededNeed);

	// Picks the next PartOfDay activity using day order and time windows.
	bool TryStartScheduledActivity();

	// Clears timers bound to the current activity.
	void ClearActivityTimers();

	// Resolves the location of a provider offering the requested resource.
	bool FindResourceProviderLocation(const FGameplayTag& ResourceTag, FResourceProviderContext& OutProviderContext) const;

	// Resolves the target transform for an activity, optionally via the location registry.
	bool ResolveActivityTransform(const FActivityDefinition& Definition, FTransform& OutTransform);

	// Starts movement toward the actual activity location after resource acquisition.
	void StartMovementToActivityLocation(FActivityDefinition Definition);

	// Handles provider absence when a resource fetch fails.
	void HandleProviderUnavailable();

	// Applies archetype-driven tuning such as trade cooldowns.
	void ApplyArchetypeTuning();

	// Resolves the urgency of the need satisfied by the current activity, if any.
	EVillagerNeedUrgency ResolveNeedUrgencyForCurrentActivity() const;

	// Resets cached provider data after a fetch attempt completes.
	void ResetProviderContext();

	// Returns whether an activity is blocked by a provider failure cooldown.
	bool IsActivityInProviderCooldown(const FGameplayTag& ActivityTag) const;

	// Cached pointer to the clock subsystem.
	UPROPERTY()
	TObjectPtr<UVillageClockSubsystem> ClockSubsystem;

	// Cached pointer to the needs component.
	UPROPERTY()
	TObjectPtr<UVillagerNeedsComponent> NeedsComponent;

	// Cached pointer to the movement component.
	UPROPERTY()
	TObjectPtr<UVillagerMovementComponent> MovementComponent;

	// Cached pointer to the social component.
	UPROPERTY()
	TObjectPtr<UVillagerSocialComponent> SocialComponent;

	// Cached pointer to the log component.
	UPROPERTY()
	TObjectPtr<UVillagerLogComponent> LogComponent;

	// Cached archetype data for activity definitions.
	UPROPERTY(EditAnywhere, Category = "Villager")
	TObjectPtr<UVillagerArchetypeDataAsset> Archetype;

	// Runtime state of the active activity.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	FActivityRuntimeState CurrentRuntimeState;

	// Tracks whether an activity is active.
	bool bHasActiveActivity;

	// Tracks whether the villager is currently fetching a resource prerequisite.
	bool bFetchingResource = false;

	// Cached transform for the destination of the active activity.
	FTransform CachedActivityTransform;

	// Cached context for the provider selected during resource fetch.
	FResourceProviderContext CachedProviderContext;

	// Indicates whether the cached activity transform is valid.
	bool bHasCachedActivityTransform = false;

	// Cached provider identifier used for logging during resource fetches.
	FGameplayTag CachedProviderIdTag;

	// Delay (in real seconds) before retrying activity selection after a movement failure.
	UPROPERTY(EditAnywhere, Category = "Villager")
	float MovementFailureRetryDelaySeconds = 1.0f;

	// Delay (in real seconds) after reaching a provider before moving to the activity location.
	UPROPERTY(EditAnywhere, Category = "Villager")
	float ResourceFetchCooldownSeconds = 0.25f;

	// Delay (in real seconds) after failing to meet a provider before retrying that activity.
	UPROPERTY(EditAnywhere, Category = "Villager")
	float ProviderFailureCooldownSeconds = 8.0f;

	// Distance tolerance to consider a provider present at their trade location.
	UPROPERTY(EditAnywhere, Category = "Villager")
	float TradePresenceTolerance = 200.0f;

	// Timer used to throttle retries when navigation fails.
	FTimerHandle MovementFailureRetryHandle;

	// Timer used to delay movement to activity after fetching resources.
	FTimerHandle ResourceCooldownHandle;

	// Tracks when provider failures occurred to gate retries per activity.
	TMap<FGameplayTag, double> LastProviderFailureTime;

	// Tracks when specific activities last failed to move, to avoid tight retry loops.
	TMap<FGameplayTag, double> LastMovementFailureTime;
};
