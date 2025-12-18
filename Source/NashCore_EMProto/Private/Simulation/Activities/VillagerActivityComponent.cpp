// Includes the activity component declaration.
#include "Simulation/Activities/VillagerActivityComponent.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides access to world time for timer scheduling.
#include "Engine/World.h"
// Supplies timer manager functionality.
#include "TimerManager.h"
// Provides actor iteration utilities for provider lookup.
#include "EngineUtils.h"
// Exposes actor accessors for provider presence validation.
#include "GameFramework/Actor.h"
#pragma endregion EngineIncludes

// Default constructor configuring tick usage and defaults.
UVillagerActivityComponent::UVillagerActivityComponent()
	: bHasActiveActivity(false) // No activity at creation.
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

	ApplyArchetypeTuning(); // Apply archetype-driven tuning such as trade cooldowns. 

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
	ApplyArchetypeTuning(); // Pull archetype-driven tuning into component state. 
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

	if (CurrentRuntimeState.Definition.bIsPartOfDay) // Continuously check for need-driven interruptions.
	{
		RunNeedInterruptionCheck(); // Evaluate interruption chance after this minute update.
	}
}

// Starts executing a specific activity definition.
void UVillagerActivityComponent::BeginActivity(const FActivityDefinition& Definition)
{
	ClearActivityTimers(); // Reset timers from previous activity.

	if (IsActivityInProviderCooldown(Definition.ActivityTag)) // Guard against retrying activities during provider cooldown. 
	{
		bHasActiveActivity = false; // Mark activity inactive. 
		if (LogComponent) // Log the skip for clarity. 
		{
			LogComponent->LogMessage(FString::Printf(TEXT("Delaying activity %s due to provider cooldown."), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag))); // Emit cooldown delay log. 
		}
		if (UWorld* World = GetWorld()) // Schedule a retry after the cooldown expires. 
		{
			World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, ProviderFailureCooldownSeconds, false); // Queue schedule retry after provider cooldown. 
		}
		return; // Exit without starting the activity. 
	}

	CurrentRuntimeState.Definition = Definition; // Cache definition.
	CurrentRuntimeState.ElapsedMinutes = 0.0f; // Reset elapsed time.
	CurrentRuntimeState.bWaitingForMovement = false; // Reset movement wait flag.
	bFetchingResource = false; // Reset resource fetching flag.
	bHasCachedActivityTransform = false; // Clear cached transform flag.
	CachedProviderIdTag = FGameplayTag(); // Clear provider id cache.
	ResetProviderContext(); // Clear cached provider context for fresh selection. 

	bHasActiveActivity = true; // Mark activity active.

	// Ensure the location can be resolved before logging/starting movement.
	if (Definition.bRequiresSpecificLocation && MovementComponent)
	{
		FTransform Dummy;
		if (!ResolveActivityTransform(Definition, Dummy))
		{
			if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Activity %s has no valid location; skipping."), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag)));
			}
			bHasActiveActivity = false;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, MovementFailureRetryDelaySeconds, false);
			}
			return;
		}
		CachedActivityTransform = Dummy; // Cache the resolved transform.
		bHasCachedActivityTransform = true; // Mark cache valid.
	}

	if (LogComponent) // Log start event.
	{
		const FString LocationInfo = Definition.bRequiresSpecificLocation
			? FString::Printf(TEXT(" at %s"), *UVillagerLogComponent::GetShortTagString(Definition.ActivityLocationTag))
			: FString();
		LogComponent->LogMessage(FString::Printf(TEXT("Starting activity: %s%s"), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag), *LocationInfo)); // Emit log with context.
	}

	if (Definition.bRequiresSpecificLocation && MovementComponent) // Handle movement requirement.
	{
		// Fetch prerequisite resources before moving to the activity location.
		if (Definition.RequiredResourceTag.IsValid())
		{
			FResourceProviderContext ProviderContext; // Allocate provider context holder. 
			if (FindResourceProviderLocation(Definition.RequiredResourceTag, ProviderContext)) // Attempt to resolve a provider location. 
			{
				bFetchingResource = true; // Mark resource acquisition in progress.
				CachedProviderContext = ProviderContext; // Cache provider context for availability checks. 
				CachedProviderIdTag = ProviderContext.ProviderIdTag; // Cache provider id for logging.
				CurrentRuntimeState.bWaitingForMovement = true; // Flag waiting.

				if (LogComponent)
				{
					LogComponent->LogMessage(FString::Printf(TEXT("Fetching resource %s from %s at %s before %s."),
						*UVillagerLogComponent::GetShortTagString(Definition.RequiredResourceTag),
						*UVillagerLogComponent::GetShortTagString(ProviderContext.ProviderIdTag),
						*UVillagerLogComponent::GetShortTagString(ProviderContext.TradeLocationTag),
						*UVillagerLogComponent::GetShortTagString(Definition.ActivityTag)));
				}

				MovementComponent->RequestMoveToLocation(ProviderContext.TradeLocationTransform, MovementComponent->GetAcceptanceRadius(), FOnVillagerMovementFinished::CreateUObject(this, &UVillagerActivityComponent::HandleResourceMovementFinished)); // Move to provider.
				return; // Wait for resource fetch completion.
			}
			else if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("No provider found for %s; proceeding to %s without fetch."),
					*UVillagerLogComponent::GetShortTagString(Definition.RequiredResourceTag),
					*UVillagerLogComponent::GetShortTagString(Definition.ActivityTag)));
			}
		}

		StartMovementToActivityLocation(Definition); // Move directly to the activity location.
		return; // Delay activity execution until arrival.
	}

	if (!Definition.bRequiresSpecificLocation) // Handle activities without locations.
	{
		CurrentRuntimeState.bWaitingForMovement = false; // No movement required.
	}

}

