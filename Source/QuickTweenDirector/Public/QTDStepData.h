// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Utils/EaseType.h"
#include "Utils/LoopType.h"
#include "QTDStepData.generated.h"

// ──────────────────────────────────────────────────────────────────────────────
// Enums
// ──────────────────────────────────────────────────────────────────────────────

/** Determines what kind of value this step animates. */
UENUM(BlueprintType)
enum class EQTDStepType : uint8
{
	Vector      UMETA(DisplayName = "Vector"),
	Rotator     UMETA(DisplayName = "Rotator"),
	Float       UMETA(DisplayName = "Float"),
	LinearColor UMETA(DisplayName = "Linear Color"),
	Vector2D    UMETA(DisplayName = "Vector 2D"),
	Int         UMETA(DisplayName = "Integer"),
	Empty       UMETA(DisplayName = "Empty (Delay)"),
};

/** Which FVector property on the target component to animate. */
UENUM(BlueprintType)
enum class EQTDVectorProperty : uint8
{
	WorldLocation    UMETA(DisplayName = "World Location"),
	RelativeLocation UMETA(DisplayName = "Relative Location"),
	WorldScale3D     UMETA(DisplayName = "World Scale 3D"),
	RelativeScale3D  UMETA(DisplayName = "Relative Scale 3D"),
};

/** Which FRotator property on the target component to animate. */
UENUM(BlueprintType)
enum class EQTDRotatorProperty : uint8
{
	WorldRotation    UMETA(DisplayName = "World Rotation"),
	RelativeRotation UMETA(DisplayName = "Relative Rotation"),
};

/** How the float value is applied when the step type is Float. */
UENUM(BlueprintType)
enum class EQTDFloatTarget : uint8
{
	/** Sets a scalar parameter on a UMaterialInstanceDynamic. */
	MaterialScalar      UMETA(DisplayName = "Material Scalar Parameter"),
};

/** How the color value is applied when the step type is LinearColor. */
UENUM(BlueprintType)
enum class EQTDColorTarget : uint8
{
	/** Sets a vector parameter on a UMaterialInstanceDynamic. */
	MaterialVector      UMETA(DisplayName = "Material Vector Parameter"),
};

/** How the Vector2D value is applied when the step type is Vector2D. */
UENUM(BlueprintType)
enum class EQTDVector2DTarget : uint8
{
	/** Animates the XY components of a material vector parameter (e.g. UV tiling/offset). */
	MaterialVector2D UMETA(DisplayName = "Material Vector2D Parameter (UV)"),
};

/** How the integer value is applied when the step type is Int. */
UENUM(BlueprintType)
enum class EQTDIntTarget : uint8
{
	/** Sets a scalar parameter on a UMaterialInstanceDynamic using a rounded integer value. */
	MaterialScalarInt UMETA(DisplayName = "Material Scalar (Integer)"),
};

// ──────────────────────────────────────────────────────────────────────────────
// Step data
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Self-contained description of one animation step inside a QuickTweenDirector asset.
 * All fields are serialized so the step survives editor restarts.
 */
USTRUCT(BlueprintType)
struct QUICKTWEENDIRECTOR_API FQTDStepData
{
	GENERATED_BODY()

	// ── Identity ──────────────────────────────────────────────────────────────

