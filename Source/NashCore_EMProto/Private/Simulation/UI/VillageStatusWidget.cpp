// Includes the widget declaration. 
#include "Simulation/UI/VillageStatusWidget.h" // Imports UVillageStatusWidget. 

// Region: Engine includes. 
#pragma region EngineIncludes // Groups engine includes. 
// Provides access to world context for subsystem lookups. 
#include "Engine/World.h" // Imports UWorld. 
// Enables actor iteration when auto-binding to the first villager log. 
#include "EngineUtils.h" // Imports TActorIterator. 
// Required for the actor type used during iteration. 
#include "GameFramework/Actor.h" // Imports AActor. 
// Provides pointer event types for mouse input handling. 
#include "Input/Events.h" // Imports FPointerEvent. 
// Provides mouse button key definitions. 
#include "InputCoreTypes.h" // Imports EKeys. 
#pragma endregion EngineIncludes // Ends the engine includes region. 

// Region: UMG includes. 
#pragma region UMGIncludes // Groups UMG includes. 
// Supplies widget tree creation utilities. 
#include "Blueprint/WidgetTree.h" // Imports UWidgetTree. 
// Provides basic layout containers. 
#include "Components/VerticalBox.h" // Imports UVerticalBox. 
#include "Components/VerticalBoxSlot.h" // Imports UVerticalBoxSlot. 
// Provides text rendering widget. 
#include "Components/TextBlock.h" // Imports UTextBlock. 
// Provides scroll box for log lines. 
#include "Components/ScrollBox.h" // Imports UScrollBox. 
#include "Components/ScrollBoxSlot.h" // Imports UScrollBoxSlot. 
// Provides slate color type for text styling. 
#include "Styling/SlateColor.h" // Imports FSlateColor. 
#pragma endregion UMGIncludes // Ends the UMG includes region. 

// Region: Simulation includes. 
#pragma region SimulationIncludes // Groups simulation includes. 
// Provides access to the log component for UI binding. 
#include "Simulation/Logging/VillagerLogComponent.h" // Imports UVillagerLogComponent. 
// Provides access to the clock subsystem for UI binding. 
#include "Simulation/Time/VillageClockSubsystem.h" // Imports UVillageClockSubsystem. 
#pragma endregion SimulationIncludes // Ends the simulation includes region. 

// Groups widget lifecycle functions.  
#pragma region Lifecycle
// Ensures bindings are established even if the widget is spawned purely from C++. 
void UVillageStatusWidget::NativeConstruct()
{
	Super::NativeConstruct(); // Invoke the base widget construction. 
	UVillagerLogComponent::SetOnScreenDebugEnabled(false); // Disable on-screen debug while the log UI is visible. 

	if (!ClockText || !LogScrollBox) // Build a simple layout if none was authored in UMG. 
	{
		BuildFallbackLayout(); // Build the fallback widget tree. 
	}

	if (LogScrollBox) // Disable RMB drag scrolling to avoid consuming camera look input. 
	{
		LogScrollBox->SetAllowRightClickDragScrolling(false); // Prevent RMB from being captured by the log scroll box. 
	}

	if (!ClockSubsystem.IsValid()) // Auto-resolve the clock subsystem. 
	{
		if (UWorld* World = GetWorld()) // Resolve the current world context. 
		{
			ClockSubsystem = World->GetSubsystem<UVillageClockSubsystem>(); // Cache the clock subsystem. 
		}
	}

	// If no log source was provided, grab the first available villager in the world. 
	if (UWorld* World = GetWorld()) // Resolve the current world context. 
	{
		for (TActorIterator<AActor> It(World); It; ++It) // Iterate all actors in the world. 
		{
			if (UVillagerLogComponent* Found = It->FindComponentByClass<UVillagerLogComponent>()) // Find the first log component. 
			{
				BindToLogComponent(Found); // Bind to the discovered log component. 
			}
		}
	}

	InitializeFromSources(ClockSubsystem.Get(), nullptr); // Ensure bindings are active. 
}

