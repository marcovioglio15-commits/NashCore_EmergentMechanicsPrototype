// Includes the villager needs display component declaration. 
#include "Simulation/UI/VillagerNeedsDisplayComponent.h" // Imports UVillagerNeedsDisplayComponent. 

// Region: Engine includes. 
#pragma region EngineIncludes
// Provides scene component attachment utilities. 
#include "Components/SceneComponent.h" // Imports USceneComponent. 
// Provides widget component implementation. 
#include "Components/WidgetComponent.h" // Imports UWidgetComponent. 
// Provides access to user widget base class. 
#include "Blueprint/UserWidget.h" // Imports UUserWidget. 
// Provides access to engine debug utilities.
#include "Engine/Engine.h" // Imports GEngine.
#pragma endregion EngineIncludes

// Region: Simulation includes. 
#pragma region SimulationIncludes
// Provides access to needs data for binding. 
#include "Simulation/Needs/VillagerNeedsComponent.h" // Imports UVillagerNeedsComponent. 
// Provides access to the needs widget type for initialization. 
#include "Simulation/UI/VillagerNeedsWidget.h" // Imports UVillagerNeedsWidget. 
// Provides access to social data for binding.
#include "Simulation/Social/VillagerSocialComponent.h" // Imports UVillagerSocialComponent.
#pragma endregion SimulationIncludes

// Region: Lifecycle. 
#pragma region Lifecycle
// Constructs the component with default settings. 
UVillagerNeedsDisplayComponent::UVillagerNeedsDisplayComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // Disable ticking for performance. 
}

// Builds the widget component at begin play. 
void UVillagerNeedsDisplayComponent::BeginPlay()
{
	Super::BeginPlay(); // Call the base component begin play. 

	InitializeWidgetComponent(); // Ensure the widget component exists. 
	SetWidgetVisible(!bStartHidden); // Apply the initial visibility state. 
}
#pragma endregion Lifecycle

// Region: Public interface. 
#pragma region PublicInterface
// Toggles the widget visibility on or off. 
void UVillagerNeedsDisplayComponent::ToggleWidgetVisibility()
{
	SetWidgetVisible(true); // Always show; no toggle to avoid accidental hide. 
}

// Explicitly sets the widget visibility. 
void UVillagerNeedsDisplayComponent::SetWidgetVisible(bool bVisible)
{
	InitializeWidgetComponent(); // Ensure the widget component exists. 

	bWidgetVisible = bVisible; // Cache the new visibility state. 
	if (!NeedsWidgetComponent) // Validate the widget component. 
	{
		return; // Exit if the widget component is missing. 
	}

	NeedsWidgetComponent->SetVisibility(bVisible, true); // Apply visibility to the widget component. 
	NeedsWidgetComponent->SetHiddenInGame(false); // Always keep visible in game. 

	if (bVisible) // Refresh data when showing the widget. 
	{
		RefreshWidgetData(); // Update the widget with needs data. 
	}
}

// Returns whether the widget is currently visible. 
bool UVillagerNeedsDisplayComponent::IsWidgetVisible() const
{
	return bWidgetVisible; // Return the cached visibility state. 
}
#pragma endregion PublicInterface