	/** Stable unique ID for this step (generated on construction). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step|Identity")
	FGuid StepId;

	/** Human-readable label shown inside the timeline box. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Identity")
	FString Label;

	/** The track this step belongs to (editor use). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Step|Identity")
	FGuid TrackId;

	/** Kind of value this step animates. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Identity")
	EQTDStepType StepType = EQTDStepType::Empty;

	// ── Timing ────────────────────────────────────────────────────────────────

	/** Absolute start time (seconds) relative to the director sequence start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Timing", meta = (ClampMin = "0.0"))
	float StartTime = 0.0f;

	/** Duration of one loop of this step (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Timing", meta = (ClampMin = "0.001"))
	float Duration = 1.0f;

	/** Time-scale multiplier applied to this step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Timing", meta = (ClampMin = "0.01"))
	float TimeScale = 1.0f;

	/** Number of times this step loops (1 = play once, -1 = infinite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Timing")
	int32 Loops = 1;

	/** Loop behavior. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Timing")
	ELoopType LoopType = ELoopType::Restart;

	// ── Easing ────────────────────────────────────────────────────────────────

	/** Easing curve type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Easing")
	EEaseType EaseType = EEaseType::Linear;

	/** Optional custom UCurveFloat for easing (loaded lazily at runtime). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Easing")
	TSoftObjectPtr<UCurveFloat> EaseCurve;

	// ── Target Binding ────────────────────────────────────────────────────────

	/**
	 * Named slot resolved to a USceneComponent or AActor at runtime.
	 * Leave empty to use the world-context object passed to Build().
	 * Bind the slot using UQuickTweenDirectorPlayer::BindSlot().
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Target")
	FName SlotName;

	/**
	 * Named parameter (used for material scalar / vector params).
	 * Set this to the material parameter name when FloatTarget == MaterialScalar
	 * or ColorTarget == MaterialVector.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Target")
	FName ParameterName;

	// ── Vector-specific ───────────────────────────────────────────────────────

	/** Which FVector property to animate on the bound component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector",
		meta = (EditCondition = "StepType == EQTDStepType::Vector", EditConditionHides))
	EQTDVectorProperty VectorProperty = EQTDVectorProperty::RelativeLocation;

	/** Start value. Ignored when bVectorFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector",
		meta = (EditCondition = "StepType == EQTDStepType::Vector && !bVectorFromCurrent", EditConditionHides))
	FVector VectorFrom = FVector::ZeroVector;

	/** End value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector",
		meta = (EditCondition = "StepType == EQTDStepType::Vector", EditConditionHides))
	FVector VectorTo = FVector::ZeroVector;

	/** When true the property's current value is used as the start of the tween. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector",
		meta = (EditCondition = "StepType == EQTDStepType::Vector", EditConditionHides))
	bool bVectorFromCurrent = true;

	// ── Rotator-specific ──────────────────────────────────────────────────────

	/** Which FRotator property to animate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Rotator",
		meta = (EditCondition = "StepType == EQTDStepType::Rotator", EditConditionHides))
	EQTDRotatorProperty RotatorProperty = EQTDRotatorProperty::RelativeRotation;

	/** Start rotation. Ignored when bRotatorFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Rotator",
		meta = (EditCondition = "StepType == EQTDStepType::Rotator && !bRotatorFromCurrent", EditConditionHides))
	FRotator RotatorFrom = FRotator::ZeroRotator;

	/** End rotation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Rotator",
		meta = (EditCondition = "StepType == EQTDStepType::Rotator", EditConditionHides))
	FRotator RotatorTo = FRotator::ZeroRotator;

	/** When true the property's current rotation is used as the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Rotator",
		meta = (EditCondition = "StepType == EQTDStepType::Rotator", EditConditionHides))
	bool bRotatorFromCurrent = true;

	// ── Float-specific ────────────────────────────────────────────────────────

	/** How the float value is delivered to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Float",
		meta = (EditCondition = "StepType == EQTDStepType::Float", EditConditionHides))
	EQTDFloatTarget FloatTarget = EQTDFloatTarget::MaterialScalar;

	/** Start float value. Ignored when bFloatFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Float",
		meta = (EditCondition = "StepType == EQTDStepType::Float && !bFloatFromCurrent", EditConditionHides))
	float FloatFrom = 0.0f;

	/** End float value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Float",
		meta = (EditCondition = "StepType == EQTDStepType::Float", EditConditionHides))
	float FloatTo = 1.0f;

	/** When true the current value of the property is used as the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Float",
		meta = (EditCondition = "StepType == EQTDStepType::Float", EditConditionHides))
	bool bFloatFromCurrent = false;

	// ── LinearColor-specific ──────────────────────────────────────────────────

	/** How the color is delivered to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Color",
		meta = (EditCondition = "StepType == EQTDStepType::LinearColor", EditConditionHides))
	EQTDColorTarget ColorTarget = EQTDColorTarget::MaterialVector;

	/** Start color. Ignored when bColorFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Color",
		meta = (EditCondition = "StepType == EQTDStepType::LinearColor && !bColorFromCurrent", EditConditionHides))
	FLinearColor ColorFrom = FLinearColor::White;

	/** End color. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Color",
		meta = (EditCondition = "StepType == EQTDStepType::LinearColor", EditConditionHides))
	FLinearColor ColorTo = FLinearColor::Black;

	/** When true the current color is used as the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Color",
		meta = (EditCondition = "StepType == EQTDStepType::LinearColor", EditConditionHides))
	bool bColorFromCurrent = false;

	// ── Vector2D-specific ─────────────────────────────────────────────────

	/** How the Vector2D value is delivered to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector2D",
		meta = (EditCondition = "StepType == EQTDStepType::Vector2D", EditConditionHides))
	EQTDVector2DTarget Vector2DTarget = EQTDVector2DTarget::MaterialVector2D;

	/** Start XY value. Ignored when bVector2DFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector2D",
		meta = (EditCondition = "StepType == EQTDStepType::Vector2D && !bVector2DFromCurrent", EditConditionHides))
	FVector2D Vector2DFrom = FVector2D::ZeroVector;

	/** End XY value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector2D",
		meta = (EditCondition = "StepType == EQTDStepType::Vector2D", EditConditionHides))
	FVector2D Vector2DTo = FVector2D::ZeroVector;

	/** When true the current XY of the parameter is used as the start. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Vector2D",
		meta = (EditCondition = "StepType == EQTDStepType::Vector2D", EditConditionHides))
	bool bVector2DFromCurrent = false;

	// ── Int-specific ──────────────────────────────────────────────────────

	/** How the integer value is delivered to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Int",
		meta = (EditCondition = "StepType == EQTDStepType::Int", EditConditionHides))
	EQTDIntTarget IntTarget = EQTDIntTarget::MaterialScalarInt;

	/** Start integer value. Ignored when bIntFromCurrent is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Int",
		meta = (EditCondition = "StepType == EQTDStepType::Int && !bIntFromCurrent", EditConditionHides))
	int32 IntFrom = 0;

	/** End integer value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Int",
		meta = (EditCondition = "StepType == EQTDStepType::Int", EditConditionHides))
	int32 IntTo = 1;

	/** When true the current value of the scalar parameter is used as the start (rounded). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Int",
		meta = (EditCondition = "StepType == EQTDStepType::Int", EditConditionHides))
	bool bIntFromCurrent = false;

	// ── Appearance ────────────────────────────────────────────────────────────

	/**
	 * Optional user-defined color override shown in the timeline editor.
	 * Alpha == 0 means "use the default type color".
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Step|Appearance")
	FLinearColor UserColor = FLinearColor(0.f, 0.f, 0.f, 0.f);

	// ── Helpers ───────────────────────────────────────────────────────────────

	/** Returns start + duration * loops (total wall-clock time consumed). */
	float GetEndTime() const { return StartTime + Duration * FMath::Max(Loops, 1); }

