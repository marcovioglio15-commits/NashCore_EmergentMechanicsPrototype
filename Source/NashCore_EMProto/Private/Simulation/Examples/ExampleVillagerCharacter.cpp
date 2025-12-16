// Includes the example villager character declaration.
#include "Simulation/Examples/ExampleVillagerCharacter.h"

// Constructor creating simulation components.
AExampleVillagerCharacter::AExampleVillagerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer) // Call parent constructor.
{
	NeedsComponent = CreateDefaultSubobject<UVillagerNeedsComponent>(TEXT("NeedsComponent")); // Instantiate needs component.
	ActivityComponent = CreateDefaultSubobject<UVillagerActivityComponent>(TEXT("ActivityComponent")); // Instantiate activity component.
	SocialComponent = CreateDefaultSubobject<UVillagerSocialComponent>(TEXT("SocialComponent")); // Instantiate social component.
	MovementComponent = CreateDefaultSubobject<UVillagerMovementComponent>(TEXT("MovementComponent")); // Instantiate movement component.
	LogComponent = CreateDefaultSubobject<UVillagerLogComponent>(TEXT("LogComponent")); // Instantiate log component.
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
}
