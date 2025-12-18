// Includes the fixed camera pawn declaration. 
#include "Simulation/Player/FixedCameraPawn.h" // Imports AFixedCameraPawn definitions. 

// Region: Engine includes. 
#pragma region EngineIncludes
// Provides camera component functionality. 
#include "Camera/CameraComponent.h" // Imports UCameraComponent. 
// Provides scene component definition for the root component. 
#include "Components/SceneComponent.h" // Imports USceneComponent. 
// Provides floating pawn movement implementation. 
#include "GameFramework/FloatingPawnMovement.h" // Imports UFloatingPawnMovement. 
// Provides player controller access for input subsystem retrieval. 
#include "GameFramework/PlayerController.h" // Imports APlayerController. 
// Provides gameplay statics helpers for player controller lookup. 
#include "Kismet/GameplayStatics.h" // Imports UGameplayStatics. 
// Provides access to Slate application for UI hit testing. 
#include "Framework/Application/SlateApplication.h" // Imports FSlateApplication. 
// Provides widget type information for hit testing. 
#include "Widgets/SWidget.h" // Imports SWidget. 
// Provides access to world and engine singletons.
#include "Engine/World.h" // Imports UWorld.
#include "Engine/Engine.h" // Imports GEngine.
// Provides local player definition for input subsystem resolution.
#include "Engine/LocalPlayer.h" // Imports ULocalPlayer.
#pragma endregion EngineIncludes

// Region: Enhanced input includes. 
#pragma region EnhancedInputIncludes
// Provides enhanced input component bindings. 
#include "EnhancedInputComponent.h" // Imports UEnhancedInputComponent. 
// Provides enhanced input subsystem access. 
#include "EnhancedInputSubsystems.h" // Imports UEnhancedInputLocalPlayerSubsystem. 
// Provides input action asset definitions. 
#include "InputAction.h" // Imports UInputAction. 
// Provides mapping context definitions. 
#include "InputMappingContext.h" // Imports UInputMappingContext. 
// Provides input modifier types for axis swizzling and negation. 
#include "InputModifiers.h" // Imports UInputModifier classes. 
// Provides enhanced input action value type. 
#include "InputActionValue.h" // Imports FInputActionValue. 
#pragma endregion EnhancedInputIncludes

// Region: Input core includes. 
#pragma region InputCoreIncludes
// Provides input key definitions for default bindings. 
#include "InputCoreTypes.h" // Imports FKey and EKeys. 
#pragma endregion InputCoreIncludes

// Region: Simulation includes. 
#pragma region SimulationIncludes
// Provides access to needs component for selection checks. 
#include "Simulation/Needs/VillagerNeedsComponent.h" // Imports UVillagerNeedsComponent. 
// Provides access to the needs display component for toggling UI. 
#include "Simulation/UI/VillagerNeedsDisplayComponent.h" // Imports UVillagerNeedsDisplayComponent. 
#pragma endregion SimulationIncludes

// Region: Lifecycle. 
#pragma region Lifecycle
// Constructs default components and configures camera behavior. 
AFixedCameraPawn::AFixedCameraPawn()
{
	PrimaryActorTick.bCanEverTick = false; // Disable ticking for performance. 
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene")); // Create the root scene component. 
	RootComponent = RootScene; // Assign the root component for movement updates. 
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent")); // Create the camera component. 
	CameraComponent->SetupAttachment(RootScene); // Attach the camera to the root. 
	CameraComponent->bUsePawnControlRotation = true; // Drive the camera from controller rotation. 
	CameraComponent->SetAbsolute(false, false, false); // Ensure the camera follows the pawn transform. 
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent")); // Create the movement component. 
	MovementComponent->UpdatedComponent = RootScene; // Apply movement to the root component. 
	MovementComponent->bConstrainToPlane = false; // Allow full 3D movement. 
	MovementComponent->SetPlaneConstraintAxisSetting(EPlaneConstraintAxisSetting::Z); // Keep axis setting in sync if re-enabled. 
	MovementComponent->bSnapToPlaneAtStart = false; // Avoid snapping to the world origin plane before the spawn transform is known. 
	MovementComponent->MaxSpeed = 600.0f; // Configure default movement speed. 
	MovementComponent->Acceleration = MovementComponent->MaxSpeed * 1000.0f; // Eliminate acceleration ramp for constant speed. 
	MovementComponent->Deceleration = MovementComponent->MaxSpeed * 1000.0f; // Eliminate deceleration ramp for constant speed. 
	MovementComponent->TurningBoost = 0.0f; // Disable turning boost to keep velocity steady. 
	AutoPossessPlayer = EAutoReceiveInput::Disabled; // Allow GameMode to spawn at PlayerStart locations. 
	bUseControllerRotationYaw = true; // Rotate the pawn with controller yaw. 
	bUseControllerRotationPitch = false; // Prevent pawn pitch rotation to keep upright. 
	bUseControllerRotationRoll = false; // Prevent pawn roll rotation for stability. 
	bFindCameraComponentWhenViewTarget = true; // Ensure the camera component is used for the view target. 
	ApplyCameraHeight(); // Apply initial camera height. 
}

