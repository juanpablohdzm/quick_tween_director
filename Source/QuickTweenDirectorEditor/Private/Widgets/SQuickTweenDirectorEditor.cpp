// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQuickTweenDirectorEditor.h"
#include "QuickTweenDirectorAsset.h"
#include "Widgets/SQTDTrackRow.h"
#include "Widgets/SQTDStepDialog.h"
#include "Widgets/SQTDComponentPickerDialog.h"

#include "Engine/Blueprint.h"

#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/Package.h"
#include "FileHelpers.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SQuickTweenDirectorEditor"

// ──────────────────────────────────────────────────────────────────────────────
// Inline ruler widget
// ──────────────────────────────────────────────────────────────────────────────

class SQTDRuler : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SQTDRuler) {}
		SLATE_ATTRIBUTE(float, PixelsPerSec)
		SLATE_ATTRIBUTE(float, TotalDuration)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		PixelsPerSecAttr  = InArgs._PixelsPerSec;
		TotalDurationAttr = InArgs._TotalDuration;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float PPS = PixelsPerSecAttr.Get();
		const float Dur = TotalDurationAttr.Get();
		return FVector2D(QTDEditorConstants::TrackLabelWidth + PPS * FMath::Max(Dur, 1.0f) + 60.0f,
		                 QTDEditorConstants::RulerHeight);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const float PPS  = PixelsPerSecAttr.Get();
		const float Dur  = TotalDurationAttr.Get();
		const float W    = AllottedGeometry.GetLocalSize().X;
		const float H    = AllottedGeometry.GetLocalSize().Y;
		const float OffX = QTDEditorConstants::TrackLabelWidth;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppStyle::GetBrush("ToolPanel.DarkGroupBorder"),
			ESlateDrawEffect::None,
			FLinearColor(0.08f, 0.08f, 0.08f));
		++LayerId;

		float MajorStep = 1.0f;
		if      (PPS >= 200.0f) MajorStep = 0.25f;
		else if (PPS >= 100.0f) MajorStep = 0.5f;
		const float MinorStep = MajorStep / 5.0f;

		for (float T = 0.0f; T <= Dur + MajorStep; T += MinorStep)
		{
			const float X = OffX + T * PPS;
			if (X > W) break;
			TArray<FVector2D> Pts = { FVector2D(X, H * 0.7f), FVector2D(X, H) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				Pts, ESlateDrawEffect::None, FLinearColor(0.35f, 0.35f, 0.35f), true, 1.0f);
		}

		for (float T = 0.0f; T <= Dur + MajorStep; T += MajorStep)
		{
			const float X = OffX + T * PPS;
			if (X > W) break;

			TArray<FVector2D> Pts = { FVector2D(X, H * 0.3f), FVector2D(X, H) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				Pts, ESlateDrawEffect::None, FLinearColor(0.65f, 0.65f, 0.65f), true, 1.5f);

			FSlateDrawElement::MakeText(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(60.0f, H), FSlateLayoutTransform(FVector2f(X + 3.0f, 2.0f))),
				FText::FromString(FString::Printf(TEXT("%.2fs"), T)),
				FAppStyle::GetFontStyle("TinyText"),
				ESlateDrawEffect::None,
				FLinearColor(0.75f, 0.75f, 0.75f));
		}

		return LayerId;
	}

private:
	TAttribute<float> PixelsPerSecAttr;
	TAttribute<float> TotalDurationAttr;
};

// ──────────────────────────────────────────────────────────────────────────────
// SQuickTweenDirectorEditor
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::Construct(const FArguments& InArgs)
{
	Asset     = InArgs._Asset;
	Blueprint = InArgs._Blueprint;

	auto Toolbar = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("Save", "Save"))
			.ToolTipText(LOCTEXT("SaveTip", "Save the director asset"))
			.OnClicked(this, &SQuickTweenDirectorEditor::OnSaveClicked)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("AddTrack", "+ Add Track"))
			.ToolTipText(LOCTEXT("AddTrackTip", "Pick a component to animate"))
			.OnClicked(this, &SQuickTweenDirectorEditor::OnAddTrackClicked)
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNew(SSpacer) ]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText {
				const float D = Asset ? Asset->GetTotalDuration() : 0.0f;
				return FText::FromString(FString::Printf(TEXT("Duration: %.2fs"), D));
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("Zoom", "  Zoom:"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(SBox).WidthOverride(120.0f)
			[
				SNew(SSlider)
				.Value_Raw(this, &SQuickTweenDirectorEditor::OnGetZoomValue)
				.OnValueChanged(this, &SQuickTweenDirectorEditor::OnZoomChanged)
				.MinValue(20.0f)
				.MaxValue(400.0f)
				.ToolTipText(LOCTEXT("ZoomTip", "Pixels per second"))
			]
		];

	SAssignNew(TrackContainer, SVerticalBox);

	TSharedRef<SQTDRuler> Ruler = SNew(SQTDRuler)
		.PixelsPerSec_Lambda([this]() { return PixelsPerSec; })
		.TotalDuration_Lambda([this]() { return Asset ? Asset->GetTotalDuration() : 1.0f; });
	RulerWidget = Ruler;

	// Shared horizontal scrollbar — linked to the ruler and all track rows.
	SAssignNew(HScrollBar, SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(6.0f, 6.0f));

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(2.0f)[ Toolbar ]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SScrollBox)
			.Orientation(Orient_Horizontal)
			.ScrollBarVisibility(EVisibility::Collapsed)
			.ExternalScrollbar(HScrollBar)
			+ SScrollBox::Slot()[ Ruler ]
		]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			.Orientation(Orient_Vertical)
			+ SScrollBox::Slot()[ TrackContainer.ToSharedRef() ]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			HScrollBar.ToSharedRef()
		]
	];

	RefreshFromAsset();
}

// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::RefreshFromAsset()
{
	if (!TrackContainer) return;
	TrackContainer->ClearChildren();
	if (!Asset) return;

	for (const FQTDTrackData& Track : Asset->Tracks)
	{
		TrackContainer->AddSlot().AutoHeight()
		[
			SNew(SQTDTrackRow)
			.Track(Track)
			.Asset(Asset)
			.PixelsPerSec(PixelsPerSec)
			.HScrollBar(HScrollBar)
			.OnTrackDelete_Lambda([this](FGuid Id) { DeleteTrack(Id); })
			.OnStepAdded_Lambda([this](FQTDStepData S) { AddStepToTrack(S); })
			.OnStepEdit_Lambda([this](FQTDStepData S) { OpenStepDialog(S); })
			.OnStepMoved_Lambda([this](FGuid StepId, float NewT) {
				if (!Asset) return;
				if (FQTDStepData* Step = Asset->FindStep(StepId))
				{
					Step->StartTime = FMath::Max(0.0f, NewT);
					Asset->MarkPackageDirty();
				}
			})
			.OnStepDeleted_Lambda([this](FGuid StepId) {
				if (Asset) { Asset->RemoveStep(StepId); RefreshFromAsset(); }
			})
		];
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Toolbar actions
// ──────────────────────────────────────────────────────────────────────────────

FReply SQuickTweenDirectorEditor::OnSaveClicked()
{
	if (Asset)
	{
		TArray<UPackage*> Packages;
		Packages.Add(Asset->GetPackage());
		FEditorFileUtils::PromptForCheckoutAndSave(Packages, false, false);
	}
	return FReply::Handled();
}

FReply SQuickTweenDirectorEditor::OnAddTrackClicked()
{
	if (!Asset) return FReply::Handled();

	UBlueprint* BP = Blueprint.Get();
	if (!BP)
	{
		// No Blueprint context: add a generic unnamed track
		Asset->AddTrack(TEXT("Track"), EQTDStepType::Vector);
		RefreshFromAsset();
		return FReply::Handled();
	}

	// Open component picker dialog
	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("PickerTitle", "Pick Component to Animate"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SQTDComponentPickerDialog> Picker = SNew(SQTDComponentPickerDialog)
		.Blueprint(BP)
		.ParentWindow(PickerWindow);

	PickerWindow->SetContent(Picker);
	FSlateApplication::Get().AddModalWindow(PickerWindow, AsShared());

	const SQTDComponentPickerDialog::FResult& R = Picker->GetResult();
	if (!R.bConfirmed) return FReply::Handled();

	// Create a track pre-wired to the chosen component
	FQTDTrackData NewTrack;
	NewTrack.TrackId             = FGuid::NewGuid();
	NewTrack.TrackLabel          = R.DisplayName;
	NewTrack.ComponentVariableName = R.VariableName;
	NewTrack.ComponentClass       = R.ComponentClass;

	// Default step type based on component class
	if (R.ComponentClass && R.ComponentClass->IsChildOf<USceneComponent>())
		NewTrack.DefaultStepType = EQTDStepType::Vector;
	else
		NewTrack.DefaultStepType = EQTDStepType::Float;

	Asset->Tracks.Add(NewTrack);
	Asset->MarkPackageDirty();
	RefreshFromAsset();

	return FReply::Handled();
}

// ──────────────────────────────────────────────────────────────────────────────
// Track / step actions
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::DeleteTrack(FGuid TrackId)
{
	if (Asset) { Asset->RemoveTrack(TrackId); RefreshFromAsset(); }
}

void SQuickTweenDirectorEditor::AddStepToTrack(FQTDStepData PrefilledStep)
{
	if (!Asset) return;

	// Label defaults to the step type name
	if (PrefilledStep.Label.IsEmpty())
	{
		PrefilledStep.Label = UEnum::GetValueAsString(PrefilledStep.StepType);
	}

	Asset->AddStep(PrefilledStep);
	OpenStepDialog(PrefilledStep);
	RefreshFromAsset();
}

void SQuickTweenDirectorEditor::OpenStepDialog(const FQTDStepData& Step)
{
	if (!Asset) return;

	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("StepDialogTitle", "Edit Step"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SQTDStepDialog> Dialog = SNew(SQTDStepDialog)
		.StepData(Step)
		.Asset(Asset)
		.ParentWindow(DialogWindow);

	DialogWindow->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(DialogWindow, AsShared());
	RefreshFromAsset();
}

// ──────────────────────────────────────────────────────────────────────────────
// Zoom
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::OnZoomChanged(float NewValue)
{
	PixelsPerSec = NewValue;
	RefreshFromAsset();
}

#undef LOCTEXT_NAMESPACE