// Clears dynamic bindings to avoid dangling pointers. 
void UVillageStatusWidget::NativeDestruct()
{
	if (ClockSubsystem.IsValid()) // Remove clock bindings when available. 
	{
		ClockSubsystem->OnMinuteChanged.RemoveDynamic(this, &UVillageStatusWidget::HandleMinuteChanged); // Unbind clock updates. 
	}

	for (TWeakObjectPtr<UVillagerLogComponent>& LogComp : ObservedLogComponents) // Iterate observed log components. 
	{
		if (LogComp.IsValid()) // Validate each log component. 
		{
			LogComp->OnLogLineAdded.RemoveDynamic(this, &UVillageStatusWidget::HandleLogLineAdded); // Unbind log updates. 
		}
	}

	UVillagerLogComponent::SetOnScreenDebugEnabled(true); // Re-enable on-screen debug after the log UI is removed. 
	Super::NativeDestruct(); // Invoke the base widget destruction. 
}

// Allows RMB to pass through the widget so camera rotation can start. 
FReply UVillageStatusWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) // Detect RMB to avoid UI consumption. 
	{
		return FReply::Unhandled(); // Let the game input system handle RMB. 
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent); // Defer to the default handler for other buttons. 
}

// Allows RMB release to pass through the widget for camera rotation stop. 
FReply UVillageStatusWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton) // Detect RMB release to avoid UI consumption. 
	{
		return FReply::Unhandled(); // Let the game input system handle RMB release. 
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent); // Defer to the default handler for other buttons. 
}
#pragma endregion Lifecycle // Ends the lifecycle region. 

// Region: Public API. 
#pragma region PublicAPI // Groups public API methods. 
// Binds the widget to the provided clock subsystem and log component. 
void UVillageStatusWidget::InitializeFromSources(UVillageClockSubsystem* InClockSubsystem, UVillagerLogComponent* InLogComponent)
{
	if (ClockSubsystem.IsValid()) // Clear previous clock bindings if present. 
	{
		ClockSubsystem->OnMinuteChanged.RemoveDynamic(this, &UVillageStatusWidget::HandleMinuteChanged); // Unbind existing clock updates. 
	}

	if (InClockSubsystem) // Bind to the provided clock subsystem. 
	{
		ClockSubsystem = InClockSubsystem; // Cache the new clock subsystem. 
		ClockSubsystem->OnMinuteChanged.AddDynamic(this, &UVillageStatusWidget::HandleMinuteChanged); // Bind to clock updates. 
		RefreshClockText(ClockSubsystem->GetCurrentHour(), ClockSubsystem->GetCurrentMinute()); // Initialize the clock display. 
	}

	if (InLogComponent) // Bind to the provided log component. 
	{
		BindToLogComponent(InLogComponent); // Register log bindings. 
	}

	RepopulateLog(); // Refresh the log list for all observed components. 
}
// Ends the public API region.
#pragma endregion PublicAPI  

// Groups helper methods.  
#pragma region Helpers
// Rebuilds a minimal vertical layout to display the clock and logs. 
void UVillageStatusWidget::BuildFallbackLayout()
{
	if (!WidgetTree) // Ensure the widget tree exists. 
	{
		return; // Exit when no widget tree is available. 
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox")); // Create a root vertical box. 

	ClockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ClockText")); // Create a clock text widget. 
	ClockText->SetText(FText::FromString(TEXT("Clock: --:--"))); // Initialize the clock text placeholder. 
	ClockText->SetAutoWrapText(true); // Enable wrapping for long strings. 

	LogScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("LogScrollBox")); // Create the log scroll box. 

	Root->AddChildToVerticalBox(ClockText); // Add the clock text to the root. 
	Root->AddChildToVerticalBox(LogScrollBox); // Add the log scroll box to the root. 

	WidgetTree->RootWidget = Root; // Assign the root widget for the tree. 
}

// Writes the formatted time into the clock text block. 
void UVillageStatusWidget::RefreshClockText(int32 Hour, int32 Minute)
{
	if (!ClockText) // Validate the clock text widget. 
	{
		return; // Exit when no clock text widget exists. 
	}

	FString PhaseLabel = TEXT("Day"); // Default to day phase label. 
	if (ClockSubsystem.IsValid()) // Resolve the current day phase when available. 
	{
		PhaseLabel = ClockSubsystem->GetCurrentPhase() == EVillageDayPhase::Day ? TEXT("Day") : TEXT("Night"); // Map the phase to a label. 
	}

	const FString TimeString = FString::Printf(TEXT("Clock: %02d:%02d (%s)"), Hour, Minute, *PhaseLabel); // Format the clock string. 
	ClockText->SetText(FText::FromString(TimeString)); // Apply the formatted clock string. 
}

