// Prevents multiple inclusion of the villager needs display component header. 
#pragma once // Ensures the header is included only once. 

// Region: Includes. 
#pragma region Includes
// Provides engine core types and macros. 
#include "CoreMinimal.h" // Imports CoreMinimal definitions. 
// Supplies actor component base functionality. 
#include "Components/ActorComponent.h" // Imports UActorComponent. 
#pragma endregion Includes // Ends the includes region. 

// Region: Forward declarations. 
#pragma region ForwardDeclarations
// Forward-declared widget component type. 
class UWidgetComponent; 
// Forward-declared user widget base type. 
class UUserWidget; 
#pragma endregion ForwardDeclarations // Ends the forward declarations region. 

// Generated header required by Unreal reflection. 
#include "VillagerNeedsDisplayComponent.generated.h" // Includes generated reflection data. 

// Region: Villager needs display component declaration. 
#pragma region VillagerNeedsDisplayComponentDeclaration
// Component that manages a world-space needs widget for a villager. 
UCLASS(ClassGroup = (UI), Blueprintable, meta = (BlueprintSpawnableComponent)) // Enables editor and BP usage. 
class UVillagerNeedsDisplayComponent : public UActorComponent // Declares the display component class. 
{
	GENERATED_BODY() // Enables reflection and constructors. 

public:
#pragma region PublicInterface
	// Constructs the component with default settings. 
	UVillagerNeedsDisplayComponent();

	// Toggles the widget visibility on or off. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void ToggleWidgetVisibility();

	// Explicitly sets the widget visibility. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void SetWidgetVisible(bool bVisible);

	// Returns whether the widget is currently visible. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	bool IsWidgetVisible() const;
#pragma endregion PublicInterface

protected:
#pragma region ProtectedInterface
	// Builds the widget component at begin play. 
	virtual void BeginPlay() override;
#pragma endregion ProtectedInterface

public:
#pragma region PublicMethods
	// Ensures the widget component exists and is configured. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void InitializeWidgetComponent();

	// Updates the widget with the latest needs data. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void RefreshWidgetData();
#pragma endregion PublicMethods

private:
#pragma region PrivateState
	// Widget class to instantiate for the needs display. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	TSubclassOf<UUserWidget> NeedsWidgetClass;

	// Relative offset for the widget component. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 150.0f);

	// Draw size for the widget component. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	FVector2D WidgetDrawSize = FVector2D(320.0f, 200.0f);

	// Whether to render the widget in world space for spatial placement. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	bool bUseWorldSpaceWidget = true;

	// Whether the widget should size itself to the desired size. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	bool bDrawAtDesiredSize = true;

	// Whether the widget starts hidden. 
	UPROPERTY(EditAnywhere, Category = "Villager Needs")
	bool bStartHidden = false;

	// Runtime widget component instance. 
	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> NeedsWidgetComponent;

	// Tracks whether the widget is visible. 
	bool bWidgetVisible = false;
#pragma endregion PrivateState
};
#pragma endregion VillagerNeedsDisplayComponentDeclaration // Ends the component declaration region. 
