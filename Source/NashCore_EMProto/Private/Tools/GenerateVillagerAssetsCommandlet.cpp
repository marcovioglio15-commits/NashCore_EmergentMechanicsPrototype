// Includes the commandlet header for definitions.
#include "Tools/GenerateVillagerAssetsCommandlet.h" // Commandlet declaration header.

// Region: Engine includes.
#pragma region EngineIncludes // Begin engine include region.
#include "Curves/CurveFloat.h" // Provides UCurveFloat asset type.
#include "Curves/RichCurve.h" // Provides FRichCurve and key utilities.
#include "GameplayTagsManager.h" // Provides gameplay tag lookup utilities.
#include "Misc/PackageName.h" // Provides package name and path helpers.
#include "UObject/Package.h" // Provides package creation and management.
#include "UObject/SavePackage.h" // Provides save package functionality.
#include "HAL/FileManager.h" // Provides file system helpers.
#pragma endregion EngineIncludes // End engine include region.

// Defines a local log category for asset generation diagnostics.
DEFINE_LOG_CATEGORY_STATIC(LogGenerateVillagerAssets, Log, All); // Local log category.

// Region: Local constants.
#pragma region LocalConstants // Begin local constants region.
namespace // Anonymous namespace to restrict linkage scope.
{
	static const FString CurvesRoot = TEXT("/Game/Programming/Curves"); // Root path for curve assets.
	static const FString VillagerRoot = TEXT("/Game/Programming/DataAsset/Villager"); // Root path for villager assets.
	static const float DefaultApprovalValue = 0.1f; // Baseline affection for approvals.
} // End anonymous namespace.
#pragma endregion LocalConstants // End local constants region.

// Region: Commandlet lifecycle.
#pragma region CommandletLifecycle // Begin commandlet lifecycle region.
UGenerateVillagerAssetsCommandlet::UGenerateVillagerAssetsCommandlet() // Constructor definition.
{
	IsClient = false; // Disable client behavior.
	IsServer = false; // Disable server behavior.
	LogToConsole = true; // Enable console logging.
} // End constructor.

int32 UGenerateVillagerAssetsCommandlet::Main(const FString& Params) // Main commandlet entry point.
{
	UE_LOG(LogGenerateVillagerAssets, Log, TEXT("Starting villager asset generation.")); // Log start.

	if (!BuildCurves()) // Attempt to build curves.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Curve generation failed.")); // Log curve failure.
		return 1; // Return failure code.
	}

	if (!BuildVillagers()) // Attempt to build villager assets.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Villager asset generation failed.")); // Log villager failure.
		return 1; // Return failure code.
	}

	UE_LOG(LogGenerateVillagerAssets, Log, TEXT("Villager asset generation completed.")); // Log success.
	return 0; // Return success code.
} // End Main.
#pragma endregion CommandletLifecycle // End commandlet lifecycle region.

// Region: Curve creation.
#pragma region CurveCreation // Begin curve creation region.
UCurveFloat* UGenerateVillagerAssetsCommandlet::CreateCurveFloatAsset(const FString& AssetPath, const TArray<TPair<float, float>>& Keys) // Curve asset creation helper.
{
	const FString PackageName = FPackageName::ObjectPathToPackageName(AssetPath); // Derive package name from object path.
	const FString AssetName = FPackageName::GetShortName(AssetPath); // Extract asset name.

	UPackage* Package = CreatePackage(*PackageName); // Create or load package.
	if (Package) // Validate package pointer.
	{
		Package->FullyLoad(); // Fully load existing package contents.
	}

	if (!Package) // Verify package creation success.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Failed to create package for %s"), *AssetPath); // Log package creation failure.
		return nullptr; // Abort on failure.
	}

	if (UObject* Existing = StaticFindObject(UObject::StaticClass(), Package, *AssetName)) // Check for existing asset.
	{
		Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty); // Move existing asset to transient to avoid conflicts.
	}

	UCurveFloat* CurveAsset = NewObject<UCurveFloat>(Package, *AssetName, RF_Public | RF_Standalone); // Create new curve asset.
	if (!CurveAsset) // Validate curve creation.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Failed to create curve asset for %s"), *AssetPath); // Log curve creation failure.
		return nullptr; // Abort on failure.
	}

	CurveAsset->FloatCurve.Reset(); // Clear any existing curve data.
	for (const TPair<float, float>& Pair : Keys) // Iterate provided key pairs.
	{
		const FKeyHandle Handle = CurveAsset->FloatCurve.AddKey(Pair.Key, Pair.Value); // Add key and capture handle.
		FRichCurveKey& NewKey = CurveAsset->FloatCurve.GetKey(Handle); // Resolve key reference from handle.
		NewKey.InterpMode = ERichCurveInterpMode::RCIM_Linear; // Use linear interpolation.
		NewKey.TangentMode = ERichCurveTangentMode::RCTM_Auto; // Use auto tangents.
		NewKey.TangentWeightMode = ERichCurveTangentWeightMode::RCTWM_WeightedNone; // Use default tangent weights.
	} // End key iteration.

	CurveAsset->FloatCurve.AutoSetTangents(); // Recompute tangents for smoothness.
	CurveAsset->bIsEventCurve = false; // Ensure curve is treated as value curve.
	CurveAsset->MarkPackageDirty(); // Mark package dirty for saving.

	if (!SavePackageToDisk(Package, CurveAsset)) // Save package containing curve.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Failed to save curve asset %s"), *AssetPath); // Log save failure.
		return nullptr; // Abort on failure.
	}

	return CurveAsset; // Return created curve asset.
} // End CreateCurveFloatAsset.