// Applies input mapping contexts once the pawn is possessed. 
void AFixedCameraPawn::BeginPlay()
{
	Super::BeginPlay(); // Call the base implementation. 

	if (MovementComponent && MovementComponent->bConstrainToPlane) // Align the constraint plane with the spawn transform.
	{
		MovementComponent->SetPlaneConstraintOrigin(GetActorLocation()); // Prevent plane snapping to world origin.
	}

	ApplyCameraHeight(); // Re-apply camera height for any overridden defaults. 
	EnsureInputActions(); // Guarantee input actions exist for binding. 
	ApplyInputMappingContexts(); // Register mapping contexts and fallback bindings. 
	ApplyInputMode(false); // Ensure Game & UI input mode with unlocked cursor by default. 

	if (CameraComponent) // Ensure the camera component is active. 
	{
		CameraComponent->Activate(); // Activate the camera component for view targeting. 
	}

	if (HasAuthority()) // Only the server can assign possession. 
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0); // Resolve the primary player controller. 
		if (PlayerController && PlayerController->GetPawn() != this) // Possess this pawn if another pawn is active. 
		{
			PlayerController->Possess(this); // Force possession to align camera view with this pawn. 
		}
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) // Resolve the local player controller. 
	{
		PlayerController->SetViewTarget(this); // Force the view target to this pawn. 
		PlayerController->SetControlRotation(GetActorRotation()); // Align control rotation with the spawn transform. 
	}
}
#pragma endregion Lifecycle

// Region: Input. 
#pragma region Input
// Binds enhanced input actions for movement and look. 
void AFixedCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent); // Call the base input setup. 
	EnsureInputActions(); // Ensure input actions exist before binding. 
	ApplyInputMappingContexts(); // Ensure mapping contexts are applied for this player. 
	ApplyInputMode(false); // Ensure Game & UI input mode with unlocked cursor. 

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent); // Cast to enhanced input component. 
	if (!EnhancedInput) // Validate enhanced input component availability. 
	{
		return; // Exit if enhanced input is unavailable. 
	}

	if (MoveAction) // Verify the movement input action is assigned. 
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFixedCameraPawn::HandleMove); // Bind move input updates. 
	}

	if (LookAction) // Verify the look input action is assigned. 
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFixedCameraPawn::HandleLook); // Bind look input updates. 
	}

	if (LookHoldAction) // Verify the look-hold input action is assigned. 
	{
		EnhancedInput->BindAction(LookHoldAction, ETriggerEvent::Started, this, &AFixedCameraPawn::HandleLookHoldStarted); // Bind look-hold start handling. 
		EnhancedInput->BindAction(LookHoldAction, ETriggerEvent::Completed, this, &AFixedCameraPawn::HandleLookHoldCompleted); // Bind look-hold release handling. 
		EnhancedInput->BindAction(LookHoldAction, ETriggerEvent::Canceled, this, &AFixedCameraPawn::HandleLookHoldCompleted); // Treat canceled holds as releases. 
	}

	if (SelectAction) // Verify the selection input action is assigned. 
	{
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Started, this, &AFixedCameraPawn::HandleSelectStarted); // Bind selection press handling. 
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Completed, this, &AFixedCameraPawn::HandleSelectCompleted); // Bind selection release handling. 
		EnhancedInput->BindAction(SelectAction, ETriggerEvent::Canceled, this, &AFixedCameraPawn::HandleSelectCompleted); // Treat canceled selects as releases. 
	}
}

