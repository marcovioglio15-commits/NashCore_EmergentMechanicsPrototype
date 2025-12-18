// Prevents multiple inclusion of the game mode header.
#pragma once

// Region: Includes.
#pragma region Includes
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#pragma endregion Includes

// Generated header for reflection support.
#include "CoreGameMode.generated.h"

class AVillageHUDActor;
class APlayerController; // Forward-declared player controller class.

// Simple game mode that can spawn the Village HUD actor automatically.
UCLASS()
class ACoreGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Sets default classes for HUD.
	ACoreGameMode();

	// Spawns HUD actor on begin play if not already present.
	virtual void BeginPlay() override;

	// Ensures late-joining players receive the correct HUD class.
	virtual void PostLogin(APlayerController* NewPlayer) override; // Handles post-login HUD setup.

	// Class used to spawn a HUD actor at begin play.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<AVillageHUDActor> HUDActorClass;
};
