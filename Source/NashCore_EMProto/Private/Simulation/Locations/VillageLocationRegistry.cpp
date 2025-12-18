// Includes the registry declaration.
#include "Simulation/Locations/VillageLocationRegistry.h"

// Region: Engine includes.
#pragma region EngineIncludes
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationSystem.h"
#pragma endregion EngineIncludes

// Region: Simulation includes.
#pragma region SimulationIncludes
#include "Simulation/Locations/TaggedLocationActor.h"
#pragma endregion SimulationIncludes

// Local log category for registry diagnostics.
DEFINE_LOG_CATEGORY_STATIC(LogVillageLocationRegistry, Log, All);

// Initialize and populate the registry at startup.
void UVillageLocationRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RefreshRegistry();
}

// Scans the world for tagged location actors and caches their transforms.
void UVillageLocationRegistry::RefreshRegistry()
{
	RegisteredLocations.Reset();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ATaggedLocationActor> It(World); It; ++It)
		{
			AddFromActor(*It);
		}
	}
}

// Attempts to fetch a transform for a given tag. Refreshes and projects to nav if needed.
bool UVillageLocationRegistry::TryGetLocation(const FGameplayTag& LocationTag, FTransform& OutTransform)
{
	if (!LocationTag.IsValid())
	{
		return false;
	}

	bool bFoundLocation = false;

	if (const FTransform* Found = RegisteredLocations.Find(LocationTag))
	{
		OutTransform = *Found;
		bFoundLocation = true;
	}
	else
	{
		// Registry may not have seen newly placed actors yet; refresh on demand.
		RefreshRegistry();

		if (const FTransform* Retry = RegisteredLocations.Find(LocationTag))
		{
			OutTransform = *Retry;
			bFoundLocation = true;
		}
	}

	if (!bFoundLocation)
	{
		return false;
	}

	// Always project to NavMesh to correct vertical offsets from placed actors.
	if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Projected;
		if (NavSystem->ProjectPointToNavigation(OutTransform.GetLocation(), Projected, FVector(300.0f)))
		{
			OutTransform.SetLocation(Projected.Location);
		}
	}

	return true;
}

// Adds a tagged actor to the registry, projecting to navmesh when available.
void UVillageLocationRegistry::AddFromActor(const ATaggedLocationActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	if (!Actor->LocationTag.IsValid())
	{
		UE_LOG(LogVillageLocationRegistry, Warning, TEXT("TaggedLocationActor %s has no LocationTag set; skipping."), *GetNameSafe(Actor));
		return;
	}

	FTransform UseTransform = Actor->GetTaggedTransform();

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSystem)
	{
		FNavLocation Projected;
		// Project within a generous extent to tolerate slight offsets.
		if (NavSystem->ProjectPointToNavigation(UseTransform.GetLocation(), Projected, FVector(300.0f)))
		{
			UseTransform.SetLocation(Projected.Location);
		}
		else
		{
			UE_LOG(LogVillageLocationRegistry, Warning, TEXT("TaggedLocationActor %s is not on NavMesh; using raw transform."), *GetNameSafe(Actor));
		}
	}

	if (RegisteredLocations.Contains(Actor->LocationTag))
	{
		UE_LOG(LogVillageLocationRegistry, Warning, TEXT("Duplicate LocationTag %s found; overriding previous entry."), *Actor->LocationTag.ToString());
	}

	RegisteredLocations.Add(Actor->LocationTag, UseTransform);
}
