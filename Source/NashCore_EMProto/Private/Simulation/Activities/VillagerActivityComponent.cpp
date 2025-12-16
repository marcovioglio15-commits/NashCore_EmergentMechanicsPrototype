// Includes the activity component declaration.
#include "Simulation/Activities/VillagerActivityComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to world time for timer scheduling.
#include "Engine/World.h"
// Supplies timer manager functionality.
#include "TimerManager.h"
#pragma endregion EngineIncludes

// Default constructor configuring tick usage and defaults.
UVillagerActivityComponent::UVillagerActivityComponent()
	: bHasActiveActivity(false) // No activity at creation.
	, CriticalCheckIntervalSeconds(10.0f) // Default periodic check rate.
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking to respect performance constraints.
}

// Initializes component references and starts the first activity.
void UVillagerActivityComponent::BeginPlay()
{
	Super::BeginPlay(); // Preserve parent initialization.

	NeedsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerNeedsComponent>() : nullptr; // Cache needs component.
	MovementComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerMovementComponent>() : nullptr; // Cache movement component.
	SocialComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerSocialComponent>() : nullptr; // Cache social component.
	LogComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerLogComponent>() : nullptr; // Cache log component.

	if (!Archetype && NeedsComponent) // If archetype not set explicitly.
	{
		Archetype = NeedsComponent->GetArchetype(); // Pull from needs component.
	}

	if (UWorld* World = GetWorld()) // Validate world.
	{
		ClockSubsystem = World->GetSubsystem<UVillageClockSubsystem>(); // Cache clock subsystem.

		if (ClockSubsystem) // Ensure clock exists.
		{
			ClockSubsystem->OnMinuteChanged.AddDynamic(this, &UVillagerActivityComponent::OnMinuteTick); // Subscribe to minute tick.
		}
	}

	StartNextPlannedActivity(); // Begin initial activity.
}

// Tries to start the next planned activity based on schedule.
void UVillagerActivityComponent::StartNextPlannedActivity()
{
	if (TryStartNeedSatisfyingActivity(EVillagerNeedUrgency::Critical)) // Prioritize critical needs.
	{
		return; // Already started satisfying activity.
	}

	if (!TryStartScheduledActivity()) // Attempt to start scheduled PartOfDay activity.
	{
		if (TryStartNeedSatisfyingActivity(EVillagerNeedUrgency::Mild)) // Fallback to mild needs.
		{
			return; // Started mild need satisfier.
		}
	}
}

// Forces a specific activity by tag.
void UVillagerActivityComponent::ForceActivityByTag(const FGameplayTag& ActivityTag)
{
	if (!Archetype) // Validate archetype presence.
	{
		return; // Abort if missing.
	}

	for (const FActivityDefinition& Definition : Archetype->ActivityDefinitions) // Iterate definitions.
	{
		if (Definition.ActivityTag == ActivityTag) // Match by tag.
		{
			BeginActivity(Definition); // Start matched activity.
			return; // Stop searching.
		}
	}
}

// Returns whether an activity is active.
bool UVillagerActivityComponent::IsActivityActive() const
{
	return bHasActiveActivity; // Provide state flag.
}

// Returns the current runtime state reference.
const FActivityRuntimeState& UVillagerActivityComponent::GetCurrentRuntime() const
{
	return CurrentRuntimeState; // Provide runtime struct.
}

// Sets a new archetype reference for activity lookups.
void UVillagerActivityComponent::SetArchetype(UVillagerArchetypeDataAsset* InArchetype)
{
	Archetype = InArchetype; // Store archetype pointer.
}

// Handles per-minute updates from the clock.
void UVillagerActivityComponent::OnMinuteTick(int32 Hour, int32 Minute)
{
	if (!bHasActiveActivity) // Skip when idle.
	{
		return; // Nothing to process.
	}

	if (CurrentRuntimeState.Definition.bIsPartOfDay) // Handle scheduled activity timing.
	{
		if (Hour >= CurrentRuntimeState.Definition.PartOfDayWindow.AllowedEndHour || Hour < CurrentRuntimeState.Definition.PartOfDayWindow.AllowedStartHour) // Check time window exit.
		{
			CompleteCurrentActivity(); // End activity if outside window.
			return; // Exit early after completion.
		}
	}
	else // Handle duration-based activity.
	{
		const float Remaining = CurrentRuntimeState.Definition.NonDailyDurationMinutes - CurrentRuntimeState.ElapsedMinutes; // Compute remaining time.
		if (Remaining <= 0.0f) // Check completion.
		{
			CompleteCurrentActivity(); // End activity.
			return; // Exit after completion.
		}
	}

	ApplyNeedDeltasForMinute(); // Apply per-minute curves.

	CurrentRuntimeState.ElapsedMinutes += 1.0f; // Advance elapsed time.
}