bool UGenerateVillagerAssetsCommandlet::BuildCurves() // Curve build routine.
{
	CurveMap.Reset(); // Clear cached curves.

	UCurveFloat* ForceHunger = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Force_Hunger"), { {0.0f, 1.0f}, {0.3f, 0.85f}, {0.6f, 0.35f}, {1.0f, 0.05f} }); // Create hunger force curve (higher force when satisfaction is low).
	if (!ForceHunger) // Validate hunger force curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Force_Hunger"), ForceHunger); // Cache hunger force curve.

	UCurveFloat* ForceThirst = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Force_Thirst"), { {0.0f, 1.0f}, {0.3f, 0.85f}, {0.6f, 0.35f}, {1.0f, 0.05f} }); // Create thirst force curve (higher force when satisfaction is low).
	if (!ForceThirst) // Validate thirst force curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Force_Thirst"), ForceThirst); // Cache thirst force curve.

	UCurveFloat* ForceSleep = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Force_Sleep"), { {0.0f, 1.0f}, {0.3f, 0.85f}, {0.6f, 0.35f}, {1.0f, 0.05f} }); // Create sleep force curve (higher force when satisfaction is low).
	if (!ForceSleep) // Validate sleep force curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Force_Sleep"), ForceSleep); // Cache sleep force curve.

    UCurveFloat* WorkHunger = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Working_Hunger"), { {0.0f, -0.0025f}, {1440.0f, -0.0025f} }); // Create working hunger decay curve.
	if (!WorkHunger) // Validate working hunger curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Work_Hunger"), WorkHunger); // Cache working hunger curve.

    UCurveFloat* WorkThirst = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Working_Thirst"), { {0.0f, -0.0030f}, {1440.0f, -0.0030f} }); // Create working thirst decay curve.
	if (!WorkThirst) // Validate working thirst curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Work_Thirst"), WorkThirst); // Cache working thirst curve.

    UCurveFloat* WorkSleep = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Working_Sleep"), { {0.0f, -0.0020f}, {1440.0f, -0.0020f} }); // Create working sleep decay curve.
	if (!WorkSleep) // Validate working sleep curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Work_Sleep"), WorkSleep); // Cache working sleep curve.

    UCurveFloat* EatHunger = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Eating_Hunger"), { {0.0f, 0.02f}, {120.0f, 0.02f} }); // Create eating hunger recovery curve.
	if (!EatHunger) // Validate eating curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Eat_Hunger"), EatHunger); // Cache eating curve.

    UCurveFloat* EatThirst = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Eating_Thirst"), { {0.0f, -0.0015f}, {120.0f, -0.0015f} }); // Create eating thirst decay curve.
	if (!EatThirst) // Validate eating thirst curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Eat_Thirst"), EatThirst); // Cache eating thirst curve.

    UCurveFloat* EatSleep = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Eating_Sleep"), { {0.0f, -0.0010f}, {120.0f, -0.0010f} }); // Create eating sleep decay curve.
	if (!EatSleep) // Validate eating sleep curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Eat_Sleep"), EatSleep); // Cache eating sleep curve.

    UCurveFloat* DrinkThirst = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Drinking_Thirst"), { {0.0f, 0.02f}, {90.0f, 0.02f} }); // Create drinking thirst recovery curve.
	if (!DrinkThirst) // Validate drinking curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Drink_Thirst"), DrinkThirst); // Cache drinking curve.

    UCurveFloat* DrinkHunger = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Drinking_Hunger"), { {0.0f, -0.0010f}, {90.0f, -0.0010f} }); // Create drinking hunger decay curve.
	if (!DrinkHunger) // Validate drinking hunger curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Drink_Hunger"), DrinkHunger); // Cache drinking hunger curve.

    UCurveFloat* DrinkSleep = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Drinking_Sleep"), { {0.0f, -0.0010f}, {90.0f, -0.0010f} }); // Create drinking sleep decay curve.
	if (!DrinkSleep) // Validate drinking sleep curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Drink_Sleep"), DrinkSleep); // Cache drinking sleep curve.

    UCurveFloat* SleepCurve = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Sleeping_Sleep"), { {0.0f, 0.006f}, {480.0f, 0.006f} }); // Create sleeping recovery curve.
	if (!SleepCurve) // Validate sleeping curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Sleep_Sleep"), SleepCurve); // Cache sleeping curve.

    UCurveFloat* SleepHunger = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Sleeping_Hunger"), { {0.0f, -0.0015f}, {480.0f, -0.0015f} }); // Create sleeping hunger decay curve.
	if (!SleepHunger) // Validate sleeping hunger curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Sleep_Hunger"), SleepHunger); // Cache sleeping hunger curve.

    UCurveFloat* SleepThirst = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_Sleeping_Thirst"), { {0.0f, -0.0015f}, {480.0f, -0.0015f} }); // Create sleeping thirst decay curve.
	if (!SleepThirst) // Validate sleeping thirst curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Sleep_Thirst"), SleepThirst); // Cache sleeping thirst curve.

	UCurveFloat* AffectionCurve = CreateCurveFloatAsset(CurvesRoot + TEXT("/Curve_AffectionToQuantity"), { {-1.0f, 0.25f}, {0.0f, 1.0f}, {1.0f, 2.0f} }); // Create affection curve.
	if (!AffectionCurve) // Validate affection curve.
	{
		return false; // Abort on failure.
	}
	CurveMap.Add(TEXT("Affection_Quantity"), AffectionCurve); // Cache affection curve.

	return true; // Indicate curve build success.
} // End BuildCurves.
#pragma endregion CurveCreation // End curve creation region.