// Clears and rebuilds the log UI from the buffered messages. 
void UVillageStatusWidget::RepopulateLog()
{
	if (!LogScrollBox) // Validate the log scroll box. 
	{
		return; // Exit when the log scroll box is missing. 
	}

	LogScrollBox->ClearChildren(); // Remove existing log entries. 

	for (const TWeakObjectPtr<UVillagerLogComponent>& LogComp : ObservedLogComponents) // Iterate observed log components. 
	{
		if (!LogComp.IsValid()) // Skip invalid components. 
		{
			continue; // Move to the next component. 
		}

		const TArray<FString> Messages = LogComp->GetRecentMessages(); // Retrieve cached messages for this component. 
		for (const FString& Message : Messages) // Iterate each buffered message. 
		{
			AddLogEntry(LogComp.Get(), Message); // Add a styled entry for each message. 
		}
	}
}

// Handles the minute change delegate from the clock subsystem. 
void UVillageStatusWidget::HandleMinuteChanged(int32 Hour, int32 Minute)
{
	RefreshClockText(Hour, Minute); // Update the clock text. 
}

// Handles new log messages emitted by a villager. 
void UVillageStatusWidget::HandleLogLineAdded(UVillagerLogComponent* SourceComponent, const FString& Message)
{
	AddLogEntry(SourceComponent, Message); // Forward to the shared log entry creation routine. 
}

// Adds a new line to the visible log list. 
void UVillageStatusWidget::AddLogEntry(UVillagerLogComponent* SourceComponent, const FString& Message)
{
	if (!LogScrollBox) // Validate the log scroll box. 
	{
		return; // Exit when the log scroll box is missing. 
	}

	UTextBlock* Entry = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()); // Allocate a new text entry widget. 
	Entry->SetText(FText::FromString(Message)); // Assign the log message text. 
	Entry->SetAutoWrapText(true); // Enable wrapping for long messages. 
	if (SourceComponent) // Apply per-villager styling when available. 
	{
		const FLinearColor EntryColor = SourceComponent->GetResolvedLogTextColor(); // Retrieve the configured color. 
		const int32 EntryFontSize = SourceComponent->GetLogFontSize(); // Retrieve the configured font size. 
		FSlateFontInfo FontInfo = Entry->GetFont(); // Copy the existing font info. 
		FontInfo.Size = EntryFontSize; // Apply the configured font size. 
		Entry->SetFont(FontInfo); // Update the text font. 
		Entry->SetColorAndOpacity(FSlateColor(EntryColor)); // Apply the configured text color. 
	}

	LogScrollBox->AddChild(Entry); // Add the log entry widget to the scroll box. 
	LogScrollBox->ScrollToEnd(); // Scroll to the latest entry. 
}

// Binds to a log component and avoids duplicate subscriptions. 
void UVillageStatusWidget::BindToLogComponent(UVillagerLogComponent* InLogComponent)
{
	if (!InLogComponent) // Validate the incoming log component. 
	{
		return; // Exit when no log component was provided. 
	}

	const bool bAlreadyBound = ObservedLogComponents.ContainsByPredicate([InLogComponent](const TWeakObjectPtr<UVillagerLogComponent>& WeakLog) // Check existing bindings. 
	{
		return WeakLog.Get() == InLogComponent; // Report true when the component is already tracked. 
	});

	if (bAlreadyBound) // Skip duplicate bindings. 
	{
		return; // Exit when already bound. 
	}

	InLogComponent->OnLogLineAdded.AddDynamic(this, &UVillageStatusWidget::HandleLogLineAdded); // Bind to log updates. 
	ObservedLogComponents.Add(InLogComponent); // Track the observed log component. 
}
#pragma endregion Helpers // Ends the helper methods region. 
