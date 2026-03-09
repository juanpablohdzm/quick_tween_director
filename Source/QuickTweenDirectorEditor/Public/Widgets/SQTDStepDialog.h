// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "QTDStepData.h"

class UQuickTweenDirectorAsset;
class SBorder;

/**
 * Modal-style dialog for editing a single FQTDStepData.
 * Opened by double-clicking a step box in the timeline.
 *
 * Uses UE's property editor (IDetailsView) to show the step's UPROPERTY fields
 * via a thin UObject wrapper, then writes changes back to the asset on confirm.
 */
class SQTDStepDialog : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQTDStepDialog) {}
		SLATE_ARGUMENT(FQTDStepData,                 StepData)
		SLATE_ARGUMENT(UQuickTweenDirectorAsset*,    Asset)
		SLATE_ARGUMENT(TSharedPtr<SWindow>,          ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply OnConfirm();
	FReply OnCancel();

	FQTDStepData EditedStep;
	UQuickTweenDirectorAsset* Asset = nullptr;
	TWeakPtr<SWindow> WeakParentWindow;

	// ── Cached widget refs for reading values back ────────────────────────────
	TSharedPtr<SEditableTextBox>  LabelBox;

	// Timing
	TSharedPtr<SEditableTextBox>  DurationBox;
	TSharedPtr<SEditableTextBox>  TimeScaleBox;
	TSharedPtr<SEditableTextBox>  LoopsBox;

	// Easing
	TArray<TSharedPtr<FString>>   EaseTypeOptions;
	TSharedPtr<STextBlock>        EaseTypeDisplay;
	int32                         SelectedEaseIndex = 0;
	TSoftObjectPtr<UCurveFloat>   EditEaseCurve;

	// Loop type
	TArray<TSharedPtr<FString>>   LoopTypeOptions;
	TSharedPtr<STextBlock>        LoopTypeDisplay;
	int32                         SelectedLoopTypeIndex = 0;

	// Parameter name (for material steps)
	TSharedPtr<SEditableTextBox>  ParameterNameBox;

	// Vector
	TSharedPtr<SEditableTextBox>  VecFromXBox, VecFromYBox, VecFromZBox;
	TSharedPtr<SEditableTextBox>  VecToXBox,   VecToYBox,   VecToZBox;

	// Rotator
	TSharedPtr<SEditableTextBox>  RotFromPitch, RotFromYaw, RotFromRoll;
	TSharedPtr<SEditableTextBox>  RotToPitch,   RotToYaw,   RotToRoll;

	// Float
	TSharedPtr<SEditableTextBox>  FloatFromBox, FloatToBox;

	// Color
	TSharedPtr<SBorder>           ColorFromSwatch, ColorToSwatch;
	FLinearColor                  EditColorFrom, EditColorTo;

	// User color override
	TSharedPtr<SBorder>           UserColorSwatch;
	FLinearColor                  EditUserColor;

	// ── Helper builders ───────────────────────────────────────────────────────
	TSharedRef<SWidget> BuildVectorRow(const FText& Label, TSharedPtr<SEditableTextBox>& X,
	                                   TSharedPtr<SEditableTextBox>& Y,
	                                   TSharedPtr<SEditableTextBox>& Z,
	                                   const FVector& Initial);

	TSharedRef<SWidget> BuildFloatRow(const FText& Label,
	                                  TSharedPtr<SEditableTextBox>& Box,
	                                  float Initial);

	TSharedRef<SWidget> BuildTypeSpecificSection();

	/** Writes all widget values back into EditedStep. Returns true if valid. */
	bool CollectValues();

	FReply LaunchColorPicker(bool bIsFrom);
	void OnColorPickerCommitted(FLinearColor NewColor, bool bIsFrom);
};
