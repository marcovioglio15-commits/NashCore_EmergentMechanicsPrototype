// Prevents multiple inclusion of the log component header.
#pragma once

// Region: Includes.
#pragma region Includes
// Provides engine core types and macros.
#include "CoreMinimal.h"
// Supplies the base actor component functionality.
#include "Components/ActorComponent.h"
// Exposes gameplay tag type for villager identifiers.
#include "GameplayTagContainer.h"
// Brings in delegate declarations for notifying UI.
#include "Delegates/DelegateCombinations.h"
#pragma endregion Includes

// Generated header required by Unreal reflection.
#include "VillagerLogComponent.generated.h"

// Forward declaration for delegate signatures.
class UVillagerLogComponent;

// Raised whenever a new log line is added so UI can subscribe.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVillagerLogLineAdded, UVillagerLogComponent*, SourceComponent, const FString&, Message);

// Component that emits on-screen logs for simulation events.
UCLASS(ClassGroup = (Simulation), Blueprintable, meta = (BlueprintSpawnableComponent))
class UVillagerLogComponent : public UActorComponent
{
	GENERATED_BODY() // Enables reflection and constructors.

public:
	// Constructor disabling tick.
	UVillagerLogComponent();

	// Emits a formatted log line to the screen.
	void LogMessage(const FString& Message);

	// Enables or disables on-screen debug rendering for all villager log components.
	static void SetOnScreenDebugEnabled(bool bEnabled);

	// Returns whether on-screen debug rendering is currently enabled.
	static bool IsOnScreenDebugEnabled();

	// Returns the resolved log text color for this villager.
	FLinearColor GetResolvedLogTextColor() const;

	// Returns the configured log font size for this villager.
	int32 GetLogFontSize() const;

	// Emits a log line with explicit actor/target villager tags for clarity.
	UFUNCTION(BlueprintCallable, Category = "Villager Log")
	void LogAction(const FString& ActionDescription, const FGameplayTag& TargetVillagerTag = FGameplayTag());

	// Returns a shortened tag string without parent prefixes.
	static FString GetShortTagString(const FGameplayTag& Tag);

	// Sets the identifier tag representing the owner villager.
	UFUNCTION(BlueprintCallable, Category = "Villager Log")
	void SetVillagerIdTag(const FGameplayTag& InVillagerId);

	// Returns the buffered log lines for UI consumption.
	UFUNCTION(BlueprintCallable, Category = "Villager Log")
	TArray<FString> GetRecentMessages() const { return RecentMessages; }

	// Delegate fired whenever a new message arrives.
	UPROPERTY(BlueprintAssignable, Category = "Villager Log")
	FOnVillagerLogLineAdded OnLogLineAdded;

	// Maximum number of log lines to retain for the widget.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager Log")
	int32 MaxStoredMessages = 50;

	// Default log text color for this villager.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager Log")
	FLinearColor LogTextColor = FLinearColor::White;

	// Font size used for this villager's log entries.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager Log")
	int32 LogFontSize = 12;

	// Automatically derive a unique log color from the villager identifier.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager Log")
	bool bAutoAssignColorFromId = true;

	// Identifier tag for the owning villager, used to prefix log lines.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Villager Log")
	FGameplayTag VillagerIdTag;

private:
	// Global toggle for on-screen debug rendering to prevent duplicate UI output.
	static bool bOnScreenDebugEnabled;

	// Cached auto-assigned color for this villager.
	mutable FLinearColor CachedAutoColor = FLinearColor::White;

	// Tracks whether the auto color has been computed.
	mutable bool bHasCachedAutoColor = false;

	// In-memory buffer of recent log lines for UI display.
	UPROPERTY()
	TArray<FString> RecentMessages;
};
