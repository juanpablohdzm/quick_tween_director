// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QuickTweenDirectorPlayer.h"

#include "QuickTweenDirectorAsset.h"
#include "QuickTweenManager.h"
#include "Tweens/QuickVectorTween.h"
#include "Tweens/QuickRotatorTween.h"
#include "Tweens/QuickFloatTween.h"
#include "Tweens/QuickEmptyTween.h"
#include "Tweens/QuickVector2DTween.h"
#include "Tweens/QuickIntTween.h"
#include "Utils/CommonValues.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogQTDPlayer, Log, All);

// ──────────────────────────────────────────────────────────────────────────────
// Factory (used only by UQuickTweenDirectorLibrary)
// ──────────────────────────────────────────────────────────────────────────────

UQuickTweenDirectorPlayer* UQuickTweenDirectorPlayer::Create(UObject* WorldContext, UQuickTweenDirectorAsset* InAsset)
{
	if (!InAsset)
	{
		UE_LOG(LogQTDPlayer, Warning, TEXT("Create called with null asset."));
		return nullptr;
	}

	UQuickTweenManager* Manager = UQuickTweenManager::Get(WorldContext);
	if (!Manager)
	{
		UE_LOG(LogQTDPlayer, Warning, TEXT("Create: no QuickTweenManager found for world context."));
		return nullptr;
	}

	UQuickTweenDirectorPlayer* Player = NewObject<UQuickTweenDirectorPlayer>(WorldContext);
	Player->Asset              = InAsset;
	Player->WorldContextObject = WorldContext;
	Manager->AddTween(Player);
	return Player;
}

// ──────────────────────────────────────────────────────────────────────────────
// Slot binding
// ──────────────────────────────────────────────────────────────────────────────

void UQuickTweenDirectorPlayer::BindSlot(FName SlotName, UObject* Object)
{
	if (!Object)
	{
		UE_LOG(LogQTDPlayer, Warning, TEXT("BindSlot(%s): Object is null."), *SlotName.ToString());
		return;
	}
	SlotBindings.Add(SlotName, TWeakObjectPtr<UObject>(Object));
}

// ──────────────────────────────────────────────────────────────────────────────
// Tween accessors
// ──────────────────────────────────────────────────────────────────────────────

UQuickTweenBase* UQuickTweenDirectorPlayer::GetTweenByStepId(FGuid StepId) const
{
	for (int32 i = 0; i < BuiltStepData.Num(); ++i)
	{
		if (BuiltStepData[i].StepId == StepId && BuiltSteps.IsValidIndex(i))
		{
			return Cast<UQuickTweenBase>(BuiltSteps[i].Tween);
		}
	}
	return nullptr;
}

UQuickTweenBase* UQuickTweenDirectorPlayer::GetTweenByLabel(const FString& Label) const
{
	for (int32 i = 0; i < BuiltStepData.Num(); ++i)
	{
		if (BuiltStepData[i].Label == Label && BuiltSteps.IsValidIndex(i))
		{
			return Cast<UQuickTweenBase>(BuiltSteps[i].Tween);
		}
	}
	return nullptr;
}

// ──────────────────────────────────────────────────────────────────────────────
// Build
// ──────────────────────────────────────────────────────────────────────────────

USceneComponent* UQuickTweenDirectorPlayer::ResolveComponentSlot(FName SlotName) const
{
	// Empty name → use the world context object itself if it's an actor/component.
	const TWeakObjectPtr<UObject>* Found = SlotName.IsNone() ? nullptr : SlotBindings.Find(SlotName);

	UObject* Obj = Found ? Found->Get() : const_cast<UObject*>(WorldContextObject);
	if (!Obj) return nullptr;

	if (USceneComponent* Comp = Cast<USceneComponent>(Obj)) return Comp;
	if (AActor* Actor = Cast<AActor>(Obj))                  return Actor->GetRootComponent();

	return nullptr;
}

UMaterialInstanceDynamic* UQuickTweenDirectorPlayer::ResolveMIDSlot(FName SlotName) const
{
	const TWeakObjectPtr<UObject>* Found = SlotBindings.Find(SlotName);
	return Found ? Cast<UMaterialInstanceDynamic>(Found->Get()) : nullptr;
}

