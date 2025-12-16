// Prevents multiple inclusion of the example villager header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides character base functionality.
#include "GameFramework/Character.h"
// Imports villager needs component.
#include "Simulation/Needs/VillagerNeedsComponent.h"
// Imports activity component.
#include "Simulation/Activities/VillagerActivityComponent.h"
// Imports social component.
#include "Simulation/Social/VillagerSocialComponent.h"
// Imports movement component.
#include "Simulation/Movement/VillagerMovementComponent.h"
// Imports logging component.
#include "Simulation/Logging/VillagerLogComponent.h"
#pragma endregion Includes

// Generated header required for reflection.
#include "ExampleVillagerCharacter.generated.h"

// Simple example character wiring the villager simulation components together.
UCLASS()
class AExampleVillagerCharacter : public ACharacter
{
	GENERATED_BODY() // Enables reflection and constructors.

public:
	// Constructor creating default subobjects.
	AExampleVillagerCharacter(const FObjectInitializer& ObjectInitializer);

	// Applies archetype data to components once the game begins.
	virtual void BeginPlay() override;

private:
	// Archetype asset defining this villager's data.
	UPROPERTY(EditAnywhere, Category = "Villager")
	TObjectPtr<UVillagerArchetypeDataAsset> ArchetypeData;

	// Needs component instance.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TObjectPtr<UVillagerNeedsComponent> NeedsComponent;

	// Activity component instance.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TObjectPtr<UVillagerActivityComponent> ActivityComponent;

	// Social component instance.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TObjectPtr<UVillagerSocialComponent> SocialComponent;

	// Movement component instance.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TObjectPtr<UVillagerMovementComponent> MovementComponent;

	// Log component instance.
	UPROPERTY(VisibleAnywhere, Category = "Villager")
	TObjectPtr<UVillagerLogComponent> LogComponent;
};