// Region: Villager data construction.
#pragma region VillagerConstruction // Begin villager construction region.
UVillagerArchetypeDataAsset* UGenerateVillagerAssetsCommandlet::CreateVillagerAsset(const FString& AssetPath) // Villager asset creation helper.
{
	const FString PackageName = FPackageName::ObjectPathToPackageName(AssetPath); // Derive package name.
	const FString AssetName = FPackageName::GetShortName(AssetPath); // Extract asset name.

	UPackage* Package = CreatePackage(*PackageName); // Create or load package.
	if (Package) // Validate package pointer.
	{
		Package->FullyLoad(); // Fully load existing contents.
	}

	if (!Package) // Verify package creation success.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Failed to create package for %s"), *AssetPath); // Log package failure.
		return nullptr; // Abort on failure.
	}

	if (UObject* Existing = StaticFindObject(UObject::StaticClass(), Package, *AssetName)) // Check for existing asset.
	{
		Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_DoNotDirty); // Move existing asset away.
	}

	UVillagerArchetypeDataAsset* DataAsset = NewObject<UVillagerArchetypeDataAsset>(Package, *AssetName, RF_Public | RF_Standalone); // Create new data asset.
	if (!DataAsset) // Validate asset creation.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Failed to create villager asset for %s"), *AssetPath); // Log asset failure.
		return nullptr; // Abort on failure.
	}

	return DataAsset; // Return created data asset.
} // End CreateVillagerAsset.

FGameplayTag UGenerateVillagerAssetsCommandlet::SafeRequestTag(const FName& TagName) const // Safe tag resolver.
{
	const FGameplayTag Tag = UGameplayTagsManager::Get().RequestGameplayTag(TagName, false); // Request tag without errors.
	if (!Tag.IsValid()) // Check tag validity.
	{
		UE_LOG(LogGenerateVillagerAssets, Warning, TEXT("Missing gameplay tag: %s"), *TagName.ToString()); // Warn on missing tag.
	}
	return Tag; // Return resolved tag (may be invalid).
} // End SafeRequestTag.