	/** Returns the intrinsic type color for this step. */
	FLinearColor GetTypeColor() const;

	/** Returns UserColor when its alpha > 0, otherwise GetTypeColor(). */
	FLinearColor GetDisplayColor() const { return UserColor.A > 0.f ? UserColor : GetTypeColor(); }
};

// ──────────────────────────────────────────────────────────────────────────────
// Track data
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Editor-only track metadata. Tracks group steps visually in the timeline.
 * They have no runtime meaning beyond step assignment.
 */
USTRUCT(BlueprintType)
struct QUICKTWEENDIRECTOR_API FQTDTrackData
{
	GENERATED_BODY()

	/** Unique track identifier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Track")
	FGuid TrackId;

	/** Label shown on the left panel of the timeline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	FString TrackLabel = TEXT("New Track");

	/** Step type assumed when the user adds a step to this track without specifying a type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Track")
	EQTDStepType DefaultStepType = EQTDStepType::Vector;

	/**
	 * The Blueprint variable name of the component this track animates.
	 * Set automatically when the track is created via the Blueprint Director panel.
	 * At runtime, UQuickTweenDirectorPlayer will auto-bind this name to the matching
	 * component on the owning actor — no manual BindSlot() call needed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Track")
	FName ComponentVariableName;

	/**
	 * The class of the component. Stored so the editor can filter animation type
	 * options without needing a live Blueprint reference.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Track")
	TSubclassOf<UActorComponent> ComponentClass;

};
