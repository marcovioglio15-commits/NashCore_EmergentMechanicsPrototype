// Includes the example villager character declaration.
#include "Simulation/Examples/ExampleVillagerCharacter.h"

// Region: Engine includes.
#pragma region EngineIncludes
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#pragma endregion EngineIncludes

// Constructor creating simulation components.
AExampleVillagerCharacter::AExampleVillagerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) // Call parent constructor.
{
	NeedsComponent = CreateDefaultSubobject<UVillagerNeedsComponent>(TEXT("NeedsComponent")); // Instantiate needs component.
	ActivityComponent = CreateDefaultSubobject<UVillagerActivityComponent>(TEXT("ActivityComponent")); // Instantiate activity component.
	SocialComponent = CreateDefaultSubobject<UVillagerSocialComponent>(TEXT("SocialComponent")); // Instantiate social component.
	MovementComponent = CreateDefaultSubobject<UVillagerMovementComponent>(TEXT("MovementComponent")); // Instantiate movement component.
	LogComponent = CreateDefaultSubobject<UVillagerLogComponent>(TEXT("LogComponent")); // Instantiate log component.
	NeedsDisplayComponent = CreateDefaultSubobject<UVillagerNeedsDisplayComponent>(TEXT("NeedsDisplayComponent")); // Instantiate needs display component.

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned; // Ensure AI controller possession in PIE/world.
	AIControllerClass = AAIController::StaticClass(); // Default AI controller if none specified.
	bUseControllerRotationYaw = false; // Let movement control rotation.
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
	}
}

// Applies archetype data to all simulation components.
void AExampleVillagerCharacter::BeginPlay()
{
	Super::BeginPlay(); // Preserve parent initialization.

	if (ArchetypeData && NeedsComponent) // Configure needs component.
	{
		NeedsComponent->SetArchetype(ArchetypeData); // Apply archetype.
	}

	if (ArchetypeData && ActivityComponent) // Configure activity component.
	{
		ActivityComponent->SetArchetype(ArchetypeData); // Apply archetype.
	}

	if (ArchetypeData && SocialComponent) // Configure social component.
	{
		SocialComponent->SetArchetype(ArchetypeData); // Apply archetype.
	}

	if (ArchetypeData && MovementComponent) // Configure movement component.
	{
		MovementComponent->ApplyMovementDefinition(ArchetypeData->MovementDefinition); // Apply movement tuning.
	}

	if (ArchetypeData && LogComponent) // Configure log identity.
	{
		LogComponent->SetVillagerIdTag(ArchetypeData->VillagerIdTag);
	}

	if (NeedsDisplayComponent) // Force widget component creation on begin play.
	{
		NeedsDisplayComponent->InitializeWidgetComponent(); // Ensure widget component exists.
		NeedsDisplayComponent->SetWidgetVisible(true); // Show widget by default to avoid hidden state.
	}
}