FNeedDefinition UGenerateVillagerAssetsCommandlet::BuildNeedDefinition(const FGameplayTag& NeedTag, float StartingValue, float MinValue, float MaxValue, float MildThreshold, float CriticalThreshold, float PriorityWeight, UCurveFloat* ForceCurve, const FGameplayTag& SatisfyingActivityTag) const // Need definition builder.
{
	FNeedDefinition Definition; // Instantiate definition.
	Definition.NeedTag = NeedTag; // Assign need tag.
	Definition.StartingValue = StartingValue; // Set starting value.
	Definition.MinValue = MinValue; // Set minimum value.
	Definition.MaxValue = MaxValue; // Set maximum value.
	Definition.Thresholds.MildThreshold = MildThreshold; // Set mild threshold.
	Definition.Thresholds.CriticalThreshold = CriticalThreshold; // Set critical threshold.
	Definition.PriorityWeight = PriorityWeight; // Set priority weight.
	Definition.ForceActivityProbabilityCurve = ForceCurve; // Assign force curve.
	Definition.SatisfyingActivityTag = SatisfyingActivityTag; // Assign satisfying activity.
	return Definition; // Return configured definition.
} // End BuildNeedDefinition.

FActivityDefinition UGenerateVillagerAssetsCommandlet::BuildActivityDefinition(const FGameplayTag& ActivityTag, int32 DayOrder, int32 StartHour, int32 EndHour, const FGameplayTag& LocationTag, const FGameplayTag& RequiredResourceTag, const TMap<FGameplayTag, UCurveFloat*>& NeedCurves) const // Activity definition builder.
{
	FActivityDefinition Definition; // Instantiate definition.
	Definition.ActivityTag = ActivityTag; // Assign activity tag.
	Definition.bIsPartOfDay = true; // Mark as part of day.
	Definition.DayOrder = DayOrder; // Set day order.
	Definition.NeedCurves = NeedCurves; // Assign need curves.
	Definition.bRequiresSpecificLocation = true; // Require location.
	Definition.ActivityLocationTag = LocationTag; // Assign location tag.
	Definition.RequiredResourceTag = RequiredResourceTag; // Assign required resource.
	Definition.PartOfDayWindow.AllowedStartHour = StartHour; // Set window start.
	Definition.PartOfDayWindow.AllowedEndHour = EndHour; // Set window end.
	return Definition; // Return configured definition.
} // End BuildActivityDefinition.

FTaggedLocation UGenerateVillagerAssetsCommandlet::BuildTaggedLocation(const FGameplayTag& LocationTag) const // Tagged location builder.
{
	FTaggedLocation Location; // Instantiate tagged location.
	Location.LocationTag = LocationTag; // Assign location tag.
	Location.LocationTransform = FTransform::Identity; // Use identity transform.
	return Location; // Return configured location.
} // End BuildTaggedLocation.

FApprovalEntry UGenerateVillagerAssetsCommandlet::BuildApprovalEntry(const FGameplayTag& VillagerIdTag, float AffectionValue) const // Approval entry builder.
{
	FApprovalEntry Entry; // Instantiate approval entry.
	Entry.VillagerIdTag = VillagerIdTag; // Assign villager id tag.
	Entry.AffectionValue = AffectionValue; // Assign affection value.
	return Entry; // Return configured entry.
} // End BuildApprovalEntry.

FSocialDefinition UGenerateVillagerAssetsCommandlet::BuildSocialDefinition(const FGameplayTag& ProvidedResourceTag, UCurveFloat* AffectionCurve, const TArray<FGameplayTag>& ApprovalTags, const FGameplayTag& TradeLocationTag) const // Social definition builder.
{
	FSocialDefinition Social; // Instantiate social definition.
	Social.ProvidedResourceTag = ProvidedResourceTag; // Assign provided resource tag.
	Social.AffectionToQuantityCurve = AffectionCurve; // Assign affection curve.
	Social.Approvals.Reset(); // Clear approvals array.
	for (const FGameplayTag& ApprovalTag : ApprovalTags) // Iterate approval tags.
	{
		Social.Approvals.Add(BuildApprovalEntry(ApprovalTag, DefaultApprovalValue)); // Add approval entry.
	}
	Social.TradeLocations.Reset(); // Clear trade locations array.
	Social.TradeLocations.Add(BuildTaggedLocation(TradeLocationTag)); // Add trade location aligned to work.
	Social.BuyerAffectionGainOnTrade = 0.05f; // Set buyer affection gain.
	Social.SellerAffectionGainPerTrade = 0.025f; // Set seller affection gain.
	Social.AffectionLossOnMiss = 0.1f; // Set affection loss on miss.
	return Social; // Return configured social definition.
} // End BuildSocialDefinition.

