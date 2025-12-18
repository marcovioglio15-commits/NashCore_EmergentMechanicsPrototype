// Ensures the header is included only once during compilation to avoid duplicate symbols. 
#pragma once // Single-include guard directive.

// Region: Includes for commandlet declarations. 
#pragma region Includes // Begin include region.
// Provides core Unreal types and macros. 
#include "CoreMinimal.h" // Core definitions and utilities.
// Declares the base commandlet class for editor automation. 
#include "Commandlets/Commandlet.h" // Commandlet base class.
// Exposes villager data asset structs and types. 
#include "Simulation/Data/VillagerDataAssets.h" // Villager data definitions.
#pragma endregion Includes // End include region.

// Generated header include for Unreal reflection. 
#include "GenerateVillagerAssetsCommandlet.generated.h" // Auto-generated reflection data.

// Declares a commandlet for generating villager assets and curves. 
UCLASS() // Enable reflection for the commandlet class.
class UGenerateVillagerAssetsCommandlet : public UCommandlet // Derives from UCommandlet for editor execution.
{ // Begin commandlet class definition.
	// Enables reflection and boilerplate generation. 
	GENERATED_BODY() // Macro expanding to reflection code.

public: // Public interface section.
	// Initializes commandlet metadata and defaults. 
	UGenerateVillagerAssetsCommandlet(); // Constructor declaration.

	// Entry point invoked by the commandlet runner. 
	virtual int32 Main(const FString& Params) override; // Main execution method.

private: // Private helper section.
	// Builds and saves all curve assets used by villagers. 
	bool BuildCurves(); // Curve generation routine.

	// Builds and saves all villager archetype data assets. 
	bool BuildVillagers(); // Villager asset generation routine.

	// Creates or replaces a curve asset with specified keys. 
	UCurveFloat* CreateCurveFloatAsset(const FString& AssetPath, const TArray<TPair<float, float>>& Keys); // Curve creation helper.

	// Creates or replaces a villager archetype asset with configured data. 
	UVillagerArchetypeDataAsset* CreateVillagerAsset(const FString& AssetPath); // Villager asset creation helper.

	// Saves a package to disk using standard Unreal serialization. 
	bool SavePackageToDisk(UPackage* Package, UObject* Asset); // Package save helper.

	// Resolves a gameplay tag safely without hard errors. 
	FGameplayTag SafeRequestTag(const FName& TagName) const; // Tag lookup helper.

	// Builds a need definition with the supplied parameters. 
	FNeedDefinition BuildNeedDefinition(const FGameplayTag& NeedTag, float StartingValue, float MinValue, float MaxValue, float MildThreshold, float CriticalThreshold, float PriorityWeight, UCurveFloat* ForceCurve, const FGameplayTag& SatisfyingActivityTag) const; // Need definition builder.

	// Builds a daily activity definition with time window and location data. 
	FActivityDefinition BuildActivityDefinition(const FGameplayTag& ActivityTag, int32 DayOrder, int32 StartHour, int32 EndHour, const FGameplayTag& LocationTag, const FGameplayTag& RequiredResourceTag, const TMap<FGameplayTag, UCurveFloat*>& NeedCurves) const; // Activity definition builder.

	// Builds a tagged location entry with identity transform. 
	FTaggedLocation BuildTaggedLocation(const FGameplayTag& LocationTag) const; // Tagged location builder.

	// Builds an approval entry with the specified affection value. 
	FApprovalEntry BuildApprovalEntry(const FGameplayTag& VillagerIdTag, float AffectionValue) const; // Approval entry builder.

	// Builds a social definition aligned to work and approvals. 
	FSocialDefinition BuildSocialDefinition(const FGameplayTag& ProvidedResourceTag, UCurveFloat* AffectionCurve, const TArray<FGameplayTag>& ApprovalTags, const FGameplayTag& TradeLocationTag) const; // Social definition builder.

	// Builds a default movement definition for villagers. 
	FMovementDefinition BuildMovementDefinition() const; // Movement definition builder.

	// Caches curve assets by name for later assignment. 
	TMap<FName, TObjectPtr<UCurveFloat>> CurveMap; // Curve cache storage.
}; // End commandlet class definition.
