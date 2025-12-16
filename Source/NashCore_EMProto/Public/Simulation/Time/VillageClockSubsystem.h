// Prevents multiple inclusion of this header during compilation.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides core types and helper macros.
#include "CoreMinimal.h"
// Supplies the base class for world subsystems that live with the world.
#include "Subsystems/WorldSubsystem.h"
// Brings in shared villager data types including the day phase enumeration.
#include "Simulation/Data/VillagerDataAssets.h"
// Grants access to delegates for broadcasting clock events.
#include "Delegates/DelegateCombinations.h"
#pragma endregion Includes

// Generated header include required for Unreal reflection.
#include "VillageClockSubsystem.generated.h"

// Declares a delegate fired every in-game minute with hour and minute payloads.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVillageMinuteChanged, int32, Hour, int32, Minute);

// Declares a delegate fired when the hour value changes.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVillageHourChanged, int32, Hour);

// Declares a delegate fired when the day phase toggles.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVillagePhaseChanged, EVillageDayPhase, NewPhase);

// World subsystem that owns the authoritative simulation clock.
UCLASS()
class UVillageClockSubsystem : public UWorldSubsystem
{
	GENERATED_BODY() // Generates constructors and reflection metadata.

public:
	// Constructor that sets safe defaults for time progression.
	UVillageClockSubsystem();

	// Returns true to create the subsystem for any world.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Initializes the subsystem and starts the clock timer.
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Cleans up timers on shutdown.
	virtual void Deinitialize() override;

	// Starts the in-game clock if it is not already running.
	void StartClock();

	// Stops the in-game clock and clears timers.
	void StopClock();

	// Sets the number of real seconds that equal one in-game minute.
	void SetSecondsPerGameMinute(float InSecondsPerGameMinute);

	// Retrieves the current hour.
	int32 GetCurrentHour() const;

	// Retrieves the current minute.
	int32 GetCurrentMinute() const;

	// Retrieves the current day phase.
	EVillageDayPhase GetCurrentPhase() const;

	// Multicast delegate raised each minute.
	UPROPERTY(BlueprintAssignable, Category = "Village Clock")
	FOnVillageMinuteChanged OnMinuteChanged;

	// Multicast delegate raised each hour.
	UPROPERTY(BlueprintAssignable, Category = "Village Clock")
	FOnVillageHourChanged OnHourChanged;

	// Multicast delegate raised when the day-night phase toggles.
	UPROPERTY(BlueprintAssignable, Category = "Village Clock")
	FOnVillagePhaseChanged OnPhaseChanged;

private:
	// Internal tick called by the timer to advance one in-game minute.
	void AdvanceOneMinute();

	// Evaluates whether the day phase should switch based on the hour.
	void UpdatePhaseFromHour();

	// Cached current hour (0-23).
	int32 CurrentHour;

	// Cached current minute (0-59).
	int32 CurrentMinute;

	// Current day phase derived from the hour.
	EVillageDayPhase CurrentPhase;

	// Real seconds corresponding to one in-game minute.
	float SecondsPerGameMinute;

	// Timer handle managing the recurring clock advancement.
	FTimerHandle ClockTimerHandle;
};