// Handles completion of movement before performing the activity.
void UVillagerActivityComponent::HandleMovementFinished(bool bSuccess)
{
	CurrentRuntimeState.bWaitingForMovement = false; // Clear wait flag.

	if (!bSuccess) // Handle failed movement.
	{
		ClearActivityTimers(); // Stop any active timers before retrying selection.
		bHasActiveActivity = false; // Mark as idle to prevent immediate re-entry.

		if (LogComponent) // Log failure.
		{
			LogComponent->LogMessage(FString::Printf(TEXT("Movement failed for activity %s, retrying selection after %.1f seconds."), *UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.ActivityTag), MovementFailureRetryDelaySeconds)); // Emit failure log with actor context.
		}

		LastMovementFailureTime.FindOrAdd(CurrentRuntimeState.Definition.ActivityTag) = FPlatformTime::Seconds(); // Remember failure time to avoid tight loops.

		if (UWorld* World = GetWorld()) // Schedule a delayed retry to avoid tight loops.
		{
			World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, MovementFailureRetryDelaySeconds, false);
		}

		return; // Stop processing.
	}

	if (LogComponent) // Log arrival.
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Arrived at activity location for %s."), *UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.ActivityTag))); // Emit arrival log with actor context.
	}

	LastMovementFailureTime.Remove(CurrentRuntimeState.Definition.ActivityTag); // Clear failure record on success.

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

// Continuously checks for need-driven interruptions during PartOfDay activities.
void UVillagerActivityComponent::RunNeedInterruptionCheck()
{
	if (!bHasActiveActivity) // Skip if no activity.
	{
		return; // Nothing to do.
	}

	if (CurrentRuntimeState.bWaitingForMovement) // Avoid interruptions while moving to the activity.
	{
		return; // Defer checks until movement is complete.
	}

	if (!CurrentRuntimeState.Definition.bIsPartOfDay || !NeedsComponent) // Only PartOfDay activities can be interrupted.
	{
		return; // Skip non-PartOfDay or missing dependencies.
	}

	FNeedRuntimeState CriticalNeed;
	if (NeedsComponent->GetHighestPriorityNeed(EVillagerNeedUrgency::Critical, CriticalNeed)) // Check critical needs first.
	{
		if (ShouldForceNeedActivity(CriticalNeed) && TryStartNeedSatisfyingActivity(CriticalNeed))
		{
			if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Activity interrupted by need: %s"), *UVillagerLogComponent::GetShortTagString(CriticalNeed.NeedTag)));
			}
		}
		return; // Do not consider mild needs when a critical one exists.
	}

	FNeedRuntimeState MildNeed;
	if (NeedsComponent->GetHighestPriorityNeed(EVillagerNeedUrgency::Mild, MildNeed)) // Fall back to mild needs.
	{
		if (ShouldForceNeedActivity(MildNeed) && TryStartNeedSatisfyingActivity(MildNeed))
		{
			if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Activity interrupted by need: %s"), *UVillagerLogComponent::GetShortTagString(MildNeed.NeedTag)));
			}
		}
	}
}