// Starts executing a specific activity definition.
void UVillagerActivityComponent::BeginActivity(const FActivityDefinition& Definition)
{
	ClearActivityTimers(); // Reset timers from previous activity.

	CurrentRuntimeState.Definition = Definition; // Cache definition.
	CurrentRuntimeState.ElapsedMinutes = 0.0f; // Reset elapsed time.
	CurrentRuntimeState.bWaitingForMovement = false; // Reset movement wait flag.

	bHasActiveActivity = true; // Mark activity active.

	if (LogComponent) // Log start event.
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Starting activity: %s"), *Definition.ActivityTag.ToString())); // Emit log.
	}

	if (Definition.bRequiresSpecificLocation && MovementComponent) // Handle movement requirement.
	{
		CurrentRuntimeState.bWaitingForMovement = true; // Flag waiting state.
		MovementComponent->RequestMoveToLocation(Definition.ActivityLocation, MovementComponent->GetAcceptanceRadius(), FOnVillagerMovementFinished::CreateUObject(this, &UVillagerActivityComponent::HandleMovementFinished)); // Issue move request.
		return; // Delay activity execution until arrival.
	}

	if (CurrentRuntimeState.Definition.bIsPartOfDay) // Schedule critical checks for PartOfDay.
	{
		if (UWorld* World = GetWorld()) // Validate world.
		{
			World->GetTimerManager().SetTimer(CriticalCheckTimerHandle, this, &UVillagerActivityComponent::RunCriticalInterruptionCheck, CriticalCheckIntervalSeconds, true); // Start periodic checks.
		}
	}
}

// Handles completion of movement before performing the activity.
void UVillagerActivityComponent::HandleMovementFinished(bool bSuccess)
{
	CurrentRuntimeState.bWaitingForMovement = false; // Clear wait flag.

	if (!bSuccess) // Handle failed movement.
	{
		if (LogComponent) // Log failure.
		{
			LogComponent->LogMessage(TEXT("Movement failed, attempting next activity.")); // Emit failure log.
		}
		CompleteCurrentActivity(); // End activity and choose another.
		return; // Stop processing.
	}

	if (LogComponent) // Log arrival.
	{
		LogComponent->LogMessage(TEXT("Arrived at activity location.")); // Emit arrival log.
	}

	if (CurrentRuntimeState.Definition.bIsPartOfDay) // Setup critical checks again in case movement consumed time.
	{
		if (UWorld* World = GetWorld()) // Verify world.
		{
			World->GetTimerManager().SetTimer(CriticalCheckTimerHandle, this, &UVillagerActivityComponent::RunCriticalInterruptionCheck, CriticalCheckIntervalSeconds, true); // Start checks.
		}
	}
}

// Applies per-minute need deltas from the active activity curves.
void UVillagerActivityComponent::ApplyNeedDeltasForMinute()
{
	if (!NeedsComponent) // Ensure needs component exists.
	{
		return; // Abort if missing.
	}

	for (const TPair<FGameplayTag, UCurveFloat*>& Pair : CurrentRuntimeState.Definition.NeedCurves) // Iterate curves.
	{
		if (!Pair.Value) // Validate curve pointer.
		{
			continue; // Skip null entries.
		}

		const float Delta = Pair.Value->GetFloatValue(CurrentRuntimeState.ElapsedMinutes); // Sample delta at elapsed minutes.
		NeedsComponent->ApplyNeedDelta(Pair.Key, Delta); // Apply to needs component.
	}
}

