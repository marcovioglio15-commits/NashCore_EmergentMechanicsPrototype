// Includes the tagged location actor declaration.
#include "Simulation/Locations/TaggedLocationActor.h"

// Region: Engine includes.
#pragma region EngineIncludes
#include "Components/BillboardComponent.h"
#include "Engine/CollisionProfile.h"
#include "UObject/ConstructorHelpers.h"
#pragma endregion EngineIncludes

// Default constructor disables ticking and adds a simple sprite for editor visibility.
ATaggedLocationActor::ATaggedLocationActor()
{
	PrimaryActorTick.bCanEverTick = false;

#if WITH_EDITORONLY_DATA
	// Add a billboard so designers can see the marker in the viewport.
	UBillboardComponent* Sprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	Sprite->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	Sprite->Mobility = EComponentMobility::Movable;
	RootComponent = Sprite;
#endif
}