FMovementDefinition UGenerateVillagerAssetsCommandlet::BuildMovementDefinition() const // Movement definition builder.
{
	FMovementDefinition Movement; // Instantiate movement definition.
	Movement.WalkSpeed = 200.0f; // Assign walk speed.
	Movement.MaxAcceleration = 1024.0f; // Assign max acceleration.
	Movement.AcceptanceRadius = 75.0f; // Assign acceptance radius.
	return Movement; // Return configured movement definition.
} // End BuildMovementDefinition.

bool UGenerateVillagerAssetsCommandlet::SavePackageToDisk(UPackage* Package, UObject* Asset) // Package save helper.
{
	if (!Package || !Asset) // Validate inputs.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Invalid package or asset during save.")); // Log invalid input.
		return false; // Abort on failure.
	}

	const FString PackageName = Package->GetName(); // Get package name.
	const FString FilePath = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()); // Convert to file path.

	FSavePackageArgs SaveArgs; // Create save arguments.
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone; // Set top-level flags.
	SaveArgs.Error = GError; // Route errors to global output.

	return UPackage::SavePackage(Package, Asset, *FilePath, SaveArgs); // Save package to disk.
} // End SavePackageToDisk.

bool UGenerateVillagerAssetsCommandlet::BuildVillagers() // Villager build routine.
{
	const FGameplayTag HungerTag = SafeRequestTag(TEXT("Need.Hunger")); // Resolve hunger tag.
	const FGameplayTag ThirstTag = SafeRequestTag(TEXT("Need.Thirst")); // Resolve thirst tag.
	const FGameplayTag SleepTag = SafeRequestTag(TEXT("Need.Sleep")); // Resolve sleep tag.

	const FGameplayTag EatingTag = SafeRequestTag(TEXT("Activities.Eating")); // Resolve eating tag.
	const FGameplayTag DrinkingTag = SafeRequestTag(TEXT("Activities.Drinking")); // Resolve drinking tag.
	const FGameplayTag SleepingTag = SafeRequestTag(TEXT("Activities.Sleeping")); // Resolve sleeping tag.
	const FGameplayTag WorkingTag = SafeRequestTag(TEXT("Activities.Working")); // Resolve working tag.

	const FGameplayTag FoodResourceTag = SafeRequestTag(TEXT("Resources.Food")); // Resolve food resource tag.
	const FGameplayTag WaterResourceTag = SafeRequestTag(TEXT("Resources.Water")); // Resolve water resource tag.
	const FGameplayTag CottonResourceTag = SafeRequestTag(TEXT("Resources.Cotton")); // Resolve cotton resource tag.

	const FGameplayTag FoodId = SafeRequestTag(TEXT("VillagerID.FoodProvider")); // Resolve food provider id.
	const FGameplayTag WaterId = SafeRequestTag(TEXT("VillagerID.WaterProvider")); // Resolve water provider id.
	const FGameplayTag CottonId = SafeRequestTag(TEXT("VillagerID.CottonProvider")); // Resolve cotton provider id.

	UCurveFloat* ForceHunger = CurveMap.FindRef(TEXT("Force_Hunger")); // Fetch hunger force curve.
	UCurveFloat* ForceThirst = CurveMap.FindRef(TEXT("Force_Thirst")); // Fetch thirst force curve.
	UCurveFloat* ForceSleep = CurveMap.FindRef(TEXT("Force_Sleep")); // Fetch sleep force curve.
	UCurveFloat* WorkHunger = CurveMap.FindRef(TEXT("Work_Hunger")); // Fetch working hunger curve.
	UCurveFloat* WorkThirst = CurveMap.FindRef(TEXT("Work_Thirst")); // Fetch working thirst curve.
	UCurveFloat* WorkSleep = CurveMap.FindRef(TEXT("Work_Sleep")); // Fetch working sleep curve.
	UCurveFloat* EatHunger = CurveMap.FindRef(TEXT("Eat_Hunger")); // Fetch eating hunger curve.
	UCurveFloat* EatThirst = CurveMap.FindRef(TEXT("Eat_Thirst")); // Fetch eating thirst curve.
	UCurveFloat* EatSleep = CurveMap.FindRef(TEXT("Eat_Sleep")); // Fetch eating sleep curve.
	UCurveFloat* DrinkThirst = CurveMap.FindRef(TEXT("Drink_Thirst")); // Fetch drinking thirst curve.
	UCurveFloat* DrinkHunger = CurveMap.FindRef(TEXT("Drink_Hunger")); // Fetch drinking hunger curve.
	UCurveFloat* DrinkSleep = CurveMap.FindRef(TEXT("Drink_Sleep")); // Fetch drinking sleep curve.
	UCurveFloat* SleepCurve = CurveMap.FindRef(TEXT("Sleep_Sleep")); // Fetch sleeping curve.
	UCurveFloat* SleepHunger = CurveMap.FindRef(TEXT("Sleep_Hunger")); // Fetch sleeping hunger curve.
	UCurveFloat* SleepThirst = CurveMap.FindRef(TEXT("Sleep_Thirst")); // Fetch sleeping thirst curve.
	UCurveFloat* AffectionCurve = CurveMap.FindRef(TEXT("Affection_Quantity")); // Fetch affection curve.

	if (!ForceHunger || !ForceThirst || !ForceSleep || !WorkHunger || !WorkThirst || !WorkSleep || !EatHunger || !EatThirst || !EatSleep || !DrinkThirst || !DrinkHunger || !DrinkSleep || !SleepCurve || !SleepHunger || !SleepThirst || !AffectionCurve) // Validate curve availability.
	{
		UE_LOG(LogGenerateVillagerAssets, Error, TEXT("Missing required curves for villager generation.")); // Log missing curves.
		return false; // Abort on failure.
	}

	{ // Begin food provider scope.
		UVillagerArchetypeDataAsset* FoodAsset = CreateVillagerAsset(VillagerRoot + TEXT("/DA_FoodProviderVillager")); // Create food asset.
		if (!FoodAsset) // Validate asset creation.
		{
			return false; // Abort on failure.
		}

		FoodAsset->VillagerIdTag = FoodId; // Assign villager id.
		FoodAsset->NeedDefinitions = { // Assign need definitions.
			BuildNeedDefinition(HungerTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceHunger, EatingTag), // Hunger need definition (starts full and decays).
			BuildNeedDefinition(ThirstTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceThirst, DrinkingTag), // Thirst need definition (starts full and decays).
			BuildNeedDefinition(SleepTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceSleep, SleepingTag) // Sleep need definition (starts full and decays).
		}; // End need definitions.

		TMap<FGameplayTag, UCurveFloat*> WorkCurves; // Map for work need curves.
		WorkCurves.Add(HungerTag, WorkHunger); // Add hunger work curve.
		WorkCurves.Add(ThirstTag, WorkThirst); // Add thirst work curve.
		WorkCurves.Add(SleepTag, WorkSleep); // Add sleep work curve.
		TMap<FGameplayTag, UCurveFloat*> EatCurves; // Map for eating curves.
		EatCurves.Add(HungerTag, EatHunger); // Add hunger eating curve.
		EatCurves.Add(ThirstTag, EatThirst); // Add thirst decay during eating.
		EatCurves.Add(SleepTag, EatSleep); // Add sleep decay during eating.
		TMap<FGameplayTag, UCurveFloat*> DrinkCurves; // Map for drinking curves.
		DrinkCurves.Add(ThirstTag, DrinkThirst); // Add thirst drinking curve.
		DrinkCurves.Add(HungerTag, DrinkHunger); // Add hunger decay during drinking.
		DrinkCurves.Add(SleepTag, DrinkSleep); // Add sleep decay during drinking.
		TMap<FGameplayTag, UCurveFloat*> SleepCurves; // Map for sleeping curves.
		SleepCurves.Add(SleepTag, SleepCurve); // Add sleep sleeping curve.
		SleepCurves.Add(HungerTag, SleepHunger); // Add hunger decay during sleeping.
		SleepCurves.Add(ThirstTag, SleepThirst); // Add thirst decay during sleeping.

		const FGameplayTag BedTag = SafeRequestTag(TEXT("Locations.Bed_FoodProvider")); // Resolve bed tag.
		const FGameplayTag KitchenTag = SafeRequestTag(TEXT("Locations.Kitchen_FoodProvider")); // Resolve kitchen tag.
		const FGameplayTag WellTag = SafeRequestTag(TEXT("Locations.Well_FoodProvider")); // Resolve well tag.
		const FGameplayTag WorkTag = SafeRequestTag(TEXT("Locations.WorkingPlace_FoodProvider")); // Resolve work tag.

		FoodAsset->ActivityDefinitions = { // Assign activity definitions.
			BuildActivityDefinition(SleepingTag, 0, 0, 6, BedTag, CottonResourceTag, SleepCurves), // Sleeping activity.
			BuildActivityDefinition(EatingTag, 1, 6, 7, KitchenTag, FoodResourceTag, EatCurves), // Eating activity.
			BuildActivityDefinition(DrinkingTag, 2, 7, 8, WellTag, WaterResourceTag, DrinkCurves), // Drinking activity.
			BuildActivityDefinition(WorkingTag, 3, 8, 24, WorkTag, FGameplayTag(), WorkCurves) // Working activity.
		}; // End activity definitions.

		FoodAsset->SocialDefinition = BuildSocialDefinition(FoodResourceTag, AffectionCurve, { WaterId, CottonId }, WorkTag); // Assign social definition.
		FoodAsset->MovementDefinition = BuildMovementDefinition(); // Assign movement definition.
		FoodAsset->MarkPackageDirty(); // Mark package dirty.
		if (!SavePackageToDisk(FoodAsset->GetOutermost(), FoodAsset)) // Save asset package.
		{
			return false; // Abort on save failure.
		}
	} // End food provider scope.

	{ // Begin water provider scope.
		UVillagerArchetypeDataAsset* WaterAsset = CreateVillagerAsset(VillagerRoot + TEXT("/DA_WaterProviderVillager")); // Create water asset.
		if (!WaterAsset) // Validate asset creation.
		{
			return false; // Abort on failure.
		}

		WaterAsset->VillagerIdTag = WaterId; // Assign villager id.
		WaterAsset->NeedDefinitions = { // Assign need definitions.
			BuildNeedDefinition(HungerTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceHunger, EatingTag), // Hunger need definition (starts full and decays).
			BuildNeedDefinition(ThirstTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceThirst, DrinkingTag), // Thirst need definition (starts full and decays).
			BuildNeedDefinition(SleepTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceSleep, SleepingTag) // Sleep need definition (starts full and decays).
		}; // End need definitions.

		TMap<FGameplayTag, UCurveFloat*> WorkCurves; // Map for work need curves.
		WorkCurves.Add(HungerTag, WorkHunger); // Add hunger work curve.
		WorkCurves.Add(ThirstTag, WorkThirst); // Add thirst work curve.
		WorkCurves.Add(SleepTag, WorkSleep); // Add sleep work curve.
		TMap<FGameplayTag, UCurveFloat*> EatCurves; // Map for eating curves.
		EatCurves.Add(HungerTag, EatHunger); // Add hunger eating curve.
		EatCurves.Add(ThirstTag, EatThirst); // Add thirst decay during eating.
		EatCurves.Add(SleepTag, EatSleep); // Add sleep decay during eating.
		TMap<FGameplayTag, UCurveFloat*> DrinkCurves; // Map for drinking curves.
		DrinkCurves.Add(ThirstTag, DrinkThirst); // Add thirst drinking curve.
		DrinkCurves.Add(HungerTag, DrinkHunger); // Add hunger decay during drinking.
		DrinkCurves.Add(SleepTag, DrinkSleep); // Add sleep decay during drinking.
		TMap<FGameplayTag, UCurveFloat*> SleepCurves; // Map for sleeping curves.
		SleepCurves.Add(SleepTag, SleepCurve); // Add sleep sleeping curve.
		SleepCurves.Add(HungerTag, SleepHunger); // Add hunger decay during sleeping.
		SleepCurves.Add(ThirstTag, SleepThirst); // Add thirst decay during sleeping.

		const FGameplayTag BedTag = SafeRequestTag(TEXT("Locations.Bed_WaterProvider")); // Resolve bed tag.
		const FGameplayTag KitchenTag = SafeRequestTag(TEXT("Locations.Kitchen_WaterProvider")); // Resolve kitchen tag.
		const FGameplayTag WellTag = SafeRequestTag(TEXT("Locations.Well_WaterProvider")); // Resolve well tag.
		const FGameplayTag WorkTag = SafeRequestTag(TEXT("Locations.WorkingPlace_WaterProvider")); // Resolve work tag.

		WaterAsset->ActivityDefinitions = { // Assign activity definitions.
			BuildActivityDefinition(SleepingTag, 0, 0, 7, BedTag, CottonResourceTag, SleepCurves), // Sleeping activity.
			BuildActivityDefinition(EatingTag, 1, 7, 8, KitchenTag, FoodResourceTag, EatCurves), // Eating activity.
			BuildActivityDefinition(DrinkingTag, 2, 8, 9, WellTag, WaterResourceTag, DrinkCurves), // Drinking activity.
			BuildActivityDefinition(WorkingTag, 3, 9, 24, WorkTag, FGameplayTag(), WorkCurves) // Working activity.
		}; // End activity definitions.

		WaterAsset->SocialDefinition = BuildSocialDefinition(WaterResourceTag, AffectionCurve, { FoodId, CottonId }, WorkTag); // Assign social definition.
		WaterAsset->MovementDefinition = BuildMovementDefinition(); // Assign movement definition.
		WaterAsset->MarkPackageDirty(); // Mark package dirty.
		if (!SavePackageToDisk(WaterAsset->GetOutermost(), WaterAsset)) // Save asset package.
		{
			return false; // Abort on save failure.
		}
	} // End water provider scope.

	{ // Begin cotton provider scope.
		UVillagerArchetypeDataAsset* CottonAsset = CreateVillagerAsset(VillagerRoot + TEXT("/DA_CottonProviderVillager")); // Create cotton asset.
		if (!CottonAsset) // Validate asset creation.
		{
			return false; // Abort on failure.
		}

		CottonAsset->VillagerIdTag = CottonId; // Assign villager id.
		CottonAsset->NeedDefinitions = { // Assign need definitions.
			BuildNeedDefinition(HungerTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceHunger, EatingTag), // Hunger need definition (starts full and decays).
			BuildNeedDefinition(ThirstTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceThirst, DrinkingTag), // Thirst need definition (starts full and decays).
			BuildNeedDefinition(SleepTag, 1.0f, 0.0f, 1.0f, 0.6f, 0.3f, 1.0f, ForceSleep, SleepingTag) // Sleep need definition (starts full and decays).
		}; // End need definitions.

		TMap<FGameplayTag, UCurveFloat*> WorkCurves; // Map for work need curves.
		WorkCurves.Add(HungerTag, WorkHunger); // Add hunger work curve.
		WorkCurves.Add(ThirstTag, WorkThirst); // Add thirst work curve.
		WorkCurves.Add(SleepTag, WorkSleep); // Add sleep work curve.
		TMap<FGameplayTag, UCurveFloat*> EatCurves; // Map for eating curves.
		EatCurves.Add(HungerTag, EatHunger); // Add hunger eating curve.
		EatCurves.Add(ThirstTag, EatThirst); // Add thirst decay during eating.
		EatCurves.Add(SleepTag, EatSleep); // Add sleep decay during eating.
		TMap<FGameplayTag, UCurveFloat*> DrinkCurves; // Map for drinking curves.
		DrinkCurves.Add(ThirstTag, DrinkThirst); // Add thirst drinking curve.
		DrinkCurves.Add(HungerTag, DrinkHunger); // Add hunger decay during drinking.
		DrinkCurves.Add(SleepTag, DrinkSleep); // Add sleep decay during drinking.
		TMap<FGameplayTag, UCurveFloat*> SleepCurves; // Map for sleeping curves.
		SleepCurves.Add(SleepTag, SleepCurve); // Add sleep sleeping curve.
		SleepCurves.Add(HungerTag, SleepHunger); // Add hunger decay during sleeping.
		SleepCurves.Add(ThirstTag, SleepThirst); // Add thirst decay during sleeping.

		const FGameplayTag BedTag = SafeRequestTag(TEXT("Locations.Bed_CottonProvider")); // Resolve bed tag.
		const FGameplayTag KitchenTag = SafeRequestTag(TEXT("Locations.Kitchen_CottonProvider")); // Resolve kitchen tag.
		const FGameplayTag WellTag = SafeRequestTag(TEXT("Locations.Well_CottonProvider")); // Resolve well tag.
		const FGameplayTag WorkTag = SafeRequestTag(TEXT("Locations.WorkingPlace_CottonProvider")); // Resolve work tag.

		CottonAsset->ActivityDefinitions = { // Assign activity definitions.
			BuildActivityDefinition(SleepingTag, 0, 0, 8, BedTag, CottonResourceTag, SleepCurves), // Sleeping activity.
			BuildActivityDefinition(EatingTag, 1, 8, 9, KitchenTag, FoodResourceTag, EatCurves), // Eating activity.
			BuildActivityDefinition(DrinkingTag, 2, 9, 10, WellTag, WaterResourceTag, DrinkCurves), // Drinking activity.
			BuildActivityDefinition(WorkingTag, 3, 10, 24, WorkTag, FGameplayTag(), WorkCurves) // Working activity.
		}; // End activity definitions.

		CottonAsset->SocialDefinition = BuildSocialDefinition(CottonResourceTag, AffectionCurve, { FoodId, WaterId }, WorkTag); // Assign social definition.
		CottonAsset->MovementDefinition = BuildMovementDefinition(); // Assign movement definition.
		CottonAsset->MarkPackageDirty(); // Mark package dirty.
		if (!SavePackageToDisk(CottonAsset->GetOutermost(), CottonAsset)) // Save asset package.
		{
			return false; // Abort on save failure.
		}
	} // End cotton provider scope.

	return true; // Indicate villager build success.
} // End BuildVillagers.
#pragma endregion VillagerConstruction // End villager construction region.