// Supplies camera data for the player camera manager. 
void AFixedCameraPawn::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (CameraComponent && CameraComponent->IsActive()) // Prefer the explicit camera component when active. 
	{
		CameraComponent->GetCameraView(DeltaTime, OutResult); // Populate the view from the camera component. 
		return; // Skip the default pawn camera calculation. 
	}

	Super::CalcCamera(DeltaTime, OutResult); // Fall back to the base camera calculation. 
}

// Ensures the camera view target is set when the pawn is possessed. 
void AFixedCameraPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController); // Preserve base possession behavior. 
	ApplyInputMappingContexts(); // Ensure input mappings are applied after possession. 
	ApplyInputMode(false); // Ensure Game & UI input mode with unlocked cursor. 

	APlayerController* PlayerController = Cast<APlayerController>(NewController); // Resolve the player controller. 
	if (PlayerController) // Validate the controller type. 
	{
		PlayerController->SetViewTarget(this); // Force the view target to this pawn. 
		PlayerController->SetControlRotation(GetActorRotation()); // Keep control rotation aligned with the pawn. 
	}
}

// Ensures the camera view target is set on controller replication. 
void AFixedCameraPawn::OnRep_Controller()
{
	Super::OnRep_Controller(); // Preserve base replication behavior. 
	ApplyInputMappingContexts(); // Ensure input mappings are applied after replication. 
	ApplyInputMode(false); // Ensure Game & UI input mode with unlocked cursor. 

	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // Resolve the player controller. 
	if (PlayerController) // Validate the controller type. 
	{
		PlayerController->SetViewTarget(this); // Force the view target to this pawn. 
		PlayerController->SetControlRotation(GetActorRotation()); // Keep control rotation aligned with the pawn. 
	}
}

// Handles planar WASD movement input. 
void AFixedCameraPawn::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveValue = Value.Get<FVector2D>(); // Extract the 2D movement input. 
	if (!Controller) // Validate controller availability for rotation context. 
	{
		return; // Exit if no controller is assigned. 
	}

	FVector Forward = FVector::ForwardVector; // Default forward vector. 
	FVector Right = FVector::RightVector; // Default right vector. 
	if (CameraComponent) // Use the camera orientation for true 3D movement. 
	{
		Forward = CameraComponent->GetForwardVector(); // Follow camera forward including vertical component. 
		Right = CameraComponent->GetRightVector(); // Follow camera right based on current view. 
	}
	else
	{
		const FRotator ControlRotation = Controller->GetControlRotation(); // Get the controller rotation. 
		const FRotationMatrix RotationMatrix(ControlRotation); // Build full rotation (pitch + yaw). 
		Forward = RotationMatrix.GetUnitAxis(EAxis::X); // Calculate forward from full rotation. 
		Right = RotationMatrix.GetUnitAxis(EAxis::Y); // Calculate right from full rotation. 
	}

	AddMovementInput(Forward, MoveValue.Y); // Apply forward/backward movement. 
	AddMovementInput(Right, MoveValue.X); // Apply right/left movement. 
}

// Handles mouse look input for yaw and pitch. 
void AFixedCameraPawn::HandleLook(const FInputActionValue& Value)
{
	if (!bIsLookInputHeld) // Ignore look input unless the hold action is active. 
	{
		return; // Exit when look-hold is not engaged. 
	}

	const FVector2D LookValue = Value.Get<FVector2D>(); // Extract the 2D look input. 
	const float YawMultiplier = bInvertYaw ? -1.0f : 1.0f; // Apply optional yaw inversion. 
	const float PitchMultiplier = bInvertPitch ? -1.0f : 1.0f; // Apply optional pitch inversion. 
	AddControllerYawInput(LookValue.X * LookSensitivity * YawMultiplier); // Apply yaw input with sensitivity scaling. 
	AddControllerPitchInput(LookValue.Y * LookSensitivity * PitchMultiplier); // Apply pitch input with sensitivity scaling. 
}

