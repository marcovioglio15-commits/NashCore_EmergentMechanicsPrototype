// Prevents multiple inclusion of the villager needs widget header. 
#pragma once // Ensures the header is included only once. 

// Region: Includes. 
#pragma region Includes
// Provides engine core types and macros. 
#include "CoreMinimal.h" // Imports CoreMinimal definitions. 
// Supplies the base user widget class. 
#include "Blueprint/UserWidget.h" // Imports UUserWidget. 
// Provides gameplay tag types for need identifiers. 
#include "GameplayTagContainer.h" // Imports FGameplayTag. 
#pragma endregion Includes // Ends the includes region. 

// Region: Forward declarations. 
#pragma region ForwardDeclarations
// Forward-declared vertical box widget type. 
class UVerticalBox; 
// Forward-declared text block widget type. 
class UTextBlock; 
// Forward-declared needs component type. 
class UVillagerNeedsComponent; 
// Forward-declared need runtime state struct. 
struct FNeedRuntimeState; 
// Forward-declared social component type.
class UVillagerSocialComponent;
#pragma endregion ForwardDeclarations // Ends the forward declarations region. 

// Generated header required by Unreal reflection. 
#include "VillagerNeedsWidget.generated.h" // Includes generated reflection data. 

// Region: Villager needs widget declaration. 
#pragma region VillagerNeedsWidgetDeclaration
// Widget that displays the current needs state for a villager. 
UCLASS(BlueprintType, Blueprintable) // Enables UMG usage and reflection. 
class UVillagerNeedsWidget : public UUserWidget // Declares the widget class type. 
{
	GENERATED_BODY() // Enables reflection and constructors. 

public:
#pragma region PublicInterface
	// Initializes the widget with a needs component as data source. 
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void InitializeFromNeeds(UVillagerNeedsComponent* InNeedsComponent);

	// Initializes the widget with both needs and social components for richer data.
	UFUNCTION(BlueprintCallable, Category = "Villager Needs")
	void InitializeFromNeedsAndSocial(UVillagerNeedsComponent* InNeedsComponent, UVillagerSocialComponent* InSocialComponent);
#pragma endregion PublicInterface

protected:
#pragma region ProtectedInterface
	// Ensures the widget is ready to receive data. 
	virtual void NativeConstruct() override;

	// Cleans up bindings when the widget is destroyed. 
	virtual void NativeDestruct() override;
#pragma endregion ProtectedInterface

private:
#pragma region PrivateTypes
	// Runtime widgets used to represent a single need row. 
	struct FNeedRowWidgets
	{
		UTextBlock* LabelText = nullptr; // Displays the need name. 
		UTextBlock* ValueText = nullptr; // Displays the numeric value. 
	};

	// Runtime widgets used to represent a single affection row.
	struct FAffectionRowWidgets
	{
		UTextBlock* LabelText = nullptr; // Displays the other villager id.
		UTextBlock* ValueText = nullptr; // Displays affection value.
	};
#pragma endregion PrivateTypes

private:
#pragma region PrivateMethods
	// Builds a fallback layout when no UMG tree was authored. 
	void BuildFallbackLayout();

	// Rebuilds the UI rows for the current needs list. 
	void RebuildNeedRows();

	// Refreshes the UI with the latest needs values. 
	void RefreshNeeds();

	// Rebuilds the UI rows for current affection entries.
	void RebuildAffectionRows();

	// Refreshes the UI with the latest affection values.
	void RefreshAffections();

	// Handles needs updates from the component. 
	UFUNCTION()
	void HandleNeedsUpdated(UVillagerNeedsComponent* UpdatedComponent);

	// Normalizes a need value into the 0-1 range. 
	float NormalizeNeedValue(const FNeedRuntimeState& Need) const;

	// Refreshes the villager identifier text block.
	void RefreshVillagerId();
#pragma endregion PrivateMethods

private:
#pragma region PrivateState
	// Optional list box bound from UMG for need rows. 
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> NeedsListBox;

	// Optional villager id text bound from UMG.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> VillagerIdText;

	// Optional list box bound from UMG for affection rows.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> AffectionListBox;

	// Cached needs component used as data source. 
	TWeakObjectPtr<UVillagerNeedsComponent> NeedsComponent;

	// Cached social component used as data source for affections.
	TWeakObjectPtr<UVillagerSocialComponent> SocialComponent;

	// Cached widget rows mapped by need tag. 
	TMap<FGameplayTag, FNeedRowWidgets> NeedRowMap;

	// Cached affection rows mapped by villager id.
	TMap<FGameplayTag, FAffectionRowWidgets> AffectionRowMap;
#pragma endregion PrivateState
};
#pragma endregion VillagerNeedsWidgetDeclaration // Ends the widget declaration region. 
