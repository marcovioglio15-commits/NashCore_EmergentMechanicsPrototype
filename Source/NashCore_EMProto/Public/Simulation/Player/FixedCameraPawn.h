// Prevents multiple inclusion of the fixed camera pawn header. 
#pragma once // Ensures the header is included only once. 

// Region: Includes. 
#pragma region Includes
// Provides engine core types and macros. 
#include "CoreMinimal.h" // Imports CoreMinimal definitions. 
// Supplies the base pawn class. 
#include "GameFramework/Pawn.h" // Imports APawn declarations. 
#pragma endregion Includes // Ends the includes region. 

// Region: Forward declarations. 
#pragma region ForwardDeclarations
// Forward-declared camera component type. 
class UCameraComponent; 
// Forward-declared scene component type. 
class USceneComponent; 
// Forward-declared pawn movement component type. 
class UFloatingPawnMovement; 
// Forward-declared enhanced input action type. 
class UInputAction; 
// Forward-declared enhanced input mapping context type. 
class UInputMappingContext; 
// Forward-declared needs display component type. 
class UVillagerNeedsDisplayComponent; 
// Forward-declared input modifier base class. 
class UInputModifier; 
// Forward-declared input action value struct. 
struct FInputActionValue; 
// Forward-declared key struct used for input mapping. 
struct FKey; 
// Forward-declared camera view info struct. 
struct FMinimalViewInfo; 
#pragma endregion ForwardDeclarations // Ends the forward declarations region. 

// Generated header required by Unreal reflection. 
#include "FixedCameraPawn.generated.h" // Includes generated reflection data. 

// Region: Fixed camera pawn declaration. 
#pragma region FixedCameraPawnDeclaration
// Pawn that acts as a fixed first-person camera with planar movement and mouse look. 
UCLASS() // Marks the class for reflection. 
class AFixedCameraPawn : public APawn // Declares the pawn class type. 
{
	GENERATED_BODY() // Enables reflection and constructors. 

public:
#pragma region PublicInterface
	// Constructs default components and configures camera behavior. 
	AFixedCameraPawn();
#pragma endregion PublicInterface

protected:
#pragma region ProtectedInterface
	// Applies input mapping contexts once the pawn is possessed. 
	virtual void BeginPlay() override;

	// Binds enhanced input actions for movement and look. 
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// Supplies camera data for the player camera manager. 
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult) override;

	// Ensures the camera view target is set when the pawn is possessed. 
	virtual void PossessedBy(AController* NewController) override;

	// Ensures the camera view target is set on controller replication. 
	virtual void OnRep_Controller() override;
#pragma endregion ProtectedInterface

private:
#pragma region PrivateMethods
	// Handles planar WASD movement input. 
	void HandleMove(const FInputActionValue& Value);

	// Handles mouse look input for yaw and pitch. 
	void HandleLook(const FInputActionValue& Value);

	// Handles look-hold activation for cursor locking and camera rotation. 
	void HandleLookHoldStarted(const FInputActionValue& Value);

	// Handles look-hold release to unlock cursor after camera rotation. 
	void HandleLookHoldCompleted(const FInputActionValue& Value);

	// Handles selection press input for villager interaction. 
	void HandleSelectStarted(const FInputActionValue& Value);

	// Handles selection release input for villager interaction. 
	void HandleSelectCompleted(const FInputActionValue& Value);

	// Syncs the camera height to the configured value. 
	void ApplyCameraHeight();

	// Ensures input actions exist for binding and fallback mapping. 
	void EnsureInputActions(); // Allocates default input actions when not configured.

	// Applies mapping contexts and runtime fallback bindings when needed. 
	void ApplyInputMappingContexts(); // Adds mapping contexts to the local input subsystem.

	// Checks whether a mapping context already contains an action mapping. 
	bool MappingContextHasAction(const UInputMappingContext* MappingContext, const UInputAction* Action) const; // Tests for existing action mappings.

	// Checks whether a mapping context already contains a specific action/key pair. 
	bool MappingContextHasActionKey(const UInputMappingContext* MappingContext, const UInputAction* Action, const FKey& Key) const; // Tests for a specific key mapping.

	// Builds fallback mappings for missing actions inside the provided context. 
	void BuildFallbackMappings(UInputMappingContext* MappingContext, bool bNeedMoveMappings, bool bNeedLookMappings, bool bNeedHoldMappings, bool bNeedSelectMappings); // Generates default key mappings.

	// Adds a mapping entry with optional modifiers. 
	void AddMappingWithModifiers(UInputMappingContext* MappingContext, UInputAction* Action, const FKey& Key, const TArray<UInputModifier*>& Modifiers); // Creates a mapping entry with modifiers.

	// Applies Game & UI input mode with optional cursor locking. 
	void ApplyInputMode(bool bLockMouse) const; // Updates the player controller input mode.

	// Checks whether the cursor is currently over a UI widget. 
	bool IsCursorOverUI() const; // Prevents look-lock when interacting with UI.

	// Attempts to select a villager under the cursor and toggle the needs widget. 
	void TrySelectVillager(); // Performs a cursor trace and toggles the villager UI.

	// Determines whether the current mouse hold qualifies as a selection click. 
	bool IsSelectClick() const; // Differentiates short clicks from drag input.
