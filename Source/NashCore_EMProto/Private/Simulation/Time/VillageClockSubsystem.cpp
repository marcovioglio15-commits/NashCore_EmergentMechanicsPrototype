// Includes the class declaration for the village clock subsystem.
#include "Simulation/Time/VillageClockSubsystem.h"

// Region: Engine headers.
#pragma region EngineIncludes
// Provides access to the world context for timer management.
#include "Engine/World.h"
// Grants access to the timer manager for scheduling recurring tasks.
#include "TimerManager.h"
#pragma endregion EngineIncludes

// Default constructor setting initial time values.
UVillageClockSubsystem::UVillageClockSubsystem()
	: CurrentHour(6) // Start the day at 6 AM.
	, CurrentMinute(0) // Start at minute zero.
	, CurrentPhase(EVillageDayPhase::Day) // Default to day phase.
	, SecondsPerGameMinute(1.0f) // One real second equals one in-game minute.
{
}

// Allows subsystem creation for any world instance.
bool UVillageClockSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true; // Always create to keep time available globally.
}

// Initializes the subsystem and starts ticking.
void UVillageClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection); // Call parent to respect lifecycle.

	UpdatePhaseFromHour(); // Ensure phase matches the starting hour.

	StartClock(); // Begin ticking minutes.
}

// Tears down timers on subsystem shutdown.
void UVillageClockSubsystem::Deinitialize()
{
	StopClock(); // Stop timers to avoid dangling callbacks.

	Super::Deinitialize(); // Run parent cleanup.
}

// Begins the recurring timer that advances minutes.
void UVillageClockSubsystem::StartClock()
{
	if (UWorld* World = GetWorld()) // Acquire world safely.
	{
		World->GetTimerManager().SetTimer(ClockTimerHandle, this, &UVillageClockSubsystem::AdvanceOneMinute, SecondsPerGameMinute, true); // Start looping timer.
	}
}

// Stops the clock timer if it is active.
void UVillageClockSubsystem::StopClock()
{
	if (UWorld* World = GetWorld()) // Ensure world exists.
	{
		World->GetTimerManager().ClearTimer(ClockTimerHandle); // Clear the timer to halt updates.
	}
}

// Adjusts the real seconds per in-game minute and restarts the timer.
void UVillageClockSubsystem::SetSecondsPerGameMinute(float InSecondsPerGameMinute)
{
	SecondsPerGameMinute = FMath::Max(0.1f, InSecondsPerGameMinute); // Clamp to avoid zero or negative values.

	StopClock(); // Stop existing timer to reschedule.

	StartClock(); // Restart with the new cadence.
}

// Returns the current hour in 24-hour format.
int32 UVillageClockSubsystem::GetCurrentHour() const
{
	return CurrentHour; // Provide cached hour.
}

// Returns the current minute.
int32 UVillageClockSubsystem::GetCurrentMinute() const
{
	return CurrentMinute; // Provide cached minute.
}

// Returns the current day phase.
EVillageDayPhase UVillageClockSubsystem::GetCurrentPhase() const
{
	return CurrentPhase; // Provide cached phase.
}

// Internal tick that advances one minute and fires events.
void UVillageClockSubsystem::AdvanceOneMinute()
{
	++CurrentMinute; // Increment minute counter.

	if (CurrentMinute >= 60) // Check for hour overflow.
	{
		CurrentMinute = 0; // Reset minutes.
		++CurrentHour; // Increment hour.

		if (CurrentHour >= 24) // Check for day rollover.
		{
			CurrentHour = 0; // Wrap to midnight.
		}

		OnHourChanged.Broadcast(CurrentHour); // Notify listeners of hour change.

		UpdatePhaseFromHour(); // Update phase based on new hour.
	}

	OnMinuteChanged.Broadcast(CurrentHour, CurrentMinute); // Notify listeners every minute.
}

// Updates the day phase and broadcasts change if needed.
void UVillageClockSubsystem::UpdatePhaseFromHour()
{
	const bool bShouldBeDay = CurrentHour >= 6 && CurrentHour < 18; // Define day window.

	const EVillageDayPhase NewPhase = bShouldBeDay ? EVillageDayPhase::Day : EVillageDayPhase::Night; // Determine phase.

	if (NewPhase != CurrentPhase) // Only broadcast when phase changes.
	{
		CurrentPhase = NewPhase; // Cache new phase.
		OnPhaseChanged.Broadcast(CurrentPhase); // Notify listeners.
	}
}
