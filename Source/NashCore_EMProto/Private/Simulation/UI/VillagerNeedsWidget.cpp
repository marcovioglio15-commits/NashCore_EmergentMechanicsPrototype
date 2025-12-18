// Includes the villager needs widget declaration. 
#include "Simulation/UI/VillagerNeedsWidget.h" // Imports UVillagerNeedsWidget definitions. 

// Region: Engine includes. 
#pragma region EngineIncludes
// Provides world access for widget updates. 
#include "Engine/World.h" // Imports UWorld. 
#pragma endregion EngineIncludes

// Region: UMG includes. 
#pragma region UMGIncludes
// Supplies widget tree creation utilities. 
#include "Blueprint/WidgetTree.h" // Imports UWidgetTree. 
// Provides vertical box layout container. 
#include "Components/VerticalBox.h" // Imports UVerticalBox. 
#include "Components/VerticalBoxSlot.h" // Imports UVerticalBoxSlot. 
// Provides horizontal box layout container. 
#include "Components/HorizontalBox.h" // Imports UHorizontalBox. 
#include "Components/HorizontalBoxSlot.h" // Imports UHorizontalBoxSlot. 
// Provides text rendering widget. 
#include "Components/TextBlock.h" // Imports UTextBlock. 
#pragma endregion UMGIncludes

// Region: Simulation includes. 
#pragma region SimulationIncludes
// Provides access to needs component data. 
#include "Simulation/Needs/VillagerNeedsComponent.h" // Imports UVillagerNeedsComponent and FNeedRuntimeState. 
// Provides access to archetype data for villager id display.
#include "Simulation/Data/VillagerDataAssets.h" // Imports UVillagerArchetypeDataAsset.
// Provides formatted tag strings for display consistency.
#include "Simulation/Logging/VillagerLogComponent.h" // Imports UVillagerLogComponent utilities.
// Provides access to social data for affection display.
#include "Simulation/Social/VillagerSocialComponent.h" // Imports UVillagerSocialComponent.
#pragma endregion SimulationIncludes

// Region: Lifecycle. 
#pragma region Lifecycle
// Ensures the widget is ready to receive data. 
void UVillagerNeedsWidget::NativeConstruct()
{
	Super::NativeConstruct(); // Call the base widget construction. 

	if (!NeedsListBox || !VillagerIdText || !AffectionListBox) // Build fallback layout when required widgets are missing. 
	{
		BuildFallbackLayout(); // Create a simple vertical layout for needs and affections. 
	}

	if (NeedsComponent.IsValid()) // Rebuild and refresh if data is already assigned. 
	{
		RebuildNeedRows(); // Build UI rows for existing needs. 
		RefreshNeeds(); // Populate values into the UI. 
		RebuildAffectionRows(); // Build affection rows when social data exists.
		RefreshAffections(); // Populate affection values.
		RefreshVillagerId(); // Refresh villager id display.
	}
}

// Cleans up bindings when the widget is destroyed. 
void UVillagerNeedsWidget::NativeDestruct()
{
	if (NeedsComponent.IsValid()) // Remove delegate bindings when component is valid. 
	{
		NeedsComponent->OnNeedsUpdated.RemoveDynamic(this, &UVillagerNeedsWidget::HandleNeedsUpdated); // Unbind update callbacks. 
	}

	Super::NativeDestruct(); // Call the base widget destruction. 
}
#pragma endregion Lifecycle

// Region: Public API. 
#pragma region PublicAPI
// Initializes the widget with a needs component as data source. 
void UVillagerNeedsWidget::InitializeFromNeeds(UVillagerNeedsComponent* InNeedsComponent)
{
	InitializeFromNeedsAndSocial(InNeedsComponent, nullptr); // Delegate to unified initializer.
}

// Initializes the widget with needs and social components.
void UVillagerNeedsWidget::InitializeFromNeedsAndSocial(UVillagerNeedsComponent* InNeedsComponent, UVillagerSocialComponent* InSocialComponent)
{
	if (NeedsComponent.IsValid()) // Remove existing bindings before reassigning. 
	{
		NeedsComponent->OnNeedsUpdated.RemoveDynamic(this, &UVillagerNeedsWidget::HandleNeedsUpdated); // Unbind previous updates. 
	}

	NeedsComponent = InNeedsComponent; // Cache the new needs component. 
	SocialComponent = InSocialComponent; // Cache the social component. 

	if (!NeedsListBox || !VillagerIdText || !AffectionListBox) // Build fallback layout when required widgets are missing. 
	{
		BuildFallbackLayout(); // Create a simple vertical layout for needs and affections. 
	}

	RebuildNeedRows(); // Rebuild the rows for the new component. 
	RebuildAffectionRows(); // Rebuild affection rows.
	RefreshVillagerId(); // Update villager id text. 
	RefreshNeeds(); // Populate the UI values. 
	RefreshAffections(); // Populate affection values.

	if (NeedsComponent.IsValid()) // Bind update events when a component is available. 
	{
		NeedsComponent->OnNeedsUpdated.AddDynamic(this, &UVillagerNeedsWidget::HandleNeedsUpdated); // Bind to needs updates. 
	}
}
#pragma endregion PublicAPI