#pragma endregion PrivateMethods

private:
#pragma region PrivateState
	// Root scene component used for movement updates. 
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USceneComponent> RootScene;

	// Camera component providing the first-person view. 
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> CameraComponent;

	// Movement component enabling horizontal translation. 
	UPROPERTY(VisibleAnywhere, Category = "Movement")
	TObjectPtr<UFloatingPawnMovement> MovementComponent;

	// Enhanced input mapping context asset for player controls. 
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	// Enhanced input action asset for planar movement. 
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	// Enhanced input action asset for mouse look. 
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	// Enhanced input action asset for activating mouse look while held. 
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> LookHoldAction;

	// Enhanced input action asset for selecting villagers. 
	UPROPERTY(EditAnywhere, Category = "Input")
	TObjectPtr<UInputAction> SelectAction;

	// Transient fallback mapping context used when required mappings are missing. 
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> FallbackMappingContext; // Stores the runtime fallback mapping context.

	// Camera height offset relative to the pawn origin. 
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraHeight = 64.0f;

	// Scalar multiplier applied to mouse look input. 
	UPROPERTY(EditAnywhere, Category = "Input")
	float LookSensitivity = 1.0f;

	// Enables inversion of yaw input for camera rotation. 
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bInvertYaw = false;

	// Enables inversion of pitch input for camera rotation. 
	UPROPERTY(EditAnywhere, Category = "Input")
	bool bInvertPitch = false;

	// Maximum hold duration to treat a mouse press as a selection click. 
	UPROPERTY(EditAnywhere, Category = "Input")
	float ClickSelectMaxHoldSeconds = 0.18f;

	// Maximum cursor delta to treat a mouse press as a selection click. 
	UPROPERTY(EditAnywhere, Category = "Input")
	float ClickSelectMaxPixelDelta = 8.0f;

	// Tracks whether the input mapping context has been applied. 
	bool bInputMappingApplied = false; // Prevents duplicate mapping context application.

	// Tracks whether fallback mappings have been constructed. 
	bool bFallbackMappingsBuilt = false; // Marks whether fallback mappings were built.

	// Tracks whether fallback mappings have been applied to the subsystem. 
	bool bFallbackMappingApplied = false; // Marks whether fallback mappings were applied.

	// Tracks whether mouse look is currently held. 
	bool bIsLookInputHeld = false; // Gates camera rotation while the hold action is active.

	// Tracks whether selection input is currently pressed. 
	bool bIsSelectPressed = false; // Tracks selection press state for click detection.

	// Tracks whether selection should be ignored because UI was hovered. 
	bool bSuppressSelection = false; // Prevents selection while interacting with UI.

	// Stores the timestamp of the selection press start. 
	float SelectPressStartTime = 0.0f; // Used to detect click duration.

	// Stores the cursor position at the start of a selection press. 
	FVector2D SelectPressStartCursorPosition = FVector2D::ZeroVector; // Used to detect click movement.

	// Tracks the currently selected villager actor. 
	TWeakObjectPtr<AActor> SelectedVillagerActor; // Used to toggle the same villager UI.

	// Tracks the display component for the selected villager. 
	TWeakObjectPtr<UVillagerNeedsDisplayComponent> SelectedNeedsDisplay; // Used to show/hide the needs widget.
#pragma endregion PrivateState
};
#pragma endregion FixedCameraPawnDeclaration // Ends the pawn declaration region. 