// Computes the probability of forcing a need-driven activity.
float UVillagerActivityComponent::GetNeedForceProbability(const FNeedRuntimeState& NeededNeed) const
{
	if (!NeededNeed.Definition.ForceActivityProbabilityCurve)
	{
		return 1.0f; // Default to always forcing when no curve is provided.
	}

	const float Range = FMath::Max(KINDA_SMALL_NUMBER, NeededNeed.Definition.MaxValue - NeededNeed.Definition.MinValue); // Avoid division by zero.
	const float Normalized = FMath::Clamp((NeededNeed.CurrentValue - NeededNeed.Definition.MinValue) / Range, 0.0f, 1.0f); // Normalize to 0-1.
	const float RawProbability = NeededNeed.Definition.ForceActivityProbabilityCurve->GetFloatValue(Normalized); // Sample the curve.
	return FMath::Clamp(RawProbability, 0.0f, 1.0f); // Clamp to valid probability range.
}

// Determines whether the current check should force the need activity.
bool UVillagerActivityComponent::ShouldForceNeedActivity(const FNeedRuntimeState& NeededNeed) const
{
	const float Probability = GetNeedForceProbability(NeededNeed);
	if (Probability <= 0.0f)
	{
		return false;
	}

	if (Probability >= 1.0f)
	{
		return true;
	}

	return FMath::FRand() <= Probability;
}

// Completes the current activity and transitions accordingly.
void UVillagerActivityComponent::CompleteCurrentActivity()
{
	ClearActivityTimers(); // Stop timers.

	bHasActiveActivity = false; // Mark activity inactive.
	ResetProviderContext(); // Clear provider cache to avoid stale references. 

	if (LogComponent) // Log completion.
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Completed activity: %s"), *UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.ActivityTag))); // Emit log with actor context.
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

	if (!ShouldForceNeedActivity(NeededNeed)) // Apply probabilistic forcing.
	{
		return false; // Skip forcing based on the probability curve.
	}

	return TryStartNeedSatisfyingActivity(NeededNeed); // Attempt to start the satisfying activity.
}

// Attempts to start the activity that satisfies the specified need.
bool UVillagerActivityComponent::TryStartNeedSatisfyingActivity(const FNeedRuntimeState& NeededNeed)
{
	if (!Archetype) // Validate data dependency.
	{
		return false; // Cannot proceed.
	}

	for (const FActivityDefinition& Definition : Archetype->ActivityDefinitions) // Search activities.
	{
		if (Definition.ActivityTag == NeededNeed.Definition.SatisfyingActivityTag) // Match satisfier.
		{
			if (IsActivityInProviderCooldown(Definition.ActivityTag)) // Respect provider cooldowns to avoid rapid retries. 
			{
				if (LogComponent) // Log skip for visibility. 
				{
					LogComponent->LogMessage(FString::Printf(TEXT("Skipping activity %s due to provider cooldown."), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag))); // Emit cooldown skip log. 
				}
				continue; // Skip this activity while cooldown is active. 
			}

			if (Definition.bRequiresSpecificLocation)
			{
				FTransform Dummy;
				if (!ResolveActivityTransform(Definition, Dummy))
				{
					continue; // Skip invalid locations.
				}
			}

			BeginActivity(Definition); // Start satisfying activity.
			if (LogComponent) // Log decision.
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Switching to satisfy need: %s"), *UVillagerLogComponent::GetShortTagString(NeededNeed.NeedTag))); // Emit log with actor context.
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
		if (IsActivityInProviderCooldown(Definition.ActivityTag)) // Skip activities blocked by provider cooldown. 
		{
			continue; // Continue when cooldown is active. 
		}

		const bool bWithinWindow = CurrentHour >= Definition.PartOfDayWindow.AllowedStartHour && CurrentHour < Definition.PartOfDayWindow.AllowedEndHour; // Check time window.

		if (Definition.bRequiresSpecificLocation)
		{
			FTransform Dummy;
			if (!ResolveActivityTransform(Definition, Dummy))
			{
				continue; // Skip invalid locations.
			}
		}

		if (bWithinWindow) // Start the first suitable activity.
		{
			BeginActivity(Definition); // Begin activity.
			return true; // Indicate success.
		}
	}

	for (const FActivityDefinition& Definition : DailyDefinitions) // Fallback to first valid daily even if off-window.
	{
		if (IsActivityInProviderCooldown(Definition.ActivityTag)) // Skip blocked activities. 
		{
			continue; // Continue to next candidate. 
		}

		if (Definition.bRequiresSpecificLocation)
		{
			FTransform Dummy;
			if (!ResolveActivityTransform(Definition, Dummy))
			{
				continue; // Skip invalid locations.
			}
		}

		BeginActivity(Definition);
		return true;
	}

	return false; // No activities available.
}

