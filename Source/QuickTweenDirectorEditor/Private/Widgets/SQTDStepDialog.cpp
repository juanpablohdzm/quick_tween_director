// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQTDStepDialog.h"
#include "QuickTweenDirectorAsset.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Utils/EaseType.h"
#include "Utils/LoopType.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Colors/SColorPicker.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Misc/FeedbackContext.h"

#define LOCTEXT_NAMESPACE "SQTDStepDialog"

// ──────────────────────────────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────────────────────────────

namespace
{
	TSharedRef<SWidget> MakeLabeledRow(const FText& Label, TSharedRef<SWidget> ValueWidget)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(130.0f)
				[
					SNew(STextBlock)
					.Text(Label)
					.Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				ValueWidget
			];
	}

	TSharedRef<SEditableTextBox> MakeFloatBox(float Initial)
	{
		return SNew(SEditableTextBox)
			.Text(FText::FromString(FString::SanitizeFloat(Initial)))
			.Font(FAppStyle::GetFontStyle("SmallFont"));
	}

	float ParseFloat(TSharedPtr<SEditableTextBox> Box, float Fallback = 0.0f)
	{
		if (!Box.IsValid()) return Fallback;
		float V = Fallback;
		LexFromString(V, *Box->GetText().ToString());
		return V;
	}

	int32 ParseInt(TSharedPtr<SEditableTextBox> Box, int32 Fallback = 1)
	{
		if (!Box.IsValid()) return Fallback;
		int32 V = Fallback;
		LexFromString(V, *Box->GetText().ToString());
		return V;
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Construct
// ──────────────────────────────────────────────────────────────────────────────

void SQTDStepDialog::Construct(const FArguments& InArgs)
{
	EditedStep       = InArgs._StepData;
	Asset            = InArgs._Asset;
	WeakParentWindow = InArgs._ParentWindow;
	EditColorFrom    = EditedStep.ColorFrom;
	EditColorTo      = EditedStep.ColorTo;

	// ── Shared timing fields ──────────────────────────────────────────────────

	SAssignNew(LabelBox,     SEditableTextBox).Text(FText::FromString(EditedStep.Label));
	SAssignNew(StartTimeBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.StartTime)));
	SAssignNew(DurationBox,  SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Duration)));
	SAssignNew(TimeScaleBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.TimeScale)));
	SAssignNew(LoopsBox,     SEditableTextBox).Text(FText::FromString(FString::FromInt(EditedStep.Loops)));

	// ── EaseType options ──────────────────────────────────────────────────────
	{
		const UEnum* EaseEnum = StaticEnum<EEaseType>();
		for (int32 i = 0; i < EaseEnum->NumEnums() - 1; ++i)
		{
			EaseTypeOptions.Add(MakeShared<FString>(EaseEnum->GetDisplayNameTextByIndex(i).ToString()));
		}
		SelectedEaseIndex = (int32)EditedStep.EaseType;
		SAssignNew(EaseTypeDisplay, STextBlock)
			.Text_Lambda([this]() {
				return EaseTypeOptions.IsValidIndex(SelectedEaseIndex)
					? FText::FromString(*EaseTypeOptions[SelectedEaseIndex])
					: FText::GetEmpty();
			})
			.Font(FAppStyle::GetFontStyle("SmallFont"));
	}

	// ── LoopType options ──────────────────────────────────────────────────────
	{
		LoopTypeOptions.Add(MakeShared<FString>(TEXT("Restart")));
		LoopTypeOptions.Add(MakeShared<FString>(TEXT("Ping Pong")));
		SelectedLoopTypeIndex = (int32)EditedStep.LoopType;
		SAssignNew(LoopTypeDisplay, STextBlock)
			.Text_Lambda([this]() {
				return LoopTypeOptions.IsValidIndex(SelectedLoopTypeIndex)
					? FText::FromString(*LoopTypeOptions[SelectedLoopTypeIndex])
					: FText::GetEmpty();
			})
			.Font(FAppStyle::GetFontStyle("SmallFont"));
	}

	// ── ParameterName ─────────────────────────────────────────────────────────
	SAssignNew(ParameterNameBox, SEditableTextBox)
		.Text(FText::FromName(EditedStep.ParameterName))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.HintText(LOCTEXT("ParamHint", "e.g. Opacity"));

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(420.0f)
		.MaxDesiredHeight(620.0f)
		[
			SNew(SVerticalBox)

			// Title bar
			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 8.0f, 8.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("EditStep", "Edit Step"))
				.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]

			// Scrollable content
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot().Padding(8.0f)
				[
					SNew(SVerticalBox)

					// ── Identity ──────────────────────────────────────────────
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("Label", "Label"), LabelBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("StartTime", "Start Time (s)"), StartTimeBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("Duration", "Duration (s)"), DurationBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("TimeScale", "Time Scale"), TimeScaleBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("Loops", "Loops (-1=∞)"), LoopsBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						MakeLabeledRow(LOCTEXT("EaseType", "Ease Type"),
							SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&EaseTypeOptions)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
							{
								if (Item.IsValid())
									SelectedEaseIndex = EaseTypeOptions.IndexOfByKey(Item);
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
							{
								return SNew(STextBlock).Text(FText::FromString(*Item))
									.Font(FAppStyle::GetFontStyle("SmallFont"));
							})
							.InitiallySelectedItem(EaseTypeOptions.IsValidIndex(SelectedEaseIndex)
								? EaseTypeOptions[SelectedEaseIndex] : nullptr)
							[ EaseTypeDisplay.ToSharedRef() ])
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						MakeLabeledRow(LOCTEXT("LoopType", "Loop Type"),
							SNew(SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&LoopTypeOptions)
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
							{
								if (Item.IsValid())
									SelectedLoopTypeIndex = LoopTypeOptions.IndexOfByKey(Item);
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
							{
								return SNew(STextBlock).Text(FText::FromString(*Item))
									.Font(FAppStyle::GetFontStyle("SmallFont"));
							})
							.InitiallySelectedItem(LoopTypeOptions.IsValidIndex(SelectedLoopTypeIndex)
								? LoopTypeOptions[SelectedLoopTypeIndex] : nullptr)
							[ LoopTypeDisplay.ToSharedRef() ])
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("ParameterName", "Parameter Name"), ParameterNameBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)
					[ SNew(SSeparator) ]

					// ── Type-specific section ─────────────────────────────────
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildTypeSpecificSection() ]
				]
			]

			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]

			// Buttons
			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OK", "OK"))
					.OnClicked(this, &SQTDStepDialog::OnConfirm)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Cancel", "Cancel"))
					.OnClicked(this, &SQTDStepDialog::OnCancel)
				]
			]
		]
	];
}