// Region: Helpers. 
#pragma region Helpers
// Builds a fallback layout when no UMG tree was authored. 
void UVillagerNeedsWidget::BuildFallbackLayout()
{
	if (!WidgetTree) // Ensure the widget tree exists. 
	{
		return; // Exit when no widget tree is available. 
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NeedsRoot")); // Create the root vertical box. 
	WidgetTree->RootWidget = RootBox; // Assign the root widget for this tree. 

	VillagerIdText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VillagerIdText")); // Create id text widget.
	VillagerIdText->SetJustification(ETextJustify::Center); // Center align id text.
	RootBox->AddChildToVerticalBox(VillagerIdText); // Add id text to layout.

	NeedsListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NeedsListBox")); // Create the needs list box.
	RootBox->AddChildToVerticalBox(NeedsListBox); // Attach needs list.

	UTextBlock* AffectionHeader = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AffectionHeader")); // Create affection header.
	AffectionHeader->SetJustification(ETextJustify::Center); // Center header.
	AffectionHeader->SetText(FText::FromString(TEXT("Affection"))); // Set header text.
	RootBox->AddChildToVerticalBox(AffectionHeader); // Add header to layout.

	AffectionListBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("AffectionListBox")); // Create affection list.
	RootBox->AddChildToVerticalBox(AffectionListBox); // Attach affection list.
}

// Rebuilds the UI rows for the current needs list. 
void UVillagerNeedsWidget::RebuildNeedRows()
{
	if (!NeedsListBox) // Validate the list box container. 
	{
		return; // Exit when the list box is missing. 
	}

	NeedRowMap.Reset(); // Clear cached row references. 
	NeedsListBox->ClearChildren(); // Remove existing row widgets. 

	if (!NeedsComponent.IsValid()) // Ensure a valid needs component is assigned. 
	{
		return; // Exit when no needs component is available. 
	}

	const TArray<FNeedRuntimeState>& RuntimeNeeds = NeedsComponent->GetRuntimeNeeds(); // Retrieve runtime needs. 
	for (const FNeedRuntimeState& Need : RuntimeNeeds) // Iterate each need definition. 
	{
		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass()); // Create a row container. 
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()); // Create label text widget. 
		UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()); // Create value text widget. 

		LabelText->SetText(FText::FromString(Need.NeedTag.ToString())); // Set the label to the need tag. 

		RowBox->AddChildToHorizontalBox(LabelText); // Add the label to the row. 
		RowBox->AddChildToHorizontalBox(ValueText); // Add the value text to the row. 
		NeedsListBox->AddChildToVerticalBox(RowBox); // Add the row to the vertical list. 

		FNeedRowWidgets RowWidgets; // Allocate a row widget container. 
		RowWidgets.LabelText = LabelText; // Cache the label widget. 
		RowWidgets.ValueText = ValueText; // Cache the value widget. 
		NeedRowMap.Add(Need.NeedTag, RowWidgets); // Store the row widgets by tag. 
	}
}

// Rebuilds the UI rows for affections.
void UVillagerNeedsWidget::RebuildAffectionRows()
{
	if (!AffectionListBox) // Validate container.
	{
		return; // Exit when missing.
	}

	AffectionRowMap.Reset(); // Clear previous rows.
	AffectionListBox->ClearChildren(); // Clear children.

	if (!SocialComponent.IsValid()) // Ensure social component exists.
	{
		return; // Nothing to show.
	}

	const TMap<FGameplayTag, float> AffectionSnapshot = SocialComponent->GetAffectionSnapshot(); // Fetch snapshot.
	for (const TPair<FGameplayTag, float>& Pair : AffectionSnapshot) // Iterate entries.
	{
		UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass()); // Create row.
		UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()); // Create label.
		UTextBlock* ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()); // Create value.

		LabelText->SetText(FText::FromString(Pair.Key.ToString())); // Assign label text.

		RowBox->AddChildToHorizontalBox(LabelText); // Add label.
		RowBox->AddChildToHorizontalBox(ValueText); // Add value.
		AffectionListBox->AddChildToVerticalBox(RowBox); // Attach row.

		FAffectionRowWidgets RowWidgets; // Allocate row cache.
		RowWidgets.LabelText = LabelText; // Cache label.
		RowWidgets.ValueText = ValueText; // Cache value.
		AffectionRowMap.Add(Pair.Key, RowWidgets); // Store by id.
	}
}