// Clears timers associated with the current activity.
void UVillagerActivityComponent::ClearActivityTimers()
{
	if (UWorld* World = GetWorld()) // Validate world.
	{
		World->GetTimerManager().ClearTimer(MovementFailureRetryHandle); // Clear movement failure retry timer.
		World->GetTimerManager().ClearTimer(ResourceCooldownHandle); // Clear resource cooldown timer.
	}
}

// Resolves a provider location offering the requested resource.
bool UVillagerActivityComponent::FindResourceProviderLocation(const FGameplayTag& ResourceTag, FResourceProviderContext& OutProviderContext) const
{
	OutProviderContext = FResourceProviderContext(); // Reset output context. 

	if (!GetWorld()) // Validate world.
	{
		return false; // Abort when world is missing.
	}

	UVillageLocationRegistry* Registry = GetWorld()->GetSubsystem<UVillageLocationRegistry>(); // Resolve location registry once.
	if (!Registry) // Validate registry.
	{
		return false; // Cannot resolve tagged locations without registry.
	}

	TArray<FResourceProviderContext> PresentCandidates; // Collect providers already at a trade spot. 
	TArray<FResourceProviderContext> AbsentCandidates; // Collect providers away from their trade spot. 

	for (TActorIterator<AActor> It(GetWorld()); It; ++It) // Iterate actors in the world.
	{
		if (AActor* Actor = *It) // Validate actor pointer.
		{
			if (Actor == GetOwner()) // Skip self to avoid self-provision loops. 
			{
				continue; // Continue searching other providers. 
			}

			if (UVillagerSocialComponent* Social = Actor->FindComponentByClass<UVillagerSocialComponent>()) // Resolve social component.
			{
				const FGameplayTag Provided = Social->GetProvidedResourceTag(); // Read provided resource.
				if (Provided != ResourceTag) // Skip non-matching providers.
				{
					continue; // Continue searching.
				}

				const TArray<FGameplayTag> TradeTags = Social->GetTradeLocationTags(); // Fetch trade locations.
				for (const FGameplayTag& TradeTag : TradeTags) // Iterate trade tags.
				{
					FTransform TradeTransform;
					if (!Registry->TryGetLocation(TradeTag, TradeTransform)) // Resolve transform.
					{
						continue; // Skip unresolved trade tag entries.
					}

					FResourceProviderContext CandidateContext; // Allocate candidate context holder. 
					CandidateContext.ProviderIdTag = Social->GetVillagerIdTag(); // Cache provider id. 
					CandidateContext.TradeLocationTag = TradeTag; // Cache trade tag. 
					CandidateContext.TradeLocationTransform = TradeTransform; // Cache trade transform. 
					CandidateContext.ProviderSocialComponent = Social; // Cache provider social component. 
					CandidateContext.ProviderActor = Actor; // Cache provider actor for presence checks. 
					CandidateContext.bWasPresentAtSelection = IsProviderAtTradeLocation(CandidateContext); // Determine presence at selection time. 

					if (CandidateContext.bWasPresentAtSelection) // Prefer providers already waiting at the trade spot. 
					{
						PresentCandidates.Add(CandidateContext); // Store present candidate. 
					}
					else // Track candidates that will require waiting. 
					{
						AbsentCandidates.Add(CandidateContext); // Store absent candidate. 
					}
				}
			}
		}
	}

	const TArray<FResourceProviderContext>& SelectionPool = PresentCandidates.Num() > 0 ? PresentCandidates : AbsentCandidates; // Prefer present providers, otherwise use absent list. 

	if (SelectionPool.Num() == 0) // No matching providers with resolvable trade tags.
	{
		return false; // Abort without fallback transform to enforce tag-only resolution.
	}

	const int32 Index = FMath::RandRange(0, SelectionPool.Num() - 1); // Choose random candidate.
	OutProviderContext = SelectionPool[Index]; // Assign resolved provider context. 
	return true; // Success.
}