// ──────────────────────────────────────────────────────────────────────────────
// Type-specific section builder
// ──────────────────────────────────────────────────────────────────────────────

TSharedRef<SWidget> SQTDStepDialog::BuildVectorRow(const FText& Label,
                                                    TSharedPtr<SEditableTextBox>& X,
                                                    TSharedPtr<SEditableTextBox>& Y,
                                                    TSharedPtr<SEditableTextBox>& Z,
                                                    const FVector& Initial)
{
	SAssignNew(X, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(Initial.X)));
	SAssignNew(Y, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(Initial.Y)));
	SAssignNew(Z, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(Initial.Z)));

	return MakeLabeledRow(Label,
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ X.ToSharedRef() ]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Y.ToSharedRef() ]
		+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Z.ToSharedRef() ]
	);
}

TSharedRef<SWidget> SQTDStepDialog::BuildTypeSpecificSection()
{
	auto Box = SNew(SVerticalBox);

	if (EditedStep.StepType == EQTDStepType::Vector)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("VecSection", "Vector Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		if (!EditedStep.bVectorFromCurrent)
		{
			Box->AddSlot().AutoHeight().Padding(0, 4)
			[
				BuildVectorRow(LOCTEXT("VecFrom", "From (X,Y,Z)"),
					VecFromXBox, VecFromYBox, VecFromZBox, EditedStep.VectorFrom)
			];
		}

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			BuildVectorRow(LOCTEXT("VecTo", "To (X,Y,Z)"),
				VecToXBox, VecToYBox, VecToZBox, EditedStep.VectorTo)
		];

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("VecFromCur", "From Current"),
				SNew(SCheckBox)
				.IsChecked(EditedStep.bVectorFromCurrent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
					EditedStep.bVectorFromCurrent = (S == ECheckBoxState::Checked);
				}))
		];
	}
	else if (EditedStep.StepType == EQTDStepType::Rotator)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("RotSection", "Rotator Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		if (!EditedStep.bRotatorFromCurrent)
		{
			Box->AddSlot().AutoHeight().Padding(0, 4)
			[
				BuildVectorRow(LOCTEXT("RotFrom", "From (P,Y,R)"),
					RotFromPitch, RotFromYaw, RotFromRoll,
					FVector(EditedStep.RotatorFrom.Pitch, EditedStep.RotatorFrom.Yaw, EditedStep.RotatorFrom.Roll))
			];
		}

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			BuildVectorRow(LOCTEXT("RotTo", "To (P,Y,R)"),
				RotToPitch, RotToYaw, RotToRoll,
				FVector(EditedStep.RotatorTo.Pitch, EditedStep.RotatorTo.Yaw, EditedStep.RotatorTo.Roll))
		];

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("RotFromCur", "From Current"),
				SNew(SCheckBox)
				.IsChecked(EditedStep.bRotatorFromCurrent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
					EditedStep.bRotatorFromCurrent = (S == ECheckBoxState::Checked);
				}))
		];
	}
	else if (EditedStep.StepType == EQTDStepType::Float)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("FloatSection", "Float Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		SAssignNew(FloatFromBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.FloatFrom)));
		SAssignNew(FloatToBox,   SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.FloatTo)));

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[ MakeLabeledRow(LOCTEXT("FloatFrom", "From"), FloatFromBox.ToSharedRef()) ];
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[ MakeLabeledRow(LOCTEXT("FloatTo", "To"), FloatToBox.ToSharedRef()) ];

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("FloatFromCur", "From Current"),
				SNew(SCheckBox)
				.IsChecked(EditedStep.bFloatFromCurrent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
					EditedStep.bFloatFromCurrent = (S == ECheckBoxState::Checked);
				}))
		];
	}
	else if (EditedStep.StepType == EQTDStepType::LinearColor)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("ColorSection", "Color Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		SAssignNew(ColorFromSwatch, SBorder)
			.BorderBackgroundColor_Lambda([this]() { return EditColorFrom; })
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
				if (E.GetEffectingButton() == EKeys::LeftMouseButton) return LaunchColorPicker(true);
				return FReply::Unhandled();
			});

		SAssignNew(ColorToSwatch, SBorder)
			.BorderBackgroundColor_Lambda([this]() { return EditColorTo; })
			.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
				if (E.GetEffectingButton() == EKeys::LeftMouseButton) return LaunchColorPicker(false);
				return FReply::Unhandled();
			});

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("ColorFrom", "From (click)"),
				SNew(SBox).HeightOverride(20.0f)[ ColorFromSwatch.ToSharedRef() ])
		];
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("ColorTo", "To (click)"),
				SNew(SBox).HeightOverride(20.0f)[ ColorToSwatch.ToSharedRef() ])
		];
	}

	return Box;
}

