// Includes the game mode declaration.
#include "Simulation/Core/CoreGameMode.h"

// Region: Engine includes.
#pragma region EngineIncludes
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#pragma endregion EngineIncludes

// Region: Simulation includes.
#pragma region SimulationIncludes
#include "Simulation/UI/VillageHUDActor.h"
#include "Simulation/Player/FixedCameraPawn.h"
#pragma endregion SimulationIncludes

// Region: Game mode lifecycle. 
#pragma region GameModeLifecycle // Groups game mode lifecycle methods.
ACoreGameMode::ACoreGameMode()
{
	// Ensure the default HUD can be selected from the standard HUD drop-down.
	HUDClass = AVillageHUDActor::StaticClass();
	HUDActorClass = nullptr; // Prefer HUDClass unless an explicit override is provided.
	DefaultPawnClass = AFixedCameraPawn::StaticClass(); // Use the fixed-camera pawn by default.
}

void ACoreGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TSubclassOf<AHUD> HudClassToUse = HUDClass ? HUDClass : HUDActorClass; // Prefer the standard HUDClass when set.
	if (!HudClassToUse) // Validate the resolved HUD class before applying it.
	{
		UE_LOG(LogTemp, Warning, TEXT("CoreGameMode: HUDClass is not set; no HUD will be spawned.")); // Emit a one-shot warning when the HUD class is missing.
		return; // Exit early when no HUD class is configured.
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			if (!PC->GetHUD() || !PC->GetHUD()->IsA(AVillageHUDActor::StaticClass())) // Ensure the HUD is the expected class.
			{
				// Spawn HUD on the owning client; avoids server-only actors that never create widgets.
				PC->ClientSetHUD(HudClassToUse);
				UE_LOG(LogTemp, Log, TEXT("CoreGameMode: Requested HUD %s for player %s."), *GetNameSafe(HudClassToUse), *GetNameSafe(PC)); // Emit a one-shot log for HUD assignment.
			}
		}
	}
}

void ACoreGameMode::PostLogin(APlayerController* NewPlayer) // Handles HUD setup for late-joining players.
{
	Super::PostLogin(NewPlayer); // Invoke base post-login handling.

	if (!NewPlayer) // Validate the incoming player controller.
	{
		return; // Exit when no player controller is available.
	}

	const TSubclassOf<AHUD> HudClassToUse = HUDClass ? HUDClass : HUDActorClass; // Prefer the standard HUDClass when set.
	if (!HudClassToUse) // Validate the resolved HUD class.
	{
		UE_LOG(LogTemp, Warning, TEXT("CoreGameMode: HUDClass is not set during PostLogin; no HUD will be spawned.")); // Emit a one-shot warning when the HUD class is missing.
		return; // Exit when no HUD class can be resolved.
	}

	if (!NewPlayer->GetHUD() || !NewPlayer->GetHUD()->IsA(HudClassToUse)) // Ensure the correct HUD class is active.
	{
		NewPlayer->ClientSetHUD(HudClassToUse); // Request the client to spawn the HUD.
		UE_LOG(LogTemp, Log, TEXT("CoreGameMode: Requested HUD %s for player %s (PostLogin)."), *GetNameSafe(HudClassToUse), *GetNameSafe(NewPlayer)); // Emit a one-shot log for HUD assignment.
	}
}
#pragma endregion GameModeLifecycle // Ends the game mode lifecycle group.
