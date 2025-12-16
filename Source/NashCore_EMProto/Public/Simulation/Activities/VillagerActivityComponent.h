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
#pragma endregion Includes

// Generated header include required by Unreal.
#include "VillagerActivityComponent.generated.h"

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

	// Applies per-minute need deltas based on the activity curves.
	void ApplyNeedDeltasForMinute();

	// Checks for critical needs during PartOfDay activities to allow interruption.
	void RunCriticalInterruptionCheck();

	// Completes the current activity and transitions based on needs.
	void CompleteCurrentActivity();

	// Attempts to pick an activity that satisfies the highest priority need.
	bool TryStartNeedSatisfyingActivity(EVillagerNeedUrgency UrgencyThreshold);

	// Picks the next PartOfDay activity using day order and time windows.
	bool TryStartScheduledActivity();

	// Clears timers bound to the current activity.
	void ClearActivityTimers();

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

	// Timer handle for periodic critical checks.
	FTimerHandle CriticalCheckTimerHandle;

	// Interval in seconds between critical checks during PartOfDay activities.
	UPROPERTY(EditAnywhere, Category = "Villager")
	float CriticalCheckIntervalSeconds;
};