// Resolves the target transform for an activity, preferring a tag lookup through the location registry.
bool UVillagerActivityComponent::ResolveActivityTransform(const FActivityDefinition& Definition, FTransform& OutTransform)
{
	if (!Definition.ActivityLocationTag.IsValid())
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (UVillageLocationRegistry* Registry = World->GetSubsystem<UVillageLocationRegistry>())
		{
			return Registry->TryGetLocation(Definition.ActivityLocationTag, OutTransform);
		}
	}

	return false;
}

// Starts movement toward the activity location, respecting throttled retries.
void UVillagerActivityComponent::StartMovementToActivityLocation(FActivityDefinition Definition)
{
	if (!MovementComponent) // Validate movement component.
	{
		CurrentRuntimeState.bWaitingForMovement = false; // No movement possible.
		return; // Abort.
	}

	FTransform TargetTransform;
	if (Definition.bRequiresSpecificLocation) // Resolve location when required.
	{
		if (bHasCachedActivityTransform) // Use cached transform if available.
		{
			TargetTransform = CachedActivityTransform; // Copy cached transform.
		}
		else if (!ResolveActivityTransform(Definition, TargetTransform)) // Resolve on demand.
		{
			if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Activity %s has no valid location; skipping."), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag)));
			}
			bHasActiveActivity = false;
			StartNextPlannedActivity();
			return;
		}
	}

	if (const double* LastFailure = LastMovementFailureTime.Find(Definition.ActivityTag)) // Avoid tight retry loops when nav fails.
	{
		const double Elapsed = FPlatformTime::Seconds() - *LastFailure;
		if (Elapsed < MovementFailureRetryDelaySeconds)
		{
			const float RetryDelay = FMath::Max(0.0f, MovementFailureRetryDelaySeconds - static_cast<float>(Elapsed));
			if (LogComponent)
			{
				LogComponent->LogMessage(FString::Printf(TEXT("Delaying activity %s retry for %.1f seconds after navigation failure."), *UVillagerLogComponent::GetShortTagString(Definition.ActivityTag), RetryDelay));
			}
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, RetryDelay, false);
			}
			bHasActiveActivity = false;
			return;
		}
	}

	CurrentRuntimeState.bWaitingForMovement = Definition.bRequiresSpecificLocation; // Flag waiting only when movement occurs.

	if (Definition.bRequiresSpecificLocation) // Issue movement when needed.
	{
		MovementComponent->RequestMoveToLocation(TargetTransform, MovementComponent->GetAcceptanceRadius(), FOnVillagerMovementFinished::CreateUObject(this, &UVillagerActivityComponent::HandleMovementFinished)); // Issue move request.
	}
}

// Region: Resource helpers. 
#pragma region ResourceHelpers
// Determines whether the provider is currently at the expected trade location.
bool UVillagerActivityComponent::IsProviderAtTradeLocation(const FResourceProviderContext& ProviderContext) const
{
	if (!ProviderContext.ProviderActor.IsValid()) // Validate provider actor.
	{
		return false; // Treat invalid actors as absent.
	}

	const FVector ProviderLocation = ProviderContext.ProviderActor->GetActorLocation(); // Read provider world position.
	const FVector TradeLocation = ProviderContext.TradeLocationTransform.GetLocation(); // Read target trade position.

	float PresenceRadius = TradePresenceTolerance; // Start with configured tolerance.

	if (const AActor* ProviderActor = ProviderContext.ProviderActor.Get()) // Resolve provider actor pointer.
	{
		if (const UVillagerMovementComponent* ProviderMovement = ProviderActor->FindComponentByClass<UVillagerMovementComponent>()) // Resolve provider movement component.
		{
			PresenceRadius = FMath::Max(PresenceRadius, ProviderMovement->GetAcceptanceRadius()); // Use the larger of default tolerance or provider acceptance radius.
		}
	}

	return FVector::DistSquared(ProviderLocation, TradeLocation) <= FMath::Square(PresenceRadius); // Determine presence within tolerance.
}