// Handles look-hold activation for cursor locking and camera rotation. 
void AFixedCameraPawn::HandleLookHoldStarted(const FInputActionValue& Value)
{
	(void)Value; // Explicitly ignore unused input value. 
	bIsLookInputHeld = true; // Enable camera rotation while held. 
	ApplyInputMode(true); // Lock the cursor while look input is held. 
}

// Handles look-hold release to unlock cursor after camera rotation. 
void AFixedCameraPawn::HandleLookHoldCompleted(const FInputActionValue& Value)
{
	(void)Value; // Explicitly ignore unused input value. 
	bIsLookInputHeld = false; // Disable camera rotation after release. 
	ApplyInputMode(false); // Restore unlocked cursor for UI interaction. 
}

// Handles selection press input for villager interaction. 
void AFixedCameraPawn::HandleSelectStarted(const FInputActionValue& Value)
{
	(void)Value; // Explicitly ignore unused input value. 

	if (IsCursorOverUI()) // Skip selection when the cursor is over UI. 
	{
		bSuppressSelection = true; // Prevent selection while UI is hovered. 
		bIsSelectPressed = false; // Clear selection press state. 
		return; // Exit without tracking the selection press. 
	}

	bSuppressSelection = false; // Allow selection checks on release. 
	bIsSelectPressed = true; // Track selection press state. 
	SelectPressStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f; // Cache the press start time. 

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController())) // Resolve the player controller. 
	{
		float CursorX = 0.0f; // Allocate cursor X storage. 
		float CursorY = 0.0f; // Allocate cursor Y storage. 
		if (PlayerController->GetMousePosition(CursorX, CursorY)) // Query cursor position when available. 
		{
			SelectPressStartCursorPosition = FVector2D(CursorX, CursorY); // Cache the cursor position for click detection. 
		}
		else // Reset cursor position when it cannot be queried. 
		{
			SelectPressStartCursorPosition = FVector2D::ZeroVector; // Default to zero to avoid stale values. 
		}
	}
}

// Handles selection release input for villager interaction. 
void AFixedCameraPawn::HandleSelectCompleted(const FInputActionValue& Value)
{
	(void)Value; // Explicitly ignore unused input value. 

	if (bSuppressSelection) // Skip selection when the press originated over UI. 
	{
		bSuppressSelection = false; // Reset suppression for the next click. 
		bIsSelectPressed = false; // Clear selection press state. 
		return; // Exit without selecting a villager. 
	}

	if (bIsSelectPressed && IsSelectClick()) // Check whether the release qualifies as a selection click. 
	{
		TrySelectVillager(); // Attempt to toggle the villager needs widget. 
	}

	bIsSelectPressed = false; // Clear selection press state. 
}

// Ensures input actions exist for binding and fallback mapping. 
void AFixedCameraPawn::EnsureInputActions()
{
	if (!MoveAction) // Create a default move action when none is assigned. 
	{
		MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move_Fallback")); // Allocate a transient move action. 
		MoveAction->ValueType = EInputActionValueType::Axis2D; // Configure the action for 2D movement input. 
	}

	if (!LookAction) // Create a default look action when none is assigned. 
	{
		LookAction = NewObject<UInputAction>(this, TEXT("IA_Look_Fallback")); // Allocate a transient look action. 
		LookAction->ValueType = EInputActionValueType::Axis2D; // Configure the action for 2D look input. 
	}

	if (!LookHoldAction) // Create a default look-hold action when none is assigned. 
	{
		LookHoldAction = NewObject<UInputAction>(this, TEXT("IA_LookHold_Fallback")); // Allocate a transient look-hold action. 
		LookHoldAction->ValueType = EInputActionValueType::Boolean; // Configure the action for button input. 
	}

	if (!SelectAction) // Create a default selection action when none is assigned. 
	{
		SelectAction = NewObject<UInputAction>(this, TEXT("IA_Select_Fallback")); // Allocate a transient selection action. 
		SelectAction->ValueType = EInputActionValueType::Boolean; // Configure the action for button input. 
	}
}