// Region: Helpers. 
#pragma region Helpers
// Ensures the widget component exists and is configured. 
void UVillagerNeedsDisplayComponent::InitializeWidgetComponent()
{
	if (NeedsWidgetComponent) // Skip initialization when already created. 
	{
		return; // Exit when the widget component already exists. 
	}

	AActor* OwnerActor = GetOwner(); // Resolve the owning actor. 
	if (!OwnerActor) // Validate owner availability. 
	{
		return; // Exit when no owner is available. 
	}

	USceneComponent* RootComponent = OwnerActor->GetRootComponent(); // Resolve the root component for attachment. 
	if (!RootComponent) // Validate root component availability. 
	{
		return; // Exit when the owner has no root component. 
	}

	NeedsWidgetComponent = NewObject<UWidgetComponent>(OwnerActor, TEXT("NeedsWidgetComponent")); // Create the widget component. 
	if (!NeedsWidgetComponent) // Validate widget component allocation. 
	{
		return; // Exit when the widget component could not be created. 
	}

	NeedsWidgetComponent->SetupAttachment(RootComponent); // Attach the widget to the actor root. 
	NeedsWidgetComponent->RegisterComponent(); // Register the component with the world. 
	NeedsWidgetComponent->SetWidgetSpace(bUseWorldSpaceWidget ? EWidgetSpace::World : EWidgetSpace::Screen); // Render the widget in the configured space for spatial visibility. 
	NeedsWidgetComponent->SetDrawAtDesiredSize(bDrawAtDesiredSize); // Let the widget size itself when requested. 
	NeedsWidgetComponent->SetTwoSided(true); // Allow visibility from both sides. 
	NeedsWidgetComponent->SetRelativeLocation(WidgetOffset); // Apply the configured offset. 

	if (!bDrawAtDesiredSize) // Apply explicit sizing when not using desired size. 
	{
		NeedsWidgetComponent->SetDrawSize(WidgetDrawSize); // Apply the configured draw size. 
	}

	TSubclassOf<UUserWidget> ResolvedWidgetClass = NeedsWidgetClass; // Start with configured widget class. 
	if (!ResolvedWidgetClass) // Apply fallback when no class is configured. 
	{
		ResolvedWidgetClass = UVillagerNeedsWidget::StaticClass(); // Default to the needs widget class. 
	}
	NeedsWidgetComponent->SetWidgetClass(ResolvedWidgetClass); // Assign the widget class to the component. 

	NeedsWidgetComponent->InitWidget(); // Instantiate the widget instance through the component. 

	if (!NeedsWidgetComponent->GetUserWidgetObject()) // Provide a manual fallback when InitWidget fails to spawn an instance. 
	{
		if (UWorld* World = GetWorld()) // Resolve world context for widget creation. 
		{
			UUserWidget* ManualWidget = CreateWidget<UUserWidget>(World, ResolvedWidgetClass.Get()); // Manually create the widget instance using the fallback class. 
			NeedsWidgetComponent->SetWidget(ManualWidget); // Assign the manually created widget to the component. 
		}
	}

	NeedsWidgetComponent->SetVisibility(true, true); // Force visible.
	NeedsWidgetComponent->SetHiddenInGame(false); // Ensure not hidden in game.

	if (GEngine) // Emit debug message confirming widget creation.
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Green, FString::Printf(TEXT("Needs widget initialized: %s"), *GetNameSafe(NeedsWidgetComponent)));
	}
}

// Updates the widget with the latest needs data. 
void UVillagerNeedsDisplayComponent::RefreshWidgetData()
{
	if (!NeedsWidgetComponent) // Validate widget component availability. 
	{
		return; // Exit when no widget component exists. 
	}

	UVillagerNeedsComponent* NeedsComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerNeedsComponent>() : nullptr; // Resolve the needs component. 
	UVillagerSocialComponent* SocialComponent = GetOwner() ? GetOwner()->FindComponentByClass<UVillagerSocialComponent>() : nullptr; // Resolve the social component.
	if (!NeedsComponent) // Validate needs component availability. 
	{
		return; // Exit when no needs component is present. 
	}

	UUserWidget* UserWidget = NeedsWidgetComponent->GetUserWidgetObject(); // Resolve the user widget instance. 
	UVillagerNeedsWidget* NeedsWidget = Cast<UVillagerNeedsWidget>(UserWidget); // Cast to the expected widget type. 
	if (!NeedsWidget) // Validate the widget type. 
	{
		if (GEngine) // Emit a debug message when the widget instance is missing. 
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Red, TEXT("VillagerNeedsDisplay: Missing needs widget instance, fallback was not applied.")); // Notify about missing widget to aid debugging. 
		}
		return; // Exit when the widget is not the expected class. 
	}

	NeedsWidget->InitializeFromNeedsAndSocial(NeedsComponent, SocialComponent); // Bind the widget to the needs and social components. 
}
#pragma endregion Helpers