// Handles provider absence and applies affection penalties.
void UVillagerActivityComponent::HandleProviderUnavailable()
{
	ClearActivityTimers(); // Stop any timers to avoid overlapping retries. 
	bFetchingResource = false; // Clear resource acquisition flag.
	bHasActiveActivity = false; // Mark activity inactive.
	CurrentRuntimeState.bWaitingForMovement = false; // Clear waiting flag.

	if (SocialComponent && CachedProviderIdTag.IsValid()) // Apply affection loss toward the unavailable provider.
	{
		SocialComponent->RegisterMissedTrade(CachedProviderIdTag); // Register missed trade on buyer social map.
	}

	if (LogComponent) // Log provider absence for visibility.
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Provider %s unavailable at %s; retrying after %.1f seconds."),
			*UVillagerLogComponent::GetShortTagString(CachedProviderIdTag),
			*UVillagerLogComponent::GetShortTagString(CachedProviderContext.TradeLocationTag),
			ProviderFailureCooldownSeconds)); // Compose absence log entry.
	}

	LastProviderFailureTime.FindOrAdd(CurrentRuntimeState.Definition.ActivityTag) = FPlatformTime::Seconds(); // Track provider failure time to throttle retries.

	if (UWorld* World = GetWorld()) // Schedule a delayed retry.
	{
		World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, ProviderFailureCooldownSeconds, false); // Retry after provider failure cooldown.
	}

	ResetProviderContext(); // Clear cached provider references.
}

// Applies archetype-driven tuning values to runtime properties.
void UVillagerActivityComponent::ApplyArchetypeTuning()
{
	if (!Archetype) // Validate archetype.
	{
		return; // Abort when no data is available.
	}

	ResourceFetchCooldownSeconds = Archetype->SocialDefinition.PostTradeCooldownSeconds; // Pull trade cooldown from archetype data.
}

// Resolves the urgency of the need satisfied by the current activity.
EVillagerNeedUrgency UVillagerActivityComponent::ResolveNeedUrgencyForCurrentActivity() const
{
	if (!NeedsComponent) // Validate needs component.
	{
		return EVillagerNeedUrgency::Mild; // Default to mild urgency when unknown.
	}

	const TArray<FNeedRuntimeState>& RuntimeNeeds = NeedsComponent->GetRuntimeNeeds(); // Access runtime needs.
	for (const FNeedRuntimeState& Need : RuntimeNeeds) // Iterate needs.
	{
		if (Need.Definition.SatisfyingActivityTag == CurrentRuntimeState.Definition.ActivityTag) // Match by satisfying activity tag.
		{
			const float Range = FMath::Max(KINDA_SMALL_NUMBER, Need.Definition.MaxValue - Need.Definition.MinValue); // Compute safe range.
			const float Normalized = (Need.CurrentValue - Need.Definition.MinValue) / Range; // Normalize need value.

			if (Normalized <= Need.Definition.Thresholds.CriticalThreshold) // Check critical band.
			{
				return EVillagerNeedUrgency::Critical; // Report critical urgency.
			}

			if (Normalized <= Need.Definition.Thresholds.MildThreshold) // Check mild band.
			{
				return EVillagerNeedUrgency::Mild; // Report mild urgency.
			}

			return EVillagerNeedUrgency::Satisfied; // Default to satisfied when above thresholds.
		}
	}

	return EVillagerNeedUrgency::Mild; // Fall back to mild when no matching need is found.
}

// Clears cached provider context and identifiers.
void UVillagerActivityComponent::ResetProviderContext()
{
	CachedProviderContext = FResourceProviderContext(); // Reset provider context struct.
	CachedProviderIdTag = FGameplayTag(); // Clear provider identifier cache.
}

