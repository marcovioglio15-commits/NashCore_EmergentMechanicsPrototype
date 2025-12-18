// Includes the HUD actor declaration.
#include "Simulation/UI/VillageHUDActor.h"

// Region: Engine includes.
#pragma region EngineIncludes
// Provides actor iteration helpers.
#include "EngineUtils.h"
// Access to world context helpers.
#include "Engine/World.h"
// Supplies player controller access for widget creation.
#include "GameFramework/PlayerController.h"
#include "Logging/LogMacros.h"
#pragma endregion EngineIncludes

// Region: UMG includes.
#pragma region UMGIncludes
// Enables widget creation from C++.
#include "Blueprint/UserWidget.h" // Imports UUserWidget for widget creation.
#pragma endregion UMGIncludes

// Region: Simulation includes.
#pragma region SimulationIncludes
#include "Simulation/UI/VillageStatusWidget.h"
#include "Simulation/Logging/VillagerLogComponent.h"
#include "Simulation/Time/VillageClockSubsystem.h"
#pragma endregion SimulationIncludes

// Region: Lifecycle. 
#pragma region Lifecycle // Groups lifecycle-related methods.
// Default constructor disabling ticking.
AVillageHUDActor::AVillageHUDActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bShowHUD = true;
}

// Spawns the status widget and wires it up to the simulation data.
void AVillageHUDActor::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PlayerController = PlayerOwner.Get();
	if (!PlayerController)
	{
		PlayerController = World->GetFirstPlayerController();
	}
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("VillageHUDActor: No player controller found; widget will not be created."));
		return;
	}

	TSubclassOf<UUserWidget> ClassToUse = WidgetClass; // Select the configured widget class.
	if (!ClassToUse) // Check whether a widget class was configured.
	{
		UE_LOG(LogTemp, Warning, TEXT("VillageHUDActor: WidgetClass is not set; falling back to UVillageStatusWidget.")); // Emit a one-shot warning for missing widget configuration.
		ClassToUse = UVillageStatusWidget::StaticClass(); // Fall back to the built-in status widget class.
	}

	ActiveWidget = CreateWidget<UUserWidget>(PlayerController, ClassToUse); // Create the widget instance.

	if (ActiveWidget)
	{
		UVillagerLogComponent* LogComponent = ResolveLogComponent();
		ActiveWidget->AddToViewport(); // Attach the widget to the viewport.
		if (ActiveWidget->GetVisibility() == ESlateVisibility::Visible) // Avoid overriding intentional visibility states.
		{
			ActiveWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // Prevent full-screen panels from blocking mouse input.
		}
		if (UVillageStatusWidget* StatusWidget = Cast<UVillageStatusWidget>(ActiveWidget)) // Confirm the widget supports status bindings.
		{
			StatusWidget->InitializeFromSources(World->GetSubsystem<UVillageClockSubsystem>(), LogComponent); // Bind data sources when supported.
		}
		else // Handle non-status widget classes without data binding.
		{
			UE_LOG(LogTemp, Warning, TEXT("VillageHUDActor: Widget class %s does not derive from UVillageStatusWidget; skipping data binding."), *ClassToUse->GetName()); // Emit a one-shot warning when the widget cannot bind to data sources.
		}
		UVillagerLogComponent::SetOnScreenDebugEnabled(false); // Suppress on-screen debug while the UI log is visible.
		UE_LOG(LogTemp, Log, TEXT("VillageHUDActor: Widget created (%s) and added to viewport."), *ClassToUse->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("VillageHUDActor: Failed to create widget of class %s."), *GetNameSafe(*ClassToUse));
	}
}

// Removes the widget from the viewport when the actor is destroyed.
void AVillageHUDActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		UVillagerLogComponent::SetOnScreenDebugEnabled(true); // Re-enable on-screen debug when the UI is removed.
		ActiveWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}
#pragma endregion Lifecycle // Ends the lifecycle method group.

// Region: Helpers. 
#pragma region Helpers // Groups helper functions.
// Attempts to locate a villager log component from the preferred actor or any actor in the world.
UVillagerLogComponent* AVillageHUDActor::ResolveLogComponent() const
{
	if (PreferredVillager)
	{
		if (UVillagerLogComponent* Comp = PreferredVillager->FindComponentByClass<UVillagerLogComponent>())
		{
			return Comp;
		}
	}

	if (!bAutoFindVillager || !GetWorld())
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (UVillagerLogComponent* Comp = It->FindComponentByClass<UVillagerLogComponent>())
		{
			return Comp;
		}
	}

	return nullptr;
}
#pragma endregion Helpers // Ends the helper function group.