// Periodically checks for critical needs during PartOfDay activities.
void UVillagerActivityComponent::RunCriticalInterruptionCheck()
{
	if (!bHasActiveActivity) // Skip if no activity.
	{
		return; // Nothing to do.
	}

	if (!CurrentRuntimeState.Definition.bIsPartOfDay) // Only PartOfDay should be interrupted.
	{
		return; // Skip non-PartOfDay.
	}

	if (TryStartNeedSatisfyingActivity(EVillagerNeedUrgency::Critical)) // Attempt to interrupt for critical need.
	{
		if (LogComponent) // Log interruption.
		{
			LogComponent->LogMessage(TEXT("Activity interrupted by critical need.")); // Emit log.
		}
	}
}

// Completes the current activity and transitions accordingly.
void UVillagerActivityComponent::CompleteCurrentActivity()
{
	ClearActivityTimers(); // Stop timers.

	bHasActiveActivity = false; // Mark activity inactive.

	if (LogComponent) // Log completion.
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Completed activity: %s"), *CurrentRuntimeState.Definition.ActivityTag.ToString())); // Emit log.
	}

	if (TryStartNeedSatisfyingActivity(EVillagerNeedUrgency::Mild)) // Try to satisfy any pending needs.
	{
		return; // Already started new activity.
	}

	StartNextPlannedActivity(); // Resume schedule.
}

// Attempts to start a need-satisfying activity based on urgency.
bool UVillagerActivityComponent::TryStartNeedSatisfyingActivity(EVillagerNeedUrgency UrgencyThreshold)
{
	if (!NeedsComponent || !Archetype) // Validate dependencies.
	{
		return false; // Cannot proceed.
	}

	FNeedRuntimeState NeededNeed; // Holder for selected need.
	if (!NeedsComponent->GetHighestPriorityNeed(UrgencyThreshold, NeededNeed)) // Evaluate needs.
	{
		return false; // No matching need.
	}

	for (const FActivityDefinition& Definition : Archetype->ActivityDefinitions) // Search activities.
	{
		if (Definition.ActivityTag == NeededNeed.Definition.SatisfyingActivityTag) // Match satisfier.
		{
			BeginActivity(Definition); // Start satisfying activity.
			if (LogComponent) // Log decision.
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Switching to satisfy need: %s"), *NeededNeed.NeedTag.ToString())); // Emit log.
			}
			return true; // Success.
		}
	}

	return false; // No activity found.
}

// Attempts to start the next scheduled PartOfDay activity.
bool UVillagerActivityComponent::TryStartScheduledActivity()
{
	if (!Archetype || !ClockSubsystem) // Validate dependencies.
	{
		return false; // Cannot schedule without data.
	}

	int32 CurrentHour = ClockSubsystem->GetCurrentHour(); // Fetch current hour.

	const TArray<FActivityDefinition>& Definitions = Archetype->ActivityDefinitions; // Alias definitions.

	TArray<FActivityDefinition> DailyDefinitions; // Container for daily activities.
	for (const FActivityDefinition& Definition : Definitions) // Filter activities.
	{
		if (Definition.bIsPartOfDay) // Include only daily ones.
		{
			DailyDefinitions.Add(Definition); // Add to list.
		}
	}

	DailyDefinitions.Sort([](const FActivityDefinition& A, const FActivityDefinition& B) { return A.DayOrder < B.DayOrder; }); // Sort by day order.

	for (const FActivityDefinition& Definition : DailyDefinitions) // Iterate sorted activities.
	{
		const bool bWithinWindow = CurrentHour >= Definition.PartOfDayWindow.AllowedStartHour && CurrentHour < Definition.PartOfDayWindow.AllowedEndHour; // Check time window.

		if (bWithinWindow) // Start the first suitable activity.
		{
			BeginActivity(Definition); // Begin activity.
			return true; // Indicate success.
		}
	}

	if (DailyDefinitions.Num() > 0) // Fallback when none match time window.
	{
		BeginActivity(DailyDefinitions[0]); // Start the first daily activity even if off-window.
		return true; // Indicate success.
	}

	return false; // No activities available.
}

// Clears timers associated with the current activity.
void UVillagerActivityComponent::ClearActivityTimers()
{
	if (UWorld* World = GetWorld()) // Validate world.
	{
		World->GetTimerManager().ClearTimer(CriticalCheckTimerHandle); // Clear critical check timer.
	}
}