// Determines whether the activity is blocked by a provider failure cooldown window.
bool UVillagerActivityComponent::IsActivityInProviderCooldown(const FGameplayTag& ActivityTag) const
{
	if (!ActivityTag.IsValid()) // Validate activity tag input.
	{
		return false; // Invalid tags cannot be gated.
	}

	if (const double* LastFailure = LastProviderFailureTime.Find(ActivityTag)) // Check for recorded provider failure.
	{
		const double Elapsed = FPlatformTime::Seconds() - *LastFailure; // Compute elapsed seconds since failure.
		return Elapsed < ProviderFailureCooldownSeconds; // Report cooldown active when elapsed is below threshold.
	}

	return false; // Default to not in cooldown when no record exists.
}
#pragma endregion ResourceHelpers

// Handles completion of resource acquisition movement.
void UVillagerActivityComponent::HandleResourceMovementFinished(bool bSuccess)
{
	CurrentRuntimeState.bWaitingForMovement = false; // Clear waiting flag.

	if (!bSuccess) // Handle failure.
	{
		bFetchingResource = false; // Clear fetch state.
		bHasActiveActivity = false; // Reset activity state.
		ResetProviderContext(); // Clear cached provider references after failure. 

		if (LogComponent)
		{
			LogComponent->LogMessage(FString::Printf(TEXT("Failed to reach provider for %s; reselecting after %.1f seconds."),
				*UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.ActivityTag),
				ProviderFailureCooldownSeconds));
		}

		LastProviderFailureTime.FindOrAdd(CurrentRuntimeState.Definition.ActivityTag) = FPlatformTime::Seconds(); // Record provider failure time.

		if (UWorld* World = GetWorld()) // Retry later.
		{
			World->GetTimerManager().SetTimer(MovementFailureRetryHandle, this, &UVillagerActivityComponent::StartNextPlannedActivity, ProviderFailureCooldownSeconds, false);
		}
		return;
	}

	if (!CachedProviderContext.ProviderSocialComponent.IsValid() || !CachedProviderContext.ProviderActor.IsValid()) // Validate provider context before trading. 
	{
		HandleProviderUnavailable(); // Treat missing context as an unavailable provider. 
		return; // Exit after handling failure. 
	}

	if (!IsProviderAtTradeLocation(CachedProviderContext)) // Ensure provider presence at the trade spot. 
	{
		HandleProviderUnavailable(); // Apply affection loss and reschedule. 
		return; // Exit to avoid continuing without the resource. 
	}

	const EVillagerNeedUrgency TradeUrgency = ResolveNeedUrgencyForCurrentActivity(); // Resolve urgency to drive affection gains. 
	float GrantedQuantity = 0.0f; // Track granted resource quantity. 

	if (SocialComponent) // Validate buyer social component before requesting. 
	{
		const FGameplayTag RequesterId = SocialComponent->GetVillagerIdTag(); // Resolve requester id for provider lookup. 
		GrantedQuantity = CachedProviderContext.ProviderSocialComponent.Get()->RequestResource(RequesterId, CurrentRuntimeState.Definition.RequiredResourceTag, TradeUrgency); // Request resources from provider. 
	}

	if (LogComponent) // Log resource acquisition details. 
	{
		LogComponent->LogMessage(FString::Printf(TEXT("Acquired %.2f of %s from %s at %s; proceeding to %s."),
			GrantedQuantity,
			*UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.RequiredResourceTag),
			*UVillagerLogComponent::GetShortTagString(CachedProviderIdTag),
			*UVillagerLogComponent::GetShortTagString(CachedProviderContext.TradeLocationTag),
			*UVillagerLogComponent::GetShortTagString(CurrentRuntimeState.Definition.ActivityTag))); // Compose acquisition summary. 
	}

	bFetchingResource = false; // Resource acquired.
	CurrentRuntimeState.bWaitingForMovement = true; // Remain in waiting state during cooldown.

	ResetProviderContext(); // Clear provider cache after successful trade. 

	if (UWorld* World = GetWorld()) // Schedule delayed movement to activity.
	{
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(this, &UVillagerActivityComponent::StartMovementToActivityLocation, CurrentRuntimeState.Definition); // Bind movement start.
		World->GetTimerManager().SetTimer(ResourceCooldownHandle, Delegate, ResourceFetchCooldownSeconds, false); // Start cooldown timer.
	}
}