// Refreshes the UI with the latest needs values. 
void UVillagerNeedsWidget::RefreshNeeds()
{
	if (!NeedsComponent.IsValid() || !NeedsListBox) // Validate required references. 
	{
		return; // Exit when dependencies are missing. 
	}

	const TArray<FNeedRuntimeState>& RuntimeNeeds = NeedsComponent->GetRuntimeNeeds(); // Retrieve runtime needs. 
	for (const FNeedRuntimeState& Need : RuntimeNeeds) // Iterate each runtime need. 
	{
		FNeedRowWidgets* RowWidgets = NeedRowMap.Find(Need.NeedTag); // Locate the widget row for this need. 
		if (!RowWidgets) // Rebuild rows if any entry is missing. 
		{
			RebuildNeedRows(); // Rebuild the row layout for missing entries. 
			RowWidgets = NeedRowMap.Find(Need.NeedTag); // Attempt to locate the row again. 
		}

		if (!RowWidgets) // Skip when no row can be found. 
		{
			continue; // Continue to the next need entry. 
		}

		if (RowWidgets->ValueText) // Update the value text when available. 
		{
			const FString ValueString = FString::Printf(TEXT("%.3f"), Need.CurrentValue); // Format the numeric value with more precision. 
			RowWidgets->ValueText->SetText(FText::FromString(ValueString)); // Apply the formatted value text. 
		}
	}
}

// Refreshes the UI with the latest affection values.
void UVillagerNeedsWidget::RefreshAffections()
{
	if (!SocialComponent.IsValid() || !AffectionListBox) // Validate references.
	{
		return; // Exit if missing.
	}

	const TMap<FGameplayTag, float> AffectionSnapshot = SocialComponent->GetAffectionSnapshot(); // Fetch snapshot.
	for (const TPair<FGameplayTag, float>& Pair : AffectionSnapshot) // Iterate entries.
	{
		FAffectionRowWidgets* RowWidgets = AffectionRowMap.Find(Pair.Key); // Find row.
		if (!RowWidgets) // Rebuild if missing.
		{
			RebuildAffectionRows(); // Rebuild affection rows.
			RowWidgets = AffectionRowMap.Find(Pair.Key); // Retry lookup.
		}

		if (!RowWidgets) // Skip if still missing.
		{
			continue; // Continue loop.
		}

		if (RowWidgets->ValueText) // Update value.
		{
			const FString ValueString = FString::Printf(TEXT("%.2f"), Pair.Value); // Format value.
			RowWidgets->ValueText->SetText(FText::FromString(ValueString)); // Apply text.
		}
	}
}

// Handles needs updates from the component. 
void UVillagerNeedsWidget::HandleNeedsUpdated(UVillagerNeedsComponent* UpdatedComponent)
{
	if (NeedsComponent.Get() != UpdatedComponent) // Ignore updates from unrelated components. 
	{
		return; // Exit when the update does not match this widget. 
	}

	RefreshVillagerId(); // Keep villager id text in sync. 
	RefreshNeeds(); // Refresh the UI with the latest values. 
	RefreshAffections(); // Refresh affection values. 
}

// Normalizes a need value into the 0-1 range. 
float UVillagerNeedsWidget::NormalizeNeedValue(const FNeedRuntimeState& Need) const
{
	const float Range = FMath::Max(KINDA_SMALL_NUMBER, Need.Definition.MaxValue - Need.Definition.MinValue); // Compute a safe value range. 
	return (Need.CurrentValue - Need.Definition.MinValue) / Range; // Return the normalized value. 
}

// Refreshes the villager identifier text.
void UVillagerNeedsWidget::RefreshVillagerId()
{
	if (!VillagerIdText || !NeedsComponent.IsValid()) // Validate dependencies.
	{
		return; // Exit if missing.
	}

	if (UVillagerArchetypeDataAsset* Archetype = NeedsComponent->GetArchetype()) // Resolve archetype.
	{
		const FString IdString = UVillagerLogComponent::GetShortTagString(Archetype->VillagerIdTag); // Format id tag.
		VillagerIdText->SetText(FText::FromString(IdString)); // Apply id string.
	}
}
#pragma endregion Helpers
