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
		// No label offset — the ruler scrollbox is positioned after the label column.
		return FVector2D(PPS * FMath::Max(Dur, 1.0f) + 60.0f,
		                 QTDEditorConstants::RulerHeight);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		const float PPS = PixelsPerSecAttr.Get();
		const float Dur = TotalDurationAttr.Get();
		const float W   = AllottedGeometry.GetLocalSize().X;
		const float H   = AllottedGeometry.GetLocalSize().Y;

		// Dark ruler background
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppStyle::GetBrush("WhiteBrush"),
			ESlateDrawEffect::None,
			FLinearColor(0.013f, 0.013f, 0.013f));
		++LayerId;

		// Bottom edge separator
		TArray<FVector2D> BotEdge = { FVector2D(0.f, H - 1.f), FVector2D(W, H - 1.f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(), BotEdge,
			ESlateDrawEffect::None, FLinearColor(0.20f, 0.20f, 0.20f), true, 1.f);
		++LayerId;

		// Pick a major step that keeps labels at least ~70 px apart.
		static const float NiceSteps[] = { 0.05f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 30.0f, 60.0f };
		float MajorStep = NiceSteps[4];
		if (PPS > 0.0f)
		{
			const float RawStep = 70.f / PPS;
			MajorStep = NiceSteps[UE_ARRAY_COUNT(NiceSteps) - 1];
			for (float S : NiceSteps) { if (S >= RawStep) { MajorStep = S; break; } }
		}
		const float MinorStep = MajorStep / 5.0f;

		// Minor ticks
		for (float T = 0.0f; T <= Dur + MajorStep; T += MinorStep)
		{
			const float X = T * PPS;
			if (X > W) break;
			TArray<FVector2D> Pts = { FVector2D(X, H * 0.72f), FVector2D(X, H - 1.f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				Pts, ESlateDrawEffect::None, FLinearColor(0.26f, 0.26f, 0.26f), true, 1.0f);
		}

		// Major ticks + labels
		for (float T = 0.0f; T <= Dur + MajorStep; T += MajorStep)
		{
			const float X = T * PPS;
			if (X > W) break;

			// Accent tick mark
			TArray<FVector2D> Pts = { FVector2D(X, H * 0.30f), FVector2D(X, H - 1.f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				Pts, ESlateDrawEffect::None, FLinearColor(1.0f, 0.55f, 0.15f, 0.80f), true, 1.5f);

			// Time label — position text slightly above the tick bottom
			const FString TimeStr = (MajorStep >= 1.0f)
				? FString::Printf(TEXT("%.0fs"), T)
				: FString::Printf(TEXT("%.2fs"), T);
			FSlateDrawElement::MakeText(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(60.f, H * 0.55f),
					FSlateLayoutTransform(FVector2f(X + 4.f, 3.f))),
				FText::FromString(TimeStr),
				FAppStyle::GetFontStyle("TinyText"),
				ESlateDrawEffect::None,
				FLinearColor(0.70f, 0.70f, 0.70f));
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

	auto Toolbar =
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
		.Padding(FMargin(4.f, 0.f))
		[
			SNew(SBox).HeightOverride(QTDEditorConstants::ToolbarHeight)
			[
				SNew(SHorizontalBox)

				// ── Save ─────────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(8.f, 4.f))
					.ToolTipText(LOCTEXT("SaveTip", "Save the director asset to disk"))
					.OnClicked(this, &SQuickTweenDirectorEditor::OnSaveClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Save"))
							.DesiredSizeOverride(FVector2D(14.f, 14.f))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f)))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Save", "Save"))
							.Font(FAppStyle::GetFontStyle("SmallFont"))
							.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f))
						]
					]
				]

				// ── Divider ───────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(4.f, 4.f)
				[
					SNew(SSeparator).Orientation(Orient_Vertical)
					.SeparatorImage(FAppStyle::GetBrush("WhiteBrush"))
					.Thickness(1.f)
					.ColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f))
				]

				// ── Add Track ─────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(8.f, 4.f))
					.ToolTipText(LOCTEXT("AddTrackTip", "Add a track bound to a Blueprint component"))
					.OnClicked(this, &SQuickTweenDirectorEditor::OnAddTrackClicked)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.Plus"))
							.DesiredSizeOverride(FVector2D(12.f, 12.f))
							.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.55f, 0.15f)))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AddTrack", "Add Track"))
							.Font(FAppStyle::GetFontStyle("SmallFont"))
							.ColorAndOpacity(FLinearColor(1.0f, 0.55f, 0.15f))
						]
					]
				]

				+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(SSpacer) ]

				// ── Duration badge ────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
					.Padding(FMargin(8.f, 3.f))
					[
						SNew(STextBlock)
						.Text_Lambda([this]() -> FText {
							const float D = Asset ? Asset->GetTotalDuration() : 0.0f;
							return FText::FromString(FString::Printf(TEXT("%.2f s"), D));
						})
						.Font(FAppStyle::GetFontStyle("SmallFont"))
						.ColorAndOpacity(FLinearColor(0.60f, 0.60f, 0.60f))
					]
				]

				// ── Divider ───────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(4.f, 4.f)
				[
					SNew(SSeparator).Orientation(Orient_Vertical)
					.SeparatorImage(FAppStyle::GetBrush("WhiteBrush"))
					.Thickness(1.f)
					.ColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f))
				]

				// ── Zoom ──────────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Search"))
					.DesiredSizeOverride(FVector2D(12.f, 12.f))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f)))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)
				[
					SNew(SBox).WidthOverride(100.f)
					[
						SNew(SSlider)
						.Value_Raw(this, &SQuickTweenDirectorEditor::OnGetZoomValue)
						.OnValueChanged(this, &SQuickTweenDirectorEditor::OnZoomChanged)
						.MinValue(20.0f)
						.MaxValue(400.0f)
						.ToolTipText(LOCTEXT("ZoomTip", "Zoom: pixels per second"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText {
						return FText::FromString(FString::Printf(TEXT("%.0f%%"),
							(PixelsPerSec / QTDEditorConstants::DefaultPixelsPerSec) * 100.f));
					})
					.Font(FAppStyle::GetFontStyle("TinyText"))
					.ColorAndOpacity(FLinearColor(0.40f, 0.40f, 0.40f))
				]
			]
		];

	// ── Shared widgets created once, populated by RefreshFromAsset ───────────

	// Ruler — lives inside ContentContainer, which is inside the ONE H scroll box.
	TSharedRef<SQTDRuler> Ruler = SNew(SQTDRuler)
		.PixelsPerSec_Lambda([this]() { return PixelsPerSec; })
		.TotalDuration_Lambda([this]() { return Asset ? Asset->GetTotalDuration() : 1.0f; });
	RulerWidget = Ruler;

	// Label column (track labels, rebuilt each RefreshFromAsset).
	SAssignNew(LabelContainer, SVerticalBox);

	// Content column (ruler + step content rows, rebuilt each RefreshFromAsset).
	// All content lives inside ONE SScrollBox(H) so they scroll together perfectly.
	SAssignNew(ContentContainer, SVerticalBox);

	// The single external horizontal scrollbar.
	TSharedRef<SScrollBar> HBar = SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(6.0f, 6.0f));
	HScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.ExternalScrollbar(HBar);
	HScrollBox->AddSlot()[ ContentContainer.ToSharedRef() ];

	// ── Final layout ─────────────────────────────────────────────────────────
	//
	//  Toolbar  |  Separator
	//  ─────────────────────────────────────────────────────────────────────
	//  SScrollBox(V)          ← vertical scroll for many tracks
	//    SHorizontalBox
	//      [200px label col]     [SScrollBox(H, ExternalBar=HBar)]
	//        ruler label           ContentContainer (SVerticalBox)
	//        sep                     SQTDRuler
	//        LabelContainer          sep
	//          label rows            step content rows...
	//  HBar                   ← horizontal scrollbar, always visible
	//

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Toolbar ]

		// Main area — vertical scroll wraps labels + content together
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
			.Padding(FMargin(0.f))
			[
			SNew(SScrollBox)
			.Orientation(Orient_Vertical)
			.ScrollBarVisibility(EVisibility::Visible)
			+ SScrollBox::Slot()
			[
				SNew(SHorizontalBox)

				// ── Label column: fixed 200 px, no horizontal scroll ──────────
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SVerticalBox)

					// Ruler-label header
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBox)
						.WidthOverride(QTDEditorConstants::TrackLabelWidth)
						.HeightOverride(QTDEditorConstants::RulerHeight)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f))
							.Padding(FMargin(0.f))
							[
								SNew(SHorizontalBox)
								// Orange accent bar on the left
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox).WidthOverride(QTDEditorConstants::LabelAccentWidth)
									[
										SNew(SBorder)
										.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
										.BorderBackgroundColor(FLinearColor(1.0f, 0.55f, 0.15f))
									]
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 0.f)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("RulerLabel", "TIMELINE"))
									.Font(FAppStyle::GetFontStyle("TinyText"))
									.ColorAndOpacity(FLinearColor(0.45f, 0.45f, 0.45f))
								]
							]
						]
					]
					// Track label rows
					+ SVerticalBox::Slot().AutoHeight()[ LabelContainer.ToSharedRef() ]
				]

				// ── Content column: ONE H scroll box for ruler + all step rows ─
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					HScrollBox.ToSharedRef()
				]
			]
			]  // SBorder (dark bg)
		]

		// Horizontal scrollbar — pinned below everything, always visible
		+ SVerticalBox::Slot().AutoHeight()[ HBar ]
	];

	RefreshFromAsset();
}

// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::RefreshFromAsset()
{
	if (!LabelContainer || !ContentContainer) return;

	LabelContainer->ClearChildren();
	ContentContainer->ClearChildren();

	// Ruler is always at the top of the content column
	if (RulerWidget.IsValid())
	{
		ContentContainer->AddSlot().AutoHeight()[ RulerWidget.ToSharedRef() ];
		ContentContainer->AddSlot().AutoHeight()[ SNew(SSeparator) ];
		// Force ruler to repaint with current zoom
		RulerWidget->Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
	}

	if (!Asset) return;

	for (const FQTDTrackData& Track : Asset->Tracks)
	{
		TSharedRef<SQTDTrackRow> Row =
			SNew(SQTDTrackRow)
			.Track(Track)
			.Asset(Asset)
			.PixelsPerSec(PixelsPerSec)
			.IsSelected(SelectedTrackId == Track.TrackId)
			.OnTrackSelected_Lambda([this, TrackId = Track.TrackId]() {
				SelectedTrackId = TrackId;
				RefreshFromAsset();
			})
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
			});

		// Label goes in the fixed label column (this widget IS the label — SQTDTrackRow shows only the label).
		LabelContainer->AddSlot().AutoHeight()[ Row ];

		// Step content goes inside ContentContainer — same H scroll box as the ruler.
		ContentContainer->AddSlot().AutoHeight()[ Row->GetStepContentWidget() ];
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
	// Invalidate the ruler so it repaints with the new zoom and recalculates its desired size.
	if (RulerWidget.IsValid())
	{
		RulerWidget->Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
	}
	RefreshFromAsset();
}

#undef LOCTEXT_NAMESPACE