UQuickTweenBase* UQuickTweenDirectorPlayer::CreateTweenForStep(const FQTDStepData& Step)
{
	UCurveFloat* Curve = Step.EaseCurve.IsValid() ? Step.EaseCurve.Get() : nullptr;

	// ── Empty / delay ─────────────────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::Empty)
	{
		return UQuickEmptyTween::CreateTween(
			const_cast<UObject*>(WorldContextObject),
			Step.Duration,
			Step.Label,
			/*bAutoKill*/   false,
			/*WhilePaused*/ false,
			/*AutoPlay*/    false);
	}

	// ── Vector (location / scale) ─────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::Vector)
	{
		USceneComponent* Comp = ResolveComponentSlot(Step.SlotName);
		if (!Comp)
		{
			UE_LOG(LogQTDPlayer, Warning, TEXT("Step '%s': cannot resolve slot '%s' to a SceneComponent."),
				*Step.Label, *Step.SlotName.ToString());
			return nullptr;
		}

		TWeakObjectPtr<USceneComponent> WeakComp(Comp);
		const FVector ToVal = Step.VectorTo;
		const FVector FromVal = Step.VectorFrom;
		const bool bFromCurrent = Step.bVectorFromCurrent;

		FNativeVectorGetter GetterFrom, GetterTo;
		FNativeVectorSetter Setter;

		switch (Step.VectorProperty)
		{
			case EQTDVectorProperty::WorldLocation:
				GetterFrom = bFromCurrent
					? FNativeVectorGetter::CreateLambda([WeakComp](UQuickVectorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetComponentLocation() : FVector::ZeroVector;
					  })
					: FNativeVectorGetter::CreateLambda([FromVal](UQuickVectorTween*) { return FromVal; });
				GetterTo   = FNativeVectorGetter::CreateLambda([ToVal](UQuickVectorTween*) { return ToVal; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldLocation(V);
				});
				break;

			case EQTDVectorProperty::RelativeLocation:
				GetterFrom = bFromCurrent
					? FNativeVectorGetter::CreateLambda([WeakComp](UQuickVectorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetRelativeLocation() : FVector::ZeroVector;
					  })
					: FNativeVectorGetter::CreateLambda([FromVal](UQuickVectorTween*) { return FromVal; });
				GetterTo   = FNativeVectorGetter::CreateLambda([ToVal](UQuickVectorTween*) { return ToVal; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeLocation(V);
				});
				break;

			case EQTDVectorProperty::WorldScale3D:
				GetterFrom = bFromCurrent
					? FNativeVectorGetter::CreateLambda([WeakComp](UQuickVectorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetComponentScale() : FVector::OneVector;
					  })
					: FNativeVectorGetter::CreateLambda([FromVal](UQuickVectorTween*) { return FromVal; });
				GetterTo   = FNativeVectorGetter::CreateLambda([ToVal](UQuickVectorTween*) { return ToVal; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldScale3D(V);
				});
				break;

			case EQTDVectorProperty::RelativeScale3D:
				GetterFrom = bFromCurrent
					? FNativeVectorGetter::CreateLambda([WeakComp](UQuickVectorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetRelativeScale3D() : FVector::OneVector;
					  })
					: FNativeVectorGetter::CreateLambda([FromVal](UQuickVectorTween*) { return FromVal; });
				GetterTo   = FNativeVectorGetter::CreateLambda([ToVal](UQuickVectorTween*) { return ToVal; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeScale3D(V);
				});
				break;

			case EQTDVectorProperty::WorldLocationBy:
			{
				const FVector Snapshot = Comp->GetComponentLocation();
				const FVector Target   = Snapshot + ToVal;
				GetterFrom = FNativeVectorGetter::CreateLambda([Snapshot](UQuickVectorTween*) { return Snapshot; });
				GetterTo   = FNativeVectorGetter::CreateLambda([Target](UQuickVectorTween*)   { return Target; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldLocation(V);
				});
				break;
			}
			case EQTDVectorProperty::RelativeLocationBy:
			{
				const FVector Snapshot = Comp->GetRelativeLocation();
				const FVector Target   = Snapshot + ToVal;
				GetterFrom = FNativeVectorGetter::CreateLambda([Snapshot](UQuickVectorTween*) { return Snapshot; });
				GetterTo   = FNativeVectorGetter::CreateLambda([Target](UQuickVectorTween*)   { return Target; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeLocation(V);
				});
				break;
			}
			case EQTDVectorProperty::WorldScale3DBy:
			{
				const FVector Snapshot = Comp->GetComponentScale();
				const FVector Target   = Snapshot + ToVal;
				GetterFrom = FNativeVectorGetter::CreateLambda([Snapshot](UQuickVectorTween*) { return Snapshot; });
				GetterTo   = FNativeVectorGetter::CreateLambda([Target](UQuickVectorTween*)   { return Target; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldScale3D(V);
				});
				break;
			}
			case EQTDVectorProperty::RelativeScale3DBy:
			{
				const FVector Snapshot = Comp->GetRelativeScale3D();
				const FVector Target   = Snapshot + ToVal;
				GetterFrom = FNativeVectorGetter::CreateLambda([Snapshot](UQuickVectorTween*) { return Snapshot; });
				GetterTo   = FNativeVectorGetter::CreateLambda([Target](UQuickVectorTween*)   { return Target; });
				Setter     = FNativeVectorSetter::CreateWeakLambda(Comp, [WeakComp](const FVector& V, UQuickVectorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeScale3D(V);
				});
				break;
			}
		}

		return UQuickVectorTween::CreateTween(
			const_cast<UObject*>(WorldContextObject),
			MoveTemp(GetterFrom), MoveTemp(GetterTo), MoveTemp(Setter),
			Step.Duration, Step.TimeScale, Step.EaseType, Curve,
			Step.Loops, Step.LoopType, Step.Label,
			/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
	}

	// ── Rotator ───────────────────────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::Rotator)
	{
		USceneComponent* Comp = ResolveComponentSlot(Step.SlotName);
		if (!Comp)
		{
			UE_LOG(LogQTDPlayer, Warning, TEXT("Step '%s': cannot resolve slot '%s' to a SceneComponent."),
				*Step.Label, *Step.SlotName.ToString());
			return nullptr;
		}

		TWeakObjectPtr<USceneComponent> WeakComp(Comp);
		const FRotator ToVal    = Step.RotatorTo;
		const FRotator FromVal  = Step.RotatorFrom;
		const bool bFromCurrent = Step.bRotatorFromCurrent;

		FNativeRotatorGetter GetterFrom, GetterTo;
		FNativeRotatorSetter Setter;

		switch (Step.RotatorProperty)
		{
			case EQTDRotatorProperty::WorldRotation:
				GetterFrom = bFromCurrent
					? FNativeRotatorGetter::CreateLambda([WeakComp](UQuickRotatorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetComponentRotation() : FRotator::ZeroRotator;
					  })
					: FNativeRotatorGetter::CreateLambda([FromVal](UQuickRotatorTween*) { return FromVal; });
				GetterTo   = FNativeRotatorGetter::CreateLambda([ToVal](UQuickRotatorTween*) { return ToVal; });
				Setter     = FNativeRotatorSetter::CreateWeakLambda(Comp, [WeakComp](const FRotator& R, UQuickRotatorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldRotation(R);
				});
				break;

			case EQTDRotatorProperty::RelativeRotation:
				GetterFrom = bFromCurrent
					? FNativeRotatorGetter::CreateLambda([WeakComp](UQuickRotatorTween*) {
						  return WeakComp.IsValid() ? WeakComp->GetRelativeRotation() : FRotator::ZeroRotator;
					  })
					: FNativeRotatorGetter::CreateLambda([FromVal](UQuickRotatorTween*) { return FromVal; });
				GetterTo   = FNativeRotatorGetter::CreateLambda([ToVal](UQuickRotatorTween*) { return ToVal; });
				Setter     = FNativeRotatorSetter::CreateWeakLambda(Comp, [WeakComp](const FRotator& R, UQuickRotatorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeRotation(R);
				});
				break;

			case EQTDRotatorProperty::WorldRotationBy:
			{
				const FRotator Snapshot = Comp->GetComponentRotation();
				const FRotator Target   = Snapshot + ToVal;
				GetterFrom = FNativeRotatorGetter::CreateLambda([Snapshot](UQuickRotatorTween*) { return Snapshot; });
				GetterTo   = FNativeRotatorGetter::CreateLambda([Target](UQuickRotatorTween*)   { return Target; });
				Setter     = FNativeRotatorSetter::CreateWeakLambda(Comp, [WeakComp](const FRotator& R, UQuickRotatorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetWorldRotation(R);
				});
				break;
			}
			case EQTDRotatorProperty::RelativeRotationBy:
			{
				const FRotator Snapshot = Comp->GetRelativeRotation();
				const FRotator Target   = Snapshot + ToVal;
				GetterFrom = FNativeRotatorGetter::CreateLambda([Snapshot](UQuickRotatorTween*) { return Snapshot; });
				GetterTo   = FNativeRotatorGetter::CreateLambda([Target](UQuickRotatorTween*)   { return Target; });
				Setter     = FNativeRotatorSetter::CreateWeakLambda(Comp, [WeakComp](const FRotator& R, UQuickRotatorTween*) {
					if (WeakComp.IsValid()) WeakComp->SetRelativeRotation(R);
				});
				break;
			}
		}

		return UQuickRotatorTween::CreateTween(
			const_cast<UObject*>(WorldContextObject),
			MoveTemp(GetterFrom), MoveTemp(GetterTo),
			/*bShortestPath*/ true,
			MoveTemp(Setter),
			Step.Duration, Step.TimeScale, Step.EaseType, Curve,
			Step.Loops, Step.LoopType, Step.Label,
			/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
	}

	// ── Float ─────────────────────────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::Float)
	{
		if (Step.FloatTarget == EQTDFloatTarget::MaterialScalar)
		{
			UMaterialInstanceDynamic* MID = ResolveMIDSlot(Step.SlotName);
			if (!MID)
			{
				UE_LOG(LogQTDPlayer, Warning,
					TEXT("Step '%s': slot '%s' is not a UMaterialInstanceDynamic."),
					*Step.Label, *Step.SlotName.ToString());
				return nullptr;
			}

			TWeakObjectPtr<UMaterialInstanceDynamic> WeakMID(MID);
			const FName ParamName = Step.ParameterName;
			const float ToVal   = Step.FloatTo;
			const float FromVal = Step.FloatFrom;

			FNativeFloatGetter GetterFrom = Step.bFloatFromCurrent
				? FNativeFloatGetter::CreateLambda([WeakMID, ParamName](UQuickFloatTween*) {
					  float V = 0.0f;
					  if (WeakMID.IsValid()) WeakMID->GetScalarParameterValue(ParamName, V);
					  return V;
				  })
				: FNativeFloatGetter::CreateLambda([FromVal](UQuickFloatTween*) { return FromVal; });

			FNativeFloatGetter GetterTo = FNativeFloatGetter::CreateLambda([ToVal](UQuickFloatTween*) { return ToVal; });
			FNativeFloatSetter Setter   = FNativeFloatSetter::CreateWeakLambda(MID,
				[WeakMID, ParamName](float V, UQuickFloatTween*) {
					if (WeakMID.IsValid()) WeakMID->SetScalarParameterValue(ParamName, V);
				});

			return UQuickFloatTween::CreateTween(
				const_cast<UObject*>(WorldContextObject),
				MoveTemp(GetterFrom), MoveTemp(GetterTo), MoveTemp(Setter),
				Step.Duration, Step.TimeScale, Step.EaseType, Curve,
				Step.Loops, Step.LoopType, Step.Label,
				/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
		}

		return nullptr;
	}

	// ── LinearColor ───────────────────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::LinearColor)
	{
		if (Step.ColorTarget == EQTDColorTarget::MaterialVector)
		{
			UMaterialInstanceDynamic* MID = ResolveMIDSlot(Step.SlotName);
			if (!MID)
			{
				UE_LOG(LogQTDPlayer, Warning,
					TEXT("Step '%s': slot '%s' is not a UMaterialInstanceDynamic."),
					*Step.Label, *Step.SlotName.ToString());
				return nullptr;
			}

			TWeakObjectPtr<UMaterialInstanceDynamic> WeakMID(MID);
			const FName ParamName    = Step.ParameterName;
			const FLinearColor ToVal = Step.ColorTo;
			const FLinearColor FromVal = Step.ColorFrom;

			const FVector ToVec   = FVector(ToVal.R,   ToVal.G,   ToVal.B);
			const FVector FromVec = FVector(FromVal.R, FromVal.G, FromVal.B);

			FNativeVectorGetter GetterFrom = Step.bColorFromCurrent
				? FNativeVectorGetter::CreateLambda([WeakMID, ParamName](UQuickVectorTween*) {
					  FLinearColor C = FLinearColor::Black;
					  if (WeakMID.IsValid()) WeakMID->GetVectorParameterValue(ParamName, C);
					  return FVector(C.R, C.G, C.B);
				  })
				: FNativeVectorGetter::CreateLambda([FromVec](UQuickVectorTween*) { return FromVec; });

			FNativeVectorGetter GetterTo = FNativeVectorGetter::CreateLambda([ToVec](UQuickVectorTween*) { return ToVec; });
			FNativeVectorSetter Setter   = FNativeVectorSetter::CreateWeakLambda(MID,
				[WeakMID, ParamName](const FVector& V, UQuickVectorTween*) {
					if (WeakMID.IsValid())
						WeakMID->SetVectorParameterValue(ParamName, FLinearColor(V.X, V.Y, V.Z));
				});

			return UQuickVectorTween::CreateTween(
				const_cast<UObject*>(WorldContextObject),
				MoveTemp(GetterFrom), MoveTemp(GetterTo), MoveTemp(Setter),
				Step.Duration, Step.TimeScale, Step.EaseType, Curve,
				Step.Loops, Step.LoopType, Step.Label,
				/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
		}

		return nullptr;
	}

	// ── Vector2D (material UV) ───────────────────────────────────────────────
	if (Step.StepType == EQTDStepType::Vector2D)
	{
		if (Step.Vector2DTarget == EQTDVector2DTarget::MaterialVector2D)
		{
			UMaterialInstanceDynamic* MID = ResolveMIDSlot(Step.SlotName);
			if (!MID)
			{
				UE_LOG(LogQTDPlayer, Warning,
					TEXT("Step '%s': slot '%s' is not a UMaterialInstanceDynamic."),
					*Step.Label, *Step.SlotName.ToString());
				return nullptr;
			}
			TWeakObjectPtr<UMaterialInstanceDynamic> WeakMID(MID);
			const FName     ParamName = Step.ParameterName;
			const FVector2D ToVal     = Step.Vector2DTo;
			const FVector2D FromVal   = Step.Vector2DFrom;
			FNativeVector2DGetter GetterFrom = Step.bVector2DFromCurrent
				? FNativeVector2DGetter::CreateLambda([WeakMID, ParamName](UQuickVector2DTween*) {
					  FLinearColor C = FLinearColor::Black;
					  if (WeakMID.IsValid()) WeakMID->GetVectorParameterValue(ParamName, C);
					  return FVector2D(C.R, C.G);
				  })
				: FNativeVector2DGetter::CreateLambda([FromVal](UQuickVector2DTween*) { return FromVal; });
			FNativeVector2DGetter GetterTo = FNativeVector2DGetter::CreateLambda([ToVal](UQuickVector2DTween*) { return ToVal; });
			// Snapshot ZW channels once so the setter never needs to read back from GPU.
			FLinearColor ZWSnapshot;
			MID->GetVectorParameterValue(ParamName, ZWSnapshot);
			const float SnapshotB = ZWSnapshot.B;
			const float SnapshotA = ZWSnapshot.A;
FNativeVector2DSetter Setter = FNativeVector2DSetter::CreateWeakLambda(MID,
				[WeakMID, ParamName, SnapshotB, SnapshotA](const FVector2D& V, UQuickVector2DTween*) {
					if (WeakMID.IsValid())
						WeakMID->SetVectorParameterValue(ParamName, FLinearColor(V.X, V.Y, SnapshotB, SnapshotA));
				});
			return UQuickVector2DTween::CreateTween(
				const_cast<UObject*>(WorldContextObject),
				MoveTemp(GetterFrom), MoveTemp(GetterTo), MoveTemp(Setter),
				Step.Duration, Step.TimeScale, Step.EaseType, Curve,
				Step.Loops, Step.LoopType, Step.Label,
				/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
		}
		return nullptr;
	}

	// ── Int (material scalar as integer) ─────────────────────────────────────
	if (Step.StepType == EQTDStepType::Int)
	{
		if (Step.IntTarget == EQTDIntTarget::MaterialScalarInt)
		{
			UMaterialInstanceDynamic* MID = ResolveMIDSlot(Step.SlotName);
			if (!MID)
			{
				UE_LOG(LogQTDPlayer, Warning,
					TEXT("Step '%s': slot '%s' is not a UMaterialInstanceDynamic."),
					*Step.Label, *Step.SlotName.ToString());
				return nullptr;
			}
			TWeakObjectPtr<UMaterialInstanceDynamic> WeakMID(MID);
			const FName ParamName = Step.ParameterName;
			const int32 ToVal   = Step.IntTo;
			const int32 FromVal = Step.IntFrom;
			FNativeIntGetter GetterFrom = Step.bIntFromCurrent
				? FNativeIntGetter::CreateLambda([WeakMID, ParamName](UQuickIntTween*) {
					  float V = 0.0f;
					  if (WeakMID.IsValid()) WeakMID->GetScalarParameterValue(ParamName, V);
					  return FMath::RoundToInt(V);
				  })
				: FNativeIntGetter::CreateLambda([FromVal](UQuickIntTween*) { return FromVal; });
			FNativeIntGetter GetterTo = FNativeIntGetter::CreateLambda([ToVal](UQuickIntTween*) { return ToVal; });
			FNativeIntSetter Setter = FNativeIntSetter::CreateWeakLambda(MID,
				[WeakMID, ParamName](const int32 V, UQuickIntTween*) {
					if (WeakMID.IsValid()) WeakMID->SetScalarParameterValue(ParamName, (float)V);
				});
			return UQuickIntTween::CreateTween(
				const_cast<UObject*>(WorldContextObject),
				MoveTemp(GetterFrom), MoveTemp(GetterTo), MoveTemp(Setter),
				Step.Duration, Step.TimeScale, Step.EaseType, Curve,
				Step.Loops, Step.LoopType, Step.Label,
				/*bAutoKill*/ false, /*WhilePaused*/ false, /*AutoPlay*/ false);
		}
		return nullptr;
	}

	return nullptr;
}

bool UQuickTweenDirectorPlayer::Build(UObject* WorldContext)
{
	if (!Asset)
	{
		UE_LOG(LogQTDPlayer, Error, TEXT("Build: no asset set."));
		return false;
	}

	if (WorldContext) WorldContextObject = WorldContext;

	// ── Auto-bind component slots ──────────────────────────────────────────────
	if (AActor* OwnerActor = Cast<AActor>(const_cast<UObject*>(WorldContextObject)))
	{
		for (const FQTDTrackData& Track : Asset->Tracks)
		{
			if (Track.ComponentVariableName.IsNone()) continue;
			if (SlotBindings.Contains(Track.ComponentVariableName)) continue;

			for (UActorComponent* Comp : OwnerActor->GetComponents())
			{
				if (Comp && Comp->GetFName() == Track.ComponentVariableName)
				{
					SlotBindings.Add(Track.ComponentVariableName, TWeakObjectPtr<UObject>(Comp));
					UE_LOG(LogQTDPlayer, Verbose,
						TEXT("Build: auto-bound slot '%s' -> %s"),
						*Track.ComponentVariableName.ToString(), *Comp->GetName());
					break;
				}
			}

			if (!SlotBindings.Contains(Track.ComponentVariableName))
			{
				FObjectProperty* Prop = FindFProperty<FObjectProperty>(
					OwnerActor->GetClass(), Track.ComponentVariableName);
				if (Prop)
				{
					UObject* Obj = Prop->GetObjectPropertyValue_InContainer(OwnerActor);
					if (Obj)
					{
						SlotBindings.Add(Track.ComponentVariableName, TWeakObjectPtr<UObject>(Obj));
						UE_LOG(LogQTDPlayer, Verbose,
							TEXT("Build: auto-bound slot '%s' via reflection -> %s"),
							*Track.ComponentVariableName.ToString(), *Obj->GetName());
					}
				}

				if (!SlotBindings.Contains(Track.ComponentVariableName))
				{
					const FString Msg = FString::Printf(
						TEXT("Track '%s': slot '%s' could not be auto-bound — no matching component or property found on '%s'."),
						*Track.TrackLabel, *Track.ComponentVariableName.ToString(), *OwnerActor->GetName());
					BindingErrors.Add(Msg);
					UE_LOG(LogQTDPlayer, Warning, TEXT("%s"), *Msg);
				}
			}
		}
	}

	OwnedTweens.Empty();
	BuiltSteps.Empty();
	BuiltStepData.Empty();
	ActiveStepIndices.Empty();
	BindingErrors.Empty();

	UQuickTweenManager* Manager = UQuickTweenManager::Get(WorldContextObject);

	for (const FQTDStepData& Step : Asset->Steps)
	{
		UQuickTweenBase* Tween = CreateTweenForStep(Step);
		if (!Tween) continue;

		if (Manager) Manager->RemoveTween(Tween);
		Tween->SetOwner(this);

		OwnedTweens.Add(Tween);
		BuiltStepData.Add(Step);

		FQTDBuiltStep BuiltStep;
		BuiltStep.Tween         = Tween;
		BuiltStep.StartTime     = Step.StartTime;
		BuiltStep.TotalDuration = Tween->GetTotalDuration();
		BuiltSteps.Add(BuiltStep);
	}

	// Sort ascending by start time for efficient seek.
	// Note: BuiltSteps and BuiltStepData are sorted together via index.
	TArray<int32> SortedIndices;
	SortedIndices.SetNum(BuiltSteps.Num());
	for (int32 i = 0; i < SortedIndices.Num(); ++i) SortedIndices[i] = i;
	SortedIndices.Sort([this](int32 A, int32 B) {
		return BuiltSteps[A].StartTime < BuiltSteps[B].StartTime;
	});

	TArray<FQTDBuiltStep> SortedSteps;
	TArray<FQTDStepData>  SortedData;
	TArray<UQuickTweenable*> SortedTweens;
	for (int32 Idx : SortedIndices)
	{
		SortedSteps.Add(BuiltSteps[Idx]);
		SortedData.Add(BuiltStepData[Idx]);
		SortedTweens.Add(OwnedTweens[Idx]);
	}
	BuiltSteps    = MoveTemp(SortedSteps);
	BuiltStepData = MoveTemp(SortedData);
	OwnedTweens   = MoveTemp(SortedTweens);

	bIsBuilt = true;
	return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// Control
// ──────────────────────────────────────────────────────────────────────────────

void UQuickTweenDirectorPlayer::Play()
{
	if (HasOwner()) return;

	if (!bIsBuilt)
	{
		if (!Build(const_cast<UObject*>(WorldContextObject)))
		{
			UE_LOG(LogQTDPlayer, Warning, TEXT("Play: Build failed — player will not play."));
			return;
		}
	}

	const EQuickTweenState Prev = PlayerState;
	if (RequestStateTransition(EQuickTweenState::Play))
	{
		if (Prev == EQuickTweenState::Idle)
		{
			ElapsedTime           = bIsReversed ? GetTotalDuration() : 0.0f;
			CurrentLoop           = bIsReversed ? GetLoops() - 1 : 0;
			PreviousLoopLocalTime = bIsReversed ? GetLoopDuration() : 0.0f;
			HandleOnStart();
		}
	}
}

void UQuickTweenDirectorPlayer::Pause()
{
	if (HasOwner()) return;
	RequestStateTransition(EQuickTweenState::Pause);
}

void UQuickTweenDirectorPlayer::Reverse()
{
	if (HasOwner()) return;
	bIsReversed = !bIsReversed;
}

void UQuickTweenDirectorPlayer::Restart()
{
	if (HasOwner()) return;
	RequestStateTransition(EQuickTweenState::Idle);
}

void UQuickTweenDirectorPlayer::Complete(bool bSnapToEnd)
{
	if (HasOwner() || GetLoops() == INFINITE_LOOPS) return;
	bSnapToEndOnComplete = bSnapToEnd;
	if (RequestStateTransition(EQuickTweenState::Complete))
	{
		HandleOnComplete();
		if (GetAutoKill() && RequestStateTransition(EQuickTweenState::Kill))
		{
			HandleOnKill();
		}
	}
}

void UQuickTweenDirectorPlayer::Kill()
{
	if (HasOwner()) return;
	if (RequestStateTransition(EQuickTweenState::Kill))
	{
		HandleOnKill();
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Update / Evaluate
// ──────────────────────────────────────────────────────────────────────────────

void UQuickTweenDirectorPlayer::Update(float DeltaTime)
{
	if (HasOwner() || PlayerState != EQuickTweenState::Play) return;
	if (!bIsBuilt) return;

	ElapsedTime += (bIsReversed ? -1.0f : 1.0f) * DeltaTime;

	const float LoopDur = GetLoopDuration();
	if (FMath::IsNearlyZero(LoopDur)) return;

	int32 NewLoop = FMath::FloorToInt(ElapsedTime / LoopDur);
	if (NewLoop != CurrentLoop)
	{
		const int32 Crossed = FMath::Abs(NewLoop - CurrentLoop);
		CurrentLoop = NewLoop;
		if (bTriggerEvents && OnLoop.IsBound())
		{
			for (int32 i = 0; i < Crossed; ++i) OnLoop.Broadcast(this);
		}
	}

	const int32 TotalLoops = GetLoops();
	if (TotalLoops != INFINITE_LOOPS)
	{
		if ((!bIsReversed && CurrentLoop >= TotalLoops) || (bIsReversed && ElapsedTime < 0.0f))
		{
			if (RequestStateTransition(EQuickTweenState::Complete))
			{
				HandleOnComplete();
				if (GetAutoKill() && RequestStateTransition(EQuickTweenState::Kill))
				{
					HandleOnKill();
				}
			}
			return;
		}
	}

	float LocalTime = FMath::Fmod(ElapsedTime, LoopDur);
	if (LocalTime < 0.0f) LocalTime += LoopDur;

	if (GetLoopType() == ELoopType::PingPong && (CurrentLoop & 1) != 0)
	{
		LocalTime = LoopDur - LocalTime;
	}

	SeekTime(LocalTime);

	if (bTriggerEvents && OnUpdate.IsBound()) OnUpdate.Broadcast(this);
}

void UQuickTweenDirectorPlayer::Evaluate(const FQuickTweenEvaluatePayload& Payload, const UQuickTweenable* Instigator)
{
	if (!HasOwner() || !InstigatorIsOwner(Instigator)) return;
	if (!bIsBuilt)
	{
		if (!Build(const_cast<UObject*>(WorldContextObject))) return;
	}

	bIsReversed    = Payload.bIsReversed;
	bTriggerEvents = Payload.bShouldTriggerEvents;
	ElapsedTime    = FMath::Clamp(Payload.Value * GetTotalDuration(), 0.0f, GetTotalDuration());

	if (bWasActive != Payload.bIsActive)
	{
		if (Payload.bIsActive)
		{
			CurrentLoop           = bIsReversed ? GetLoops() - 1 : 0;
			PreviousLoopLocalTime = bIsReversed ? GetLoopDuration() : 0.0f;
			HandleOnStart();
		}
		else
		{
			const bool bAtEnd = (bIsReversed && FMath::IsNearlyZero(ElapsedTime)) ||
			                    (!bIsReversed && FMath::IsNearlyEqual(ElapsedTime, GetTotalDuration()));
			if (bAtEnd) HandleOnComplete();
		}
		bWasActive = Payload.bIsActive;
	}

	if (!Payload.bIsActive) return;

	const float LoopDur = GetLoopDuration();
	if (FMath::IsNearlyZero(LoopDur)) return;

	float LocalTime = FMath::Fmod(ElapsedTime, LoopDur);
	if (LocalTime < 0.0f) LocalTime += LoopDur;

	if (GetLoopType() == ELoopType::PingPong && (CurrentLoop & 1) != 0)
	{
		LocalTime = LoopDur - LocalTime;
	}

	SeekTime(LocalTime);

	if (bTriggerEvents && OnUpdate.IsBound()) OnUpdate.Broadcast(this);
}

// ──────────────────────────────────────────────────────────────────────────────
// SeekTime
// ──────────────────────────────────────────────────────────────────────────────

void UQuickTweenDirectorPlayer::SeekTime(float LoopLocalTime)
{
	CurrentLocalTime = LoopLocalTime;
	const bool bForward = LoopLocalTime >= PreviousLoopLocalTime;

	for (int32 i = 0; i < BuiltSteps.Num(); ++i)
	{
		FQTDBuiltStep& Entry = BuiltSteps[i];
		if (!Entry.Tween) continue;

		const float Start    = Entry.StartTime;
		const float End      = Start + Entry.TotalDuration;
		const bool  bActive        = (LoopLocalTime >= Start) && (LoopLocalTime <= End);
		const bool  bStepWasActive = ActiveStepIndices.Contains(i);

		// Fire begin/end step events on transitions.
		if (bTriggerEvents && BuiltStepData.IsValidIndex(i))
		{
			const FQTDStepData& SD = BuiltStepData[i];
			if (bActive && !bStepWasActive)
			{
				ActiveStepIndices.Add(i);
				if (OnStepBegin.IsBound()) OnStepBegin.Broadcast(this, SD.SlotName, SD.StepId);
			}
			else if (!bActive && bStepWasActive)
			{
				ActiveStepIndices.Remove(i);
				if (OnStepEnd.IsBound()) OnStepEnd.Broadcast(this, SD.SlotName, SD.StepId);
			}
		}

		const float ChildAlpha = (Entry.TotalDuration > 0.0f)
			? FMath::Clamp((LoopLocalTime - Start) / Entry.TotalDuration, 0.0f, 1.0f)
			: 1.0f;

		FQuickTweenEvaluatePayload Payload{
			/*.bIsActive            =*/ bActive,
			/*.bIsReversed          =*/ !bForward,
			/*.bShouldTriggerEvents =*/ bTriggerEvents,
			/*.Value                =*/ ChildAlpha
		};

		Entry.Tween->Evaluate(Payload, this);
	}

	PreviousLoopLocalTime = LoopLocalTime;
}

// ──────────────────────────────────────────────────────────────────────────────
// State machine
// ──────────────────────────────────────────────────────────────────────────────

bool UQuickTweenDirectorPlayer::RequestStateTransition(EQuickTweenState NewState)
{
	if (NewState == PlayerState) return false;

	const TArray<EQuickTweenState>* Allowed = ValidTransitions.Find(PlayerState);
	if (Allowed && Allowed->Contains(NewState))
	{
		PlayerState = NewState;
		return true;
	}
	return false;
}

void UQuickTweenDirectorPlayer::HandleOnStart()
{
	ActiveStepIndices.Empty();
	if (bTriggerEvents && OnStart.IsBound()) OnStart.Broadcast(this);
}

void UQuickTweenDirectorPlayer::HandleOnComplete()
{
	ElapsedTime = bIsReversed ? 0.0f : GetTotalDuration();

	const bool bSnap = bSnapToEndOnComplete
		? !(GetLoopType() == ELoopType::PingPong && GetLoops() % 2 == 0)
		: (GetLoopType() == ELoopType::PingPong && GetLoops() % 2 == 0);

	const float LoopDur = GetLoopDuration();
	SeekTime(bSnap ? LoopDur * 1.1f : LoopDur * -0.1f);

	if (bTriggerEvents && OnComplete.IsBound()) OnComplete.Broadcast(this);
}

void UQuickTweenDirectorPlayer::HandleOnKill()
{
	if (bTriggerEvents && OnKilled.IsBound()) OnKilled.Broadcast(this);
}

// ──────────────────────────────────────────────────────────────────────────────
// State queries
// ──────────────────────────────────────────────────────────────────────────────

float UQuickTweenDirectorPlayer::GetLoopDuration() const
{
	if (!Asset) return 0.0f;
	return Asset->GetTotalDuration();
}

float UQuickTweenDirectorPlayer::GetTotalDuration() const
{
	const int32 L = GetLoops();
	if (L == INFINITE_LOOPS) return TNumericLimits<float>::Max();
	return GetLoopDuration() * L;
}

int32 UQuickTweenDirectorPlayer::GetLoops() const
{
	return Asset ? Asset->Loops : 1;
}

ELoopType UQuickTweenDirectorPlayer::GetLoopType() const
{
	return Asset ? Asset->LoopType : ELoopType::Restart;
}

bool UQuickTweenDirectorPlayer::GetAutoKill() const
{
	return Asset ? Asset->bAutoKill : true;
}

bool UQuickTweenDirectorPlayer::GetShouldPlayWhilePaused() const
{
	return Asset ? Asset->bPlayWhilePaused : false;
}
