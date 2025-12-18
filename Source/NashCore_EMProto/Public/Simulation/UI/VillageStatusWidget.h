// Prevents multiple inclusion of the widget header. 
#pragma once // Ensures the header is included only once. 

// Region: Includes. 
#pragma region Includes // Groups header includes. 
// Provides base types and helper macros. 
#include "CoreMinimal.h" // Imports core Unreal types. 
// Supplies the base user widget class. 
#include "Blueprint/UserWidget.h" // Imports UUserWidget. 
// Provides the FReply type for input overrides. 
#include "Input/Reply.h" // Imports FReply. 
#pragma endregion Includes // Ends the includes region. 

// Groups forward declarations. 
#pragma region ForwardDeclarations 
// Forward-declared scroll box widget. 
class UScrollBox; 
// Forward-declared text block widget. 
class UTextBlock; 
// Forward-declared clock subsystem type. 
class UVillageClockSubsystem; 
// Forward-declared villager log component type. 
class UVillagerLogComponent; 
// Forward-declared geometry struct for mouse events. 
struct FGeometry; 
// Forward-declared pointer event struct for mouse input. 
struct FPointerEvent; 
#pragma endregion ForwardDeclarations // Ends the forward declaration region. 

// Generated header include required by Unreal reflection. 
#include "VillageStatusWidget.generated.h" // Includes generated reflection data. 

// Region: Widget declaration. 
#pragma region VillageStatusWidgetDeclaration // Groups the widget declaration. 
// Simple status widget that shows the simulation clock and recent log messages. 
UCLASS(BlueprintType, Blueprintable) // Exposes the widget to Blueprints. 
class UVillageStatusWidget : public UUserWidget // Declares the widget type. 
{ // Opens the widget class declaration. 
	GENERATED_BODY() // Enables reflection and constructors. 

public: // Begins the public section. 
#pragma region PublicInterface // Groups public methods. 
	// Initializes bindings to the clock subsystem and a villager log component. 
	UFUNCTION(BlueprintCallable, Category = "Village UI") // Exposes the initialization to Blueprints. 
	void InitializeFromSources(UVillageClockSubsystem* InClockSubsystem, UVillagerLogComponent* InLogComponent); // Binds the widget to simulation sources. 
#pragma endregion PublicInterface // Ends the public interface region. 

protected: // Begins the protected section. 
#pragma region ProtectedInterface // Groups protected overrides and widget bindings. 
	// Hook to ensure bindings are established when the widget is constructed. 
	virtual void NativeConstruct() override; 

	// Cleans up bindings when the widget is destroyed. 
	virtual void NativeDestruct() override; 

	// Allows RMB to pass through the widget so camera rotation can start. 
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override; 

	// Allows RMB release to pass through the widget for camera rotation stop. 
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override; 

	// Optional clock text block exposed for UMG binding. 
	UPROPERTY(meta = (BindWidgetOptional)) // Allows optional widget binding from UMG. 
	TObjectPtr<UTextBlock> ClockText; 

	// Optional scroll box used to render log lines in UMG. 
	UPROPERTY(meta = (BindWidgetOptional)) // Allows optional widget binding from UMG. 
	TObjectPtr<UScrollBox> LogScrollBox; 
#pragma endregion ProtectedInterface // Ends the protected interface region. 

private: // Groups private helper methods. 
#pragma region PrivateMethods
	// Builds a minimal layout in code if the widget was not designed in UMG. 
	void BuildFallbackLayout(); 

	// Updates the clock text with the provided time. 
	void RefreshClockText(int32 Hour, int32 Minute); 

	// Rebuilds the visible log list from the buffered messages. 
	void RepopulateLog(); 

	// Handles minute changes from the clock subsystem. 
	UFUNCTION() // Marks the handler for delegate binding. 
	void HandleMinuteChanged(int32 Hour, int32 Minute); 

	// Handles new log messages emitted by a villager. 
	UFUNCTION() // Marks the handler for delegate binding. 
	void HandleLogLineAdded(UVillagerLogComponent* SourceComponent, const FString& Message); 

	// Adds a styled log entry to the scroll box. 
	void AddLogEntry(UVillagerLogComponent* SourceComponent, const FString& Message); 

	// Binds to a log component if not already observed. 
	void BindToLogComponent(UVillagerLogComponent* InLogComponent); 
#pragma endregion PrivateMethods // Ends the private methods region. 

private: // Begins the private state section. 
#pragma region PrivateState // Groups private state variables. 
	// Cached pointer to the clock subsystem. 
	TWeakObjectPtr<UVillageClockSubsystem> ClockSubsystem; 

	// Cached pointers to all villager log components being observed. 
	UPROPERTY() // Keeps the observed components referenced for GC. 
	TArray<TWeakObjectPtr<UVillagerLogComponent>> ObservedLogComponents; 
#pragma endregion PrivateState // Ends the private state region. 
}; // Ends the widget class declaration. 
#pragma endregion VillageStatusWidgetDeclaration // Ends the widget declaration region. 