// ──────────────────────────────────────────────────────────────────────────────
// Color picker
// ──────────────────────────────────────────────────────────────────────────────

FReply SQTDStepDialog::LaunchColorPicker(bool bIsFrom)
{
	FColorPickerArgs Args;
	Args.InitialColor         = bIsFrom ? EditColorFrom : EditColorTo;
	Args.bUseAlpha            = false;
	Args.OnColorCommitted     = FOnLinearColorValueChanged::CreateSP(
		this, &SQTDStepDialog::OnColorPickerCommitted, bIsFrom);

	OpenColorPicker(Args);
	return FReply::Handled();
}

void SQTDStepDialog::OnColorPickerCommitted(FLinearColor NewColor, bool bIsFrom)
{
	if (bIsFrom) EditColorFrom = NewColor;
	else         EditColorTo   = NewColor;
}

// ──────────────────────────────────────────────────────────────────────────────
// Confirm / Cancel
// ──────────────────────────────────────────────────────────────────────────────

bool SQTDStepDialog::CollectValues()
{
	if (LabelBox.IsValid())    EditedStep.Label     = LabelBox->GetText().ToString();
	if (StartTimeBox.IsValid()) EditedStep.StartTime = FMath::Max(0.0f, ParseFloat(StartTimeBox));
	if (DurationBox.IsValid())  EditedStep.Duration  = FMath::Max(0.001f, ParseFloat(DurationBox, 1.0f));
	if (TimeScaleBox.IsValid()) EditedStep.TimeScale = FMath::Max(0.01f, ParseFloat(TimeScaleBox, 1.0f));
	if (LoopsBox.IsValid())     EditedStep.Loops     = ParseInt(LoopsBox, 1);

	EditedStep.EaseType  = (EEaseType)FMath::Clamp(SelectedEaseIndex, 0, EaseTypeOptions.Num() - 1);
	EditedStep.LoopType  = (ELoopType)FMath::Clamp(SelectedLoopTypeIndex, 0, LoopTypeOptions.Num() - 1);
	if (ParameterNameBox.IsValid())
		EditedStep.ParameterName = FName(*ParameterNameBox->GetText().ToString());

	if (EditedStep.StepType == EQTDStepType::Vector)
	{
		EditedStep.VectorFrom = FVector(
			ParseFloat(VecFromXBox), ParseFloat(VecFromYBox), ParseFloat(VecFromZBox));
		EditedStep.VectorTo = FVector(
			ParseFloat(VecToXBox), ParseFloat(VecToYBox), ParseFloat(VecToZBox));
	}
	else if (EditedStep.StepType == EQTDStepType::Rotator)
	{
		EditedStep.RotatorFrom = FRotator(
			ParseFloat(RotFromPitch), ParseFloat(RotFromYaw), ParseFloat(RotFromRoll));
		EditedStep.RotatorTo = FRotator(
			ParseFloat(RotToPitch), ParseFloat(RotToYaw), ParseFloat(RotToRoll));
	}
	else if (EditedStep.StepType == EQTDStepType::Float)
	{
		EditedStep.FloatFrom = ParseFloat(FloatFromBox);
		EditedStep.FloatTo   = ParseFloat(FloatToBox, 1.0f);
	}
	else if (EditedStep.StepType == EQTDStepType::LinearColor)
	{
		EditedStep.ColorFrom = EditColorFrom;
		EditedStep.ColorTo   = EditColorTo;
	}

	return true;
}

FReply SQTDStepDialog::OnConfirm()
{
	if (CollectValues() && Asset)
	{
		Asset->UpdateStep(EditedStep);
	}

	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin())
	{
		Win->RequestDestroyWindow();
	}
	return FReply::Handled();
}

FReply SQTDStepDialog::OnCancel()
{
	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin())
	{
		Win->RequestDestroyWindow();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