// Applies mapping contexts and runtime fallback bindings when needed. 
void AFixedCameraPawn::ApplyInputMappingContexts()
{
	if (bInputMappingApplied) // Skip if mapping contexts were already applied. 
	{
		return; // Avoid duplicate mapping context registration. 
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // Resolve the owning player controller. 
	if (!PlayerController || !PlayerController->IsLocalController()) // Ensure a valid local controller. 
	{
		return; // Exit if no local controller is available. 
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer(); // Resolve the local player instance. 
	if (!LocalPlayer) // Validate local player availability. 
	{
		return; // Exit if the local player is missing. 
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(); // Resolve the enhanced input subsystem. 
	if (!InputSubsystem) // Validate subsystem availability. 
	{
		return; // Exit if the subsystem cannot be resolved. 
	}

	bool bNeedsMoveMappings = true; // Assume move mappings are required until verified. 
	bool bNeedsLookMappings = true; // Assume look mappings are required until verified. 
	bool bNeedsHoldMappings = true; // Assume look-hold mappings are required until verified. 
	bool bNeedsSelectMappings = true; // Assume selection mappings are required until verified. 

	if (InputMappingContext) // Add the primary mapping context when available. 
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0); // Register the primary context at default priority. 
		bNeedsMoveMappings = !MappingContextHasAction(InputMappingContext, MoveAction); // Check for move bindings in the primary context. 
		bNeedsLookMappings = !MappingContextHasAction(InputMappingContext, LookAction); // Check for look bindings in the primary context. 
		bNeedsHoldMappings = !MappingContextHasActionKey(InputMappingContext, LookHoldAction, EKeys::RightMouseButton); // Check for RMB look-hold binding in the primary context. 
		bNeedsSelectMappings = !MappingContextHasAction(InputMappingContext, SelectAction); // Check for selection bindings in the primary context. 
	}

	if (!InputMappingContext || bNeedsMoveMappings || bNeedsLookMappings || bNeedsHoldMappings || bNeedsSelectMappings) // Build fallback mappings when required. 
	{
		if (!FallbackMappingContext) // Allocate the fallback context when missing. 
		{
			FallbackMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Fallback")); // Create a transient fallback context. 
		}

		BuildFallbackMappings(FallbackMappingContext, bNeedsMoveMappings, bNeedsLookMappings, bNeedsHoldMappings, bNeedsSelectMappings); // Populate fallback mappings for missing actions. 

		if (!bFallbackMappingApplied) // Apply fallback mapping context once. 
		{
			InputSubsystem->AddMappingContext(FallbackMappingContext, -1); // Register fallback context at lower priority. 
			bFallbackMappingApplied = true; // Mark fallback mappings as applied. 
		}
	}

	bInputMappingApplied = true; // Mark mapping contexts as applied. 
}

// Checks whether a mapping context already contains an action mapping. 
bool AFixedCameraPawn::MappingContextHasAction(const UInputMappingContext* MappingContext, const UInputAction* Action) const
{
	if (!MappingContext || !Action) // Validate input parameters. 
	{
		return false; // Report missing mappings when parameters are invalid. 
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = MappingContext->GetMappings(); // Access the context mappings. 
	for (const FEnhancedActionKeyMapping& Mapping : Mappings) // Scan all mappings for the action. 
	{
		if (Mapping.Action == Action) // Compare the action pointer. 
		{
			return true; // Report that a matching mapping exists. 
		}
	}

	return false; // Report missing action mapping. 
}

// Checks whether a mapping context already contains a specific action/key pair. 
bool AFixedCameraPawn::MappingContextHasActionKey(const UInputMappingContext* MappingContext, const UInputAction* Action, const FKey& Key) const
{
	if (!MappingContext || !Action || !Key.IsValid()) // Validate input parameters. 
	{
		return false; // Report missing mappings when parameters are invalid. 
	}

	const TArray<FEnhancedActionKeyMapping>& Mappings = MappingContext->GetMappings(); // Access the context mappings. 
	for (const FEnhancedActionKeyMapping& Mapping : Mappings) // Scan all mappings for the action/key pair. 
	{
		if (Mapping.Action == Action && Mapping.Key == Key) // Compare the action pointer and key. 
		{
			return true; // Report that a matching mapping exists. 
		}
	}

	return false; // Report missing action/key mapping. 
}

// Builds fallback mappings for missing actions inside the provided context. 
void AFixedCameraPawn::BuildFallbackMappings(UInputMappingContext* MappingContext, bool bNeedMoveMappings, bool bNeedLookMappings, bool bNeedHoldMappings, bool bNeedSelectMappings)
{
	if (!MappingContext) // Validate the fallback context. 
	{
		return; // Exit when no mapping context is provided. 
	}

	if (bFallbackMappingsBuilt) // Skip rebuilding fallback mappings. 
	{
		return; // Exit if fallback mappings were already constructed. 
	}

	if (bNeedMoveMappings && MoveAction) // Add default movement mappings when needed. 
	{
		UInputModifierSwizzleAxis* SwizzleToY = NewObject<UInputModifierSwizzleAxis>(MappingContext); // Allocate a swizzle modifier for Y axis. 
		SwizzleToY->Order = EInputAxisSwizzle::YXZ; // Remap X input into the Y axis. 
		TArray<UInputModifier*> WModifiers; // Allocate modifiers for forward movement. 
		WModifiers.Add(SwizzleToY); // Append the Y swizzle modifier. 
		AddMappingWithModifiers(MappingContext, MoveAction, EKeys::W, WModifiers); // Map W to forward movement. 

		UInputModifierSwizzleAxis* SwizzleToYNeg = NewObject<UInputModifierSwizzleAxis>(MappingContext); // Allocate a swizzle modifier for Y axis. 
		SwizzleToYNeg->Order = EInputAxisSwizzle::YXZ; // Remap X input into the Y axis. 
		UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(MappingContext); // Allocate a negate modifier for backward movement. 
		TArray<UInputModifier*> SModifiers; // Allocate modifiers for backward movement. 
		SModifiers.Add(SwizzleToYNeg); // Append the Y swizzle modifier. 
		SModifiers.Add(NegateY); // Append the negate modifier. 
		AddMappingWithModifiers(MappingContext, MoveAction, EKeys::S, SModifiers); // Map S to backward movement. 

		UInputModifierNegate* NegateX = NewObject<UInputModifierNegate>(MappingContext); // Allocate a negate modifier for left movement. 
		TArray<UInputModifier*> AModifiers; // Allocate modifiers for left movement. 
		AModifiers.Add(NegateX); // Append the negate modifier. 
		AddMappingWithModifiers(MappingContext, MoveAction, EKeys::A, AModifiers); // Map A to left movement. 

		TArray<UInputModifier*> DModifiers; // Allocate empty modifiers for right movement. 
		AddMappingWithModifiers(MappingContext, MoveAction, EKeys::D, DModifiers); // Map D to right movement. 
	}

	if (bNeedLookMappings && LookAction) // Add default look mappings when needed. 
	{
		TArray<UInputModifier*> MouseXModifiers; // Allocate empty modifiers for mouse X. 
		AddMappingWithModifiers(MappingContext, LookAction, EKeys::MouseX, MouseXModifiers); // Map mouse X to yaw input. 

		UInputModifierSwizzleAxis* SwizzleToY = NewObject<UInputModifierSwizzleAxis>(MappingContext); // Allocate a swizzle modifier for Y axis. 
		SwizzleToY->Order = EInputAxisSwizzle::YXZ; // Remap X input into the Y axis. 
		TArray<UInputModifier*> MouseYModifiers; // Allocate modifiers for mouse Y. 
		MouseYModifiers.Add(SwizzleToY); // Append the Y swizzle modifier. 
		AddMappingWithModifiers(MappingContext, LookAction, EKeys::MouseY, MouseYModifiers); // Map mouse Y to pitch input. 
	}

	if (bNeedHoldMappings && LookHoldAction) // Add default look-hold mapping when needed. 
	{
		TArray<UInputModifier*> HoldModifiers; // Allocate empty modifiers for hold input. 
		AddMappingWithModifiers(MappingContext, LookHoldAction, EKeys::RightMouseButton, HoldModifiers); // Map RMB to hold-to-look. 
	}

	if (bNeedSelectMappings && SelectAction) // Add default selection mapping when needed. 
	{
		TArray<UInputModifier*> SelectModifiers; // Allocate empty modifiers for selection input. 
		AddMappingWithModifiers(MappingContext, SelectAction, EKeys::LeftMouseButton, SelectModifiers); // Map LMB to selection. 
	}

	bFallbackMappingsBuilt = true; // Mark fallback mappings as constructed. 
}

// Adds a mapping entry with optional modifiers. 
void AFixedCameraPawn::AddMappingWithModifiers(UInputMappingContext* MappingContext, UInputAction* Action, const FKey& Key, const TArray<UInputModifier*>& Modifiers)
{
	if (!MappingContext || !Action) // Validate mapping context and action. 
	{
		return; // Exit when required mapping inputs are missing. 
	}

	FEnhancedActionKeyMapping& NewMapping = MappingContext->MapKey(Action, Key); // Create a new mapping for the key. 
	for (UInputModifier* Modifier : Modifiers) // Append each requested modifier. 
	{
		if (Modifier) // Validate modifier instance. 
		{
			NewMapping.Modifiers.Add(Modifier); // Apply the modifier to the mapping. 
		}
	}
}

// Applies Game & UI input mode with optional cursor locking. 
void AFixedCameraPawn::ApplyInputMode(bool bLockMouse) const
{
	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // Resolve the owning player controller. 
	if (!PlayerController) // Validate controller availability. 
	{
		return; // Exit if no controller is assigned. 
	}

	if (bLockMouse) // Switch to game-only input when rotating the camera. 
	{
		FInputModeGameOnly InputMode; // Configure game-only input mode for camera rotation. 
		InputMode.SetConsumeCaptureMouseDown(false); // Preserve the initial mouse down input event. 
		PlayerController->SetInputMode(InputMode); // Apply the game-only input mode. 
		PlayerController->bShowMouseCursor = false; // Hide the cursor while rotating. 
		PlayerController->bEnableClickEvents = false; // Disable click events during camera rotation. 
		PlayerController->bEnableMouseOverEvents = false; // Disable hover events during camera rotation. 
		return; // Skip Game & UI configuration while locked. 
	}

	FInputModeGameAndUI InputMode; // Configure Game & UI input mode. 
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); // Keep the cursor unlocked for UI interaction. 
	InputMode.SetHideCursorDuringCapture(false); // Keep the cursor visible when interacting with UI. 
	PlayerController->SetInputMode(InputMode); // Apply the input mode to the controller. 
	PlayerController->bShowMouseCursor = true; // Ensure the cursor is visible for UI interaction. 
	PlayerController->bEnableClickEvents = true; // Enable click events for UI interaction. 
	PlayerController->bEnableMouseOverEvents = true; // Enable hover events for UI interaction. 
}

// Checks whether the cursor is currently over a UI widget. 
bool AFixedCameraPawn::IsCursorOverUI() const
{
	if (!FSlateApplication::IsInitialized()) // Ensure Slate is available for hit testing. 
	{
		return false; // Report no UI hover when Slate is unavailable. 
	}

	const FVector2D CursorPosition = FSlateApplication::Get().GetCursorPos(); // Read the current cursor position. 
	const TArray<TSharedRef<SWindow>>& Windows = FSlateApplication::Get().GetInteractiveTopLevelWindows(); // Resolve top-level windows. 
	FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(CursorPosition, Windows, false); // Build a widget path under the cursor. 

	if (!WidgetPath.IsValid()) // Validate the widget path. 
	{
		return false; // Report no UI hover when the path is invalid. 
	}

	for (int32 Index = WidgetPath.Widgets.Num() - 1; Index >= 0; --Index) // Walk from the deepest widget upward. 
	{
		const FArrangedWidget& ArrangedWidget = WidgetPath.Widgets[Index]; // Access the arranged widget entry. 
		const TSharedRef<SWidget>& Widget = ArrangedWidget.Widget; // Extract the widget reference. 
		if (!Widget->GetVisibility().IsHitTestVisible()) // Skip non-hit-testable widgets. 
		{
			continue; // Continue scanning for hit-testable widgets. 
		}

		if (!Widget->IsInteractable()) // Skip non-interactive widgets to allow camera look. 
		{
			continue; // Continue scanning for UI widgets. 
		}

		return true; // Report UI hover when a hit-testable UI widget is found. 
	}

	return false; // Report no UI hover when only the viewport is under the cursor. 
}

// Attempts to select a villager under the cursor and toggle the needs widget. 
void AFixedCameraPawn::TrySelectVillager()
{
	if (IsCursorOverUI()) // Avoid selection when the cursor is interacting with UI. 
	{
		return; // Exit when UI has cursor focus. 
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // Resolve the owning player controller. 
	if (!PlayerController) // Validate controller availability. 
	{
		return; // Exit if no controller is assigned. 
	}

	FHitResult HitResult; // Allocate hit result storage for cursor tracing. 
	if (!PlayerController->GetHitResultUnderCursor(ECC_Visibility, true, HitResult)) // Trace the world under the cursor. 
	{
		return; // Exit if nothing was hit. 
	}

	AActor* HitActor = HitResult.GetActor(); // Resolve the hit actor. 
	if (!HitActor) // Validate the hit actor. 
	{
		return; // Exit if no actor was hit. 
	}

	if (!HitActor->FindComponentByClass<UVillagerNeedsComponent>()) // Ensure the actor exposes needs data. 
	{
		return; // Exit if the actor is not a villager with needs. 
	}

	UVillagerNeedsDisplayComponent* DisplayComponent = HitActor->FindComponentByClass<UVillagerNeedsDisplayComponent>(); // Resolve the needs display component. 
	if (!DisplayComponent) // Create a display component on demand if missing. 
	{
		DisplayComponent = NewObject<UVillagerNeedsDisplayComponent>(HitActor, TEXT("NeedsDisplayComponent")); // Allocate the display component. 
		DisplayComponent->RegisterComponent(); // Register the component with the owning actor. 
	}

	if (SelectedNeedsDisplay.IsValid()) // Hide any previously selected villager widget. 
	{
		SelectedNeedsDisplay->SetWidgetVisible(false); // Hide the previous needs widget. 
	}

	DisplayComponent->InitializeWidgetComponent(); // Ensure widget component exists.
	DisplayComponent->SetWidgetVisible(true); // Show the newly selected villager widget. 
	DisplayComponent->RefreshWidgetData(); // Ensure data is up to date.
	SelectedVillagerActor = HitActor; // Cache the currently selected actor. 
	SelectedNeedsDisplay = DisplayComponent; // Cache the display component for later toggles. 

	// Emit console log to confirm selection and widget visibility.
	if (GEngine)
	{
		const FString ActorName = HitActor->GetName();
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.0f, FColor::Green, FString::Printf(TEXT("Selected villager: %s (widget shown)"), *ActorName));
	}
}

// Determines whether the current mouse hold qualifies as a selection click. 
bool AFixedCameraPawn::IsSelectClick() const
{
	const UWorld* World = GetWorld(); // Resolve the world context for timing. 
	if (!World) // Validate world availability. 
	{
		return false; // Report no click selection when world is missing. 
	}

	const float HoldDuration = World->GetTimeSeconds() - SelectPressStartTime; // Compute how long the press lasted. 
	if (HoldDuration > ClickSelectMaxHoldSeconds) // Reject long holds meant for drag input. 
	{
		return false; // Report that the hold is not a click. 
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController()); // Resolve the owning player controller. 
	if (!PlayerController) // Validate controller availability. 
	{
		return false; // Report no click selection when controller is missing. 
	}

	float CursorX = 0.0f; // Allocate cursor X storage. 
	float CursorY = 0.0f; // Allocate cursor Y storage. 
	if (!PlayerController->GetMousePosition(CursorX, CursorY)) // Query current cursor position. 
	{
		return false; // Report no click selection when cursor position is unavailable. 
	}

	const FVector2D CurrentCursorPosition(CursorX, CursorY); // Build the current cursor position vector. 
	const float PixelDelta = FVector2D::Distance(CurrentCursorPosition, SelectPressStartCursorPosition); // Measure cursor movement during press. 
	return PixelDelta <= ClickSelectMaxPixelDelta; // Treat small movement holds as selection clicks. 
}
#pragma endregion Input

// Region: Helpers. 
#pragma region Helpers
// Syncs the camera height to the configured value. 
void AFixedCameraPawn::ApplyCameraHeight()
{
	if (!CameraComponent) // Validate camera component availability. 
	{
		return; // Exit if the camera component is missing. 
	}

	CameraComponent->SetAbsolute(false, false, false); // Force relative transforms to keep the camera attached to the pawn. 
	CameraComponent->SetRelativeLocation(FVector(0.0f, 0.0f, CameraHeight)); // Set camera height offset. 
}
#pragma endregion Helpers
