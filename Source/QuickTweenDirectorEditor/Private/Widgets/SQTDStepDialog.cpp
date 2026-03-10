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
#include "PropertyCustomizationHelpers.h"
#include "Curves/CurveFloat.h"

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
	EditUserColor    = EditedStep.UserColor;
	EditEaseCurve    = EditedStep.EaseCurve;

	// ── Shared timing fields ──────────────────────────────────────────────────

	SAssignNew(LabelBox,    SEditableTextBox).Text(FText::FromString(EditedStep.Label));
	SAssignNew(DurationBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Duration)));
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

	// Colored header strip based on step type
	const FLinearColor HeaderAccent = EditedStep.GetTypeColor();

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(430.0f)
		.MaxDesiredHeight(640.0f)
		[
			SNew(SVerticalBox)

			// Colored header bar
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f))
				.Padding(FMargin(0.f))
				[
					SNew(SHorizontalBox)
					// Type-color accent bar
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(4.f)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(HeaderAccent)
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 10.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("EditStep", "Edit Step"))
							.Font(FAppStyle::GetFontStyle("NormalFont"))
							.ColorAndOpacity(FLinearColor(0.90f, 0.90f, 0.90f))
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromString(EditedStep.Label.IsEmpty()
								? UEnum::GetValueAsString(EditedStep.StepType)
								: EditedStep.Label))
							.Font(FAppStyle::GetFontStyle("TinyText"))
							.ColorAndOpacity(HeaderAccent.CopyWithNewOpacity(0.8f))
						]
					]
				]
			]

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
					[ MakeLabeledRow(LOCTEXT("Duration", "Duration (s)"), DurationBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("TimeScale", "Time Scale"), TimeScaleBox.ToSharedRef()) ]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[ MakeLabeledRow(LOCTEXT("Loops", "Loops (min 1)"),
					SNew(SBox)
					.ToolTipText(LOCTEXT("LoopsTip", "Infinite loops are not supported inside a Director sequence. Minimum value is 1."))
					[ LoopsBox.ToSharedRef() ]) ]

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
						MakeLabeledRow(LOCTEXT("EaseCurve", "Ease Curve (override)"),
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UCurveFloat::StaticClass())
							.AllowClear(true)
							.ObjectPath_Lambda([this]() -> FString {
								return EditEaseCurve.IsNull() ? FString() : EditEaseCurve.ToSoftObjectPath().ToString();
							})
							.OnObjectChanged_Lambda([this](const FAssetData& Data) {
								EditEaseCurve = TSoftObjectPtr<UCurveFloat>(Data.GetSoftObjectPath());
							})
							.ToolTipText(LOCTEXT("EaseCurveTip", "Optional UCurveFloat. When set, overrides the Ease Type for this step."))
						)
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

					// ── Color label override ───────────────────────────────────
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
					[
						MakeLabeledRow(LOCTEXT("UserColor", "Step Color (override)"),
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SBox).HeightOverride(20.0f)
								[
									SAssignNew(UserColorSwatch, SBorder)
									.BorderBackgroundColor_Lambda([this]() { return EditUserColor; })
									.OnMouseButtonDown_Lambda([this](const FGeometry&, const FPointerEvent& E) -> FReply {
										if (E.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
										FColorPickerArgs Args;
										Args.InitialColor     = EditUserColor;
										Args.bUseAlpha        = true;
										Args.OnColorCommitted = FOnLinearColorValueChanged::CreateLambda(
											[this](FLinearColor C) { EditUserColor = C; });
										OpenColorPicker(Args);
										return FReply::Handled();
									})
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
							[
								SNew(SButton)
								.ButtonStyle(FAppStyle::Get(), "SimpleButton")
								.ContentPadding(FMargin(4.f, 2.f))
								.ToolTipText(LOCTEXT("ClearUserColor", "Reset to the default type color (set alpha to 0)"))
								.OnClicked_Lambda([this]() -> FReply {
									EditUserColor = FLinearColor(0.f, 0.f, 0.f, 0.f);
									return FReply::Handled();
								})
								[ SNew(STextBlock).Text(LOCTEXT("Reset", "Reset")).Font(FAppStyle::GetFontStyle("TinyText")) ]
							]
						)
					]

					+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 4)
					[ SNew(SSeparator) ]

					// ── Type-specific section ─────────────────────────────────
					+ SVerticalBox::Slot().AutoHeight()
					[ BuildTypeSpecificSection() ]
				]
			]

			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]

			// Buttons
			+ SVerticalBox::Slot().AutoHeight().Padding(8.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(16.f, 5.f))
					.Text(LOCTEXT("Cancel", "Cancel"))
					.TextStyle(FAppStyle::Get(), "SmallText")
					.OnClicked(this, &SQTDStepDialog::OnCancel)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
					.ContentPadding(FMargin(16.f, 5.f))
					.Text(LOCTEXT("OK", "Apply"))
					.TextStyle(FAppStyle::Get(), "SmallText")
					.OnClicked(this, &SQTDStepDialog::OnConfirm)
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
		const bool bIsByMode = (EditedStep.VectorProperty == EQTDVectorProperty::WorldLocationBy
			|| EditedStep.VectorProperty == EQTDVectorProperty::RelativeLocationBy
			|| EditedStep.VectorProperty == EQTDVectorProperty::WorldScale3DBy
			|| EditedStep.VectorProperty == EQTDVectorProperty::RelativeScale3DBy);

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("VecSection", "Vector Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		if (!bIsByMode && !EditedStep.bVectorFromCurrent)
		{
			Box->AddSlot().AutoHeight().Padding(0, 4)
			[
				BuildVectorRow(LOCTEXT("VecFrom", "From (X,Y,Z)"),
					VecFromXBox, VecFromYBox, VecFromZBox, EditedStep.VectorFrom)
			];
		}

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			BuildVectorRow(bIsByMode ? LOCTEXT("VecBy", "By / Delta (X,Y,Z)") : LOCTEXT("VecTo", "To (X,Y,Z)"),
				VecToXBox, VecToYBox, VecToZBox, EditedStep.VectorTo)
		];

		if (!bIsByMode)
		{
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
	}
	else if (EditedStep.StepType == EQTDStepType::Rotator)
	{
		const bool bIsByMode = (EditedStep.RotatorProperty == EQTDRotatorProperty::WorldRotationBy
			|| EditedStep.RotatorProperty == EQTDRotatorProperty::RelativeRotationBy);

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("RotSection", "Rotator Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		if (!bIsByMode && !EditedStep.bRotatorFromCurrent)
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
			BuildVectorRow(bIsByMode ? LOCTEXT("RotBy", "By / Delta (P,Y,R)") : LOCTEXT("RotTo", "To (P,Y,R)"),
				RotToPitch, RotToYaw, RotToRoll,
				FVector(EditedStep.RotatorTo.Pitch, EditedStep.RotatorTo.Yaw, EditedStep.RotatorTo.Roll))
		];

		if (!bIsByMode)
		{
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

	else if (EditedStep.StepType == EQTDStepType::Vector2D)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("Vec2DSection", "Vector2D Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		if (!EditedStep.bVector2DFromCurrent)
		{
			SAssignNew(Vec2DFromXBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Vector2DFrom.X)));
			SAssignNew(Vec2DFromYBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Vector2DFrom.Y)));
			Box->AddSlot().AutoHeight().Padding(0, 4)
			[
				MakeLabeledRow(LOCTEXT("Vec2DFrom", "From (X, Y)"),
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Vec2DFromXBox.ToSharedRef() ]
					+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Vec2DFromYBox.ToSharedRef() ]
				)
			];
		}

		SAssignNew(Vec2DToXBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Vector2DTo.X)));
		SAssignNew(Vec2DToYBox, SEditableTextBox).Text(FText::FromString(FString::SanitizeFloat(EditedStep.Vector2DTo.Y)));
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("Vec2DTo", "To (X, Y)"),
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Vec2DToXBox.ToSharedRef() ]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2, 0)[ Vec2DToYBox.ToSharedRef() ]
			)
		];
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("Vec2DFromCur", "From Current"),
				SNew(SCheckBox)
				.IsChecked(EditedStep.bVector2DFromCurrent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
					EditedStep.bVector2DFromCurrent = (S == ECheckBoxState::Checked);
				}))
		];
	}
	else if (EditedStep.StepType == EQTDStepType::Int)
	{
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			SNew(STextBlock).Text(LOCTEXT("IntSection", "Integer Parameters"))
			.Font(FAppStyle::GetFontStyle("SmallFontBold"))
		];

		SAssignNew(IntFromBox, SEditableTextBox).Text(FText::FromString(FString::FromInt(EditedStep.IntFrom)));
		SAssignNew(IntToBox,   SEditableTextBox).Text(FText::FromString(FString::FromInt(EditedStep.IntTo)));

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[ MakeLabeledRow(LOCTEXT("IntFrom", "From"), IntFromBox.ToSharedRef()) ];
		Box->AddSlot().AutoHeight().Padding(0, 4)
		[ MakeLabeledRow(LOCTEXT("IntTo", "To"), IntToBox.ToSharedRef()) ];

		Box->AddSlot().AutoHeight().Padding(0, 4)
		[
			MakeLabeledRow(LOCTEXT("IntFromCur", "From Current"),
				SNew(SCheckBox)
				.IsChecked(EditedStep.bIntFromCurrent ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
					EditedStep.bIntFromCurrent = (S == ECheckBoxState::Checked);
				}))
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
	if (LabelBox.IsValid())   EditedStep.Label    = LabelBox->GetText().ToString();
	if (DurationBox.IsValid()) EditedStep.Duration = FMath::Max(0.001f, ParseFloat(DurationBox, 1.0f));
	if (TimeScaleBox.IsValid()) EditedStep.TimeScale = FMath::Max(0.01f, ParseFloat(TimeScaleBox, 1.0f));
	if (LoopsBox.IsValid())     EditedStep.Loops     = FMath::Max(1, ParseInt(LoopsBox, 1));

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
	else if (EditedStep.StepType == EQTDStepType::Vector2D)
	{
		if (!EditedStep.bVector2DFromCurrent)
			EditedStep.Vector2DFrom = FVector2D(ParseFloat(Vec2DFromXBox), ParseFloat(Vec2DFromYBox));
		EditedStep.Vector2DTo = FVector2D(ParseFloat(Vec2DToXBox), ParseFloat(Vec2DToYBox));
	}
	else if (EditedStep.StepType == EQTDStepType::Int)
	{
		EditedStep.IntFrom = ParseInt(IntFromBox, 0);
		EditedStep.IntTo   = ParseInt(IntToBox,   1);
	}

	EditedStep.UserColor = EditUserColor;
	EditedStep.EaseCurve = EditEaseCurve;

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
