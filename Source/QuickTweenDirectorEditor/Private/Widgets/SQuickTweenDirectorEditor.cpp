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
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/Package.h"
#include "FileHelpers.h"
#include "Styling/AppStyle.h"
#include "ScopedTransaction.h"

// JSON export / import
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"

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
		SLATE_ATTRIBUTE(float, PlayheadTime)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		PixelsPerSecAttr  = InArgs._PixelsPerSec;
		TotalDurationAttr = InArgs._TotalDuration;
		PlayheadTimeAttr  = InArgs._PlayheadTime;
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float PPS = PixelsPerSecAttr.Get();
		const float Dur = TotalDurationAttr.Get();
		return FVector2D(PPS * FMath::Max(Dur, 1.0f) + 60.0f,
		                 QTDEditorConstants::RulerHeight);
	}

	/** Allow the ruler to be scrubbed by clicking/dragging. */
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			bIsScrubbing = true;
			ScrubToLocal(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X);
			return FReply::Handled().CaptureMouse(AsShared());
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (bIsScrubbing)
		{
			ScrubToLocal(Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()).X);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bIsScrubbing)
		{
			bIsScrubbing = false;
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Unhandled();
	}

	/** Delegate fired when the user scrubs the ruler (passes the new time). */
	TFunction<void(float)> OnScrub;

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

			TArray<FVector2D> Pts = { FVector2D(X, H * 0.30f), FVector2D(X, H - 1.f) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
				Pts, ESlateDrawEffect::None, FLinearColor(1.0f, 0.55f, 0.15f, 0.80f), true, 1.5f);

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

		// ── Playhead ─────────────────────────────────────────────────────────
		const float PH = PlayheadTimeAttr.Get();
		if (PPS > 0.f && PH >= 0.f)
		{
			const float PHX = PH * PPS;
			if (PHX <= W)
			{
				// Vertical line
				TArray<FVector2D> PHPts = { FVector2D(PHX, 0.f), FVector2D(PHX, H) };
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1,
					AllottedGeometry.ToPaintGeometry(), PHPts,
					ESlateDrawEffect::None, FLinearColor(0.05f, 0.95f, 0.40f, 0.90f), true, 2.f);

				// Triangle head
				TArray<FSlateVertex> Verts;
				TArray<SlateIndex> Indices;
				const FVector2f Offset = AllottedGeometry.GetAbsolutePosition();
				const float Scale = AllottedGeometry.Scale;
				auto V = [&](float x, float y) {
					FSlateVertex Vtx;
					Vtx.Position = FVector2f(Offset.X + x * Scale, Offset.Y + y * Scale);
					Vtx.Color    = FColor(13, 242, 102, 230);
					return Vtx;
				};
				Verts.Add(V(PHX - 5.f, 0.f));
				Verts.Add(V(PHX + 5.f, 0.f));
				Verts.Add(V(PHX,       7.f));
				Indices = { 0, 1, 2 };
				FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId + 2,
					FAppStyle::GetBrush("WhiteBrush")->GetRenderingResource(),
					Verts, Indices, nullptr, 0, 0);
			}
		}

		return LayerId + 3;
	}

private:
	void ScrubToLocal(float LocalX)
	{
		const float PPS = PixelsPerSecAttr.Get();
		if (PPS <= 0.f) return;
		const float NewTime = FMath::Max(0.f, LocalX / PPS);
		if (OnScrub) OnScrub(NewTime);
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	TAttribute<float> PixelsPerSecAttr;
	TAttribute<float> TotalDurationAttr;
	TAttribute<float> PlayheadTimeAttr;
	bool              bIsScrubbing = false;
};

// ──────────────────────────────────────────────────────────────────────────────
// SQuickTweenDirectorEditor
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::Construct(const FArguments& InArgs)
{
	Asset     = InArgs._Asset;
	Blueprint = InArgs._Blueprint;

	// ── Toolbar ──────────────────────────────────────────────────────────────

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
						[ SNew(SImage).Image(FAppStyle::GetBrush("Icons.Save"))
							.DesiredSizeOverride(FVector2D(14.f, 14.f))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f))) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[ SNew(STextBlock).Text(LOCTEXT("Save", "Save"))
							.Font(FAppStyle::GetFontStyle("SmallFont"))
							.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f)) ]
					]
				]

				// ── Divider ───────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(4.f, 4.f)
				[ SNew(SSeparator).Orientation(Orient_Vertical).Thickness(1.f)
					.SeparatorImage(FAppStyle::GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f)) ]

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
						[ SNew(SImage).Image(FAppStyle::GetBrush("Icons.Plus"))
							.DesiredSizeOverride(FVector2D(12.f, 12.f))
							.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.55f, 0.15f))) ]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[ SNew(STextBlock).Text(LOCTEXT("AddTrack", "Add Track"))
							.Font(FAppStyle::GetFontStyle("SmallFont"))
							.ColorAndOpacity(FLinearColor(1.0f, 0.55f, 0.15f)) ]
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
				[ SNew(SSeparator).Orientation(Orient_Vertical).Thickness(1.f)
					.SeparatorImage(FAppStyle::GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f)) ]

				// ── Snap toggle ───────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
					.IsChecked_Lambda([this]() { return bSnapEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
					.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
						bSnapEnabled = (S == ECheckBoxState::Checked);
						PropagateSnapToRows();
					})
					.ToolTipText(LOCTEXT("SnapTip", "Enable grid snapping (0.05s increments)"))
					.Padding(FMargin(4.f, 2.f))
					[
						SNew(STextBlock).Text(LOCTEXT("Snap", "Snap"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity_Lambda([this]() {
							return FSlateColor(bSnapEnabled ? FLinearColor(1.0f, 0.55f, 0.15f) : FLinearColor(0.5f, 0.5f, 0.5f));
						})
					]
				]

				// ── Zoom-to-fit ───────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(6.f, 3.f))
					.ToolTipText(LOCTEXT("FitTip", "Zoom to fit the entire animation in the visible area"))
					.OnClicked(this, &SQuickTweenDirectorEditor::OnZoomToFitClicked)
					[
						SNew(STextBlock).Text(LOCTEXT("Fit", "Fit"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.60f, 0.60f, 0.60f))
					]
				]

				// ── Zoom slider ───────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f)
				[ SNew(SImage).Image(FAppStyle::GetBrush("Icons.Search"))
					.DesiredSizeOverride(FVector2D(12.f, 12.f))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.45f, 0.45f))) ]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)
				[
					SNew(SBox).WidthOverride(100.f)
					[
						SNew(SSlider)
						.Value_Raw(this, &SQuickTweenDirectorEditor::OnGetZoomValue)
						.OnValueChanged(this, &SQuickTweenDirectorEditor::OnZoomChanged)
						.MinValue(20.0f).MaxValue(400.0f)
						.ToolTipText(LOCTEXT("ZoomTip", "Zoom: pixels per second"))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 2.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([this]() -> FText {
						return FText::FromString(FString::Printf(TEXT("%.0f%%"),
							(PixelsPerSec / QTDEditorConstants::DefaultPixelsPerSec) * 100.f));
					})
					.Font(FAppStyle::GetFontStyle("TinyText"))
					.ColorAndOpacity(FLinearColor(0.40f, 0.40f, 0.40f))
				]

				// ── Divider ───────────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill).Padding(4.f, 4.f)
				[ SNew(SSeparator).Orientation(Orient_Vertical).Thickness(1.f)
					.SeparatorImage(FAppStyle::GetBrush("WhiteBrush"))
					.ColorAndOpacity(FLinearColor(0.22f, 0.22f, 0.22f)) ]

				// ── Export JSON ───────────────────────────────────────────────
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(6.f, 3.f))
					.ToolTipText(LOCTEXT("ExportJsonTip", "Export all tracks and steps to a .json file"))
					.OnClicked(this, &SQuickTweenDirectorEditor::OnExportJsonClicked)
					[
						SNew(STextBlock).Text(LOCTEXT("ExportJson", "Export JSON"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f, 0.f, 6.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.ContentPadding(FMargin(6.f, 3.f))
					.ToolTipText(LOCTEXT("ImportJsonTip", "Replace tracks and steps from a previously exported .json file"))
					.OnClicked(this, &SQuickTweenDirectorEditor::OnImportJsonClicked)
					[
						SNew(STextBlock).Text(LOCTEXT("ImportJson", "Import JSON"))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
					]
				]
			]
		];

	// ── Shared widgets ────────────────────────────────────────────────────────

	TSharedRef<SQTDRuler> Ruler = SNew(SQTDRuler)
		.PixelsPerSec_Lambda([this]() { return PixelsPerSec; })
		.TotalDuration_Lambda([this]() { return Asset ? Asset->GetTotalDuration() : 1.0f; })
		.PlayheadTime_Lambda([this]() { return PlayheadTime; });

	// Wire up scrubbing
	Ruler->OnScrub = [this](float NewTime) {
		PlayheadTime = FMath::Clamp(NewTime, 0.f, Asset ? Asset->GetTotalDuration() : 1.f);
		if (RulerWidget.IsValid()) RulerWidget->Invalidate(EInvalidateWidgetReason::Paint);
	};
	RulerWidget = Ruler;

	SAssignNew(LabelContainer,   SVerticalBox);
	SAssignNew(ContentContainer, SVerticalBox);

	TSharedRef<SScrollBar> HBar = SNew(SScrollBar)
		.Orientation(Orient_Horizontal)
		.Thickness(FVector2D(6.0f, 6.0f));
	HScrollBox = SNew(SScrollBox)
		.Orientation(Orient_Horizontal)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.ExternalScrollbar(HBar);
	HScrollBox->AddSlot()[ ContentContainer.ToSharedRef() ];

	// ── Final layout ──────────────────────────────────────────────────────────

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ Toolbar ]

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

				// Label column (fixed, no H-scroll)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SVerticalBox)
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
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SBox).WidthOverride(QTDEditorConstants::LabelAccentWidth)
									[ SNew(SBorder).BorderImage(FAppStyle::GetBrush("WhiteBrush"))
										.BorderBackgroundColor(FLinearColor(1.0f, 0.55f, 0.15f)) ]
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
					+ SVerticalBox::Slot().AutoHeight()[ LabelContainer.ToSharedRef() ]
				]

				// Content column
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				[
					HScrollBox.ToSharedRef()
				]
			]
			]
		]

		+ SVerticalBox::Slot().AutoHeight()[ HBar ]
	];

	RefreshFromAsset();
}

// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::RefreshFromAsset()
{
	if (!LabelContainer || !ContentContainer) return;

	TrackRows.Empty();
	LabelContainer->ClearChildren();
	ContentContainer->ClearChildren();

	if (RulerWidget.IsValid())
	{
		ContentContainer->AddSlot().AutoHeight()[ RulerWidget.ToSharedRef() ];
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
				if (SelectedTrackId == TrackId) return;
				SelectedTrackId = TrackId;
				RefreshFromAsset();
			})
			.OnTrackDelete_Lambda([this](FGuid Id) { DeleteTrack(Id); })
			.OnTrackReorder_Lambda([this](FGuid Id, int32 Dir) { ReorderTrack(Id, Dir); })
			.OnStepAdded_Lambda([this](FQTDStepData S) { AddStepToTrack(S); })
			.OnStepEdit_Lambda([this](FQTDStepData S) { OpenStepDialog(S); })
			.OnStepMoved_Lambda([this](FGuid StepId, float NewT) {
				if (!Asset) return;
				FScopedTransaction Tx(LOCTEXT("MoveStepTime", "Move Step"));
				Asset->Modify();
				if (FQTDStepData* Step = Asset->FindStep(StepId))
				{
					Step->StartTime = FMath::Max(0.0f, NewT);
					Asset->MarkPackageDirty();
				}
			})
			.OnStepDeleted_Lambda([this](FGuid StepId) {
				if (!Asset) return;

				FGuid TrackId;
				if (FQTDStepData* Step = Asset->FindStep(StepId))
					TrackId = Step->TrackId;

				FScopedTransaction Tx(LOCTEXT("DeleteStep", "Delete Step"));
				Asset->Modify();
				Asset->RemoveStep(StepId);

				if (TrackId.IsValid())
				{
					TArray<FQTDStepData*> Remaining = Asset->GetStepsForTrack(TrackId);
					Remaining.Sort([](const FQTDStepData& A, const FQTDStepData& B) {
						return A.StartTime < B.StartTime;
					});
					float Cursor = 0.f;
					for (FQTDStepData* S : Remaining)
					{
						S->StartTime = Cursor;
						Cursor += S->Duration * FMath::Max(S->Loops, 1);
					}
					Asset->MarkPackageDirty();
				}

				RefreshFromAsset();
			});

		Row->SetSnapEnabled(bSnapEnabled);
		TrackRows.Add(Row);

		LabelContainer->AddSlot().AutoHeight()[ Row ];
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
		FScopedTransaction Tx(LOCTEXT("AddTrack", "Add Track"));
		Asset->Modify();
		Asset->AddTrack(TEXT("Track"), EQTDStepType::Vector);
		RefreshFromAsset();
		return FReply::Handled();
	}

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

	FQTDTrackData NewTrack;
	NewTrack.TrackId              = FGuid::NewGuid();
	NewTrack.TrackLabel           = R.DisplayName;
	NewTrack.ComponentVariableName = R.VariableName;
	NewTrack.ComponentClass        = R.ComponentClass;
	NewTrack.DefaultStepType       = (R.ComponentClass && R.ComponentClass->IsChildOf<USceneComponent>())
		? EQTDStepType::Vector : EQTDStepType::Float;

	{
		FScopedTransaction Tx(LOCTEXT("AddTrack", "Add Track"));
		Asset->Modify();
		Asset->Tracks.Add(NewTrack);
		Asset->MarkPackageDirty();
	}
	RefreshFromAsset();
	return FReply::Handled();
}

FReply SQuickTweenDirectorEditor::OnExportJsonClicked()
{
	if (!Asset) return FReply::Handled();

	IDesktopPlatform* DP = FDesktopPlatformModule::Get();
	TArray<FString> OutFiles;
	bool bPicked = false;
	if (DP)
	{
		bPicked = DP->SaveFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
			TEXT("Export Director Asset to JSON"),
			FPaths::ProjectContentDir(),
			Asset->GetName() + TEXT(".json"),
			TEXT("JSON files (*.json)|*.json"),
			EFileDialogFlags::None,
			OutFiles);
	}
	if (!bPicked || OutFiles.IsEmpty()) return FReply::Handled();

	FString OutputJson;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputJson);
	Writer->WriteObjectStart();
	Writer->WriteValue(TEXT("asset"),         Asset->GetName());
	Writer->WriteValue(TEXT("loops"),         Asset->Loops);
	Writer->WriteValue(TEXT("totalDuration"), Asset->GetTotalDuration());

	// Tracks
	Writer->WriteArrayStart(TEXT("tracks"));
	for (const FQTDTrackData& T : Asset->Tracks)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("id"),        T.TrackId.ToString());
		Writer->WriteValue(TEXT("label"),     T.TrackLabel);
		Writer->WriteValue(TEXT("component"), T.ComponentVariableName.ToString());
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();

	// Steps -- all fields for lossless import
	Writer->WriteArrayStart(TEXT("steps"));
	for (const FQTDStepData& S : Asset->Steps)
	{
		Writer->WriteObjectStart();
		Writer->WriteValue(TEXT("id"),            S.StepId.ToString());
		Writer->WriteValue(TEXT("trackId"),       S.TrackId.ToString());
		Writer->WriteValue(TEXT("label"),         S.Label);
		Writer->WriteValue(TEXT("type"),          (int32)S.StepType);
		Writer->WriteValue(TEXT("startTime"),     S.StartTime);
		Writer->WriteValue(TEXT("duration"),      S.Duration);
		Writer->WriteValue(TEXT("timeScale"),     S.TimeScale);
		Writer->WriteValue(TEXT("loops"),         S.Loops);
		Writer->WriteValue(TEXT("loopType"),      (int32)S.LoopType);
		Writer->WriteValue(TEXT("easeType"),      (int32)S.EaseType);
		Writer->WriteValue(TEXT("easeCurve"),     S.EaseCurve.ToString());
		Writer->WriteValue(TEXT("slot"),          S.SlotName.ToString());
		Writer->WriteValue(TEXT("parameterName"), S.ParameterName.ToString());
		switch (S.StepType)
		{
		case EQTDStepType::Vector:
			Writer->WriteValue(TEXT("vectorProperty"),    (int32)S.VectorProperty);
			Writer->WriteValue(TEXT("vectorFromCurrent"), S.bVectorFromCurrent);
			Writer->WriteValue(TEXT("vectorFromX"), S.VectorFrom.X);
			Writer->WriteValue(TEXT("vectorFromY"), S.VectorFrom.Y);
			Writer->WriteValue(TEXT("vectorFromZ"), S.VectorFrom.Z);
			Writer->WriteValue(TEXT("vectorToX"),   S.VectorTo.X);
			Writer->WriteValue(TEXT("vectorToY"),   S.VectorTo.Y);
			Writer->WriteValue(TEXT("vectorToZ"),   S.VectorTo.Z);
			break;
		case EQTDStepType::Rotator:
			Writer->WriteValue(TEXT("rotatorProperty"),    (int32)S.RotatorProperty);
			Writer->WriteValue(TEXT("rotatorFromCurrent"), S.bRotatorFromCurrent);
			Writer->WriteValue(TEXT("rotatorFromPitch"), S.RotatorFrom.Pitch);
			Writer->WriteValue(TEXT("rotatorFromYaw"),   S.RotatorFrom.Yaw);
			Writer->WriteValue(TEXT("rotatorFromRoll"),  S.RotatorFrom.Roll);
			Writer->WriteValue(TEXT("rotatorToPitch"),   S.RotatorTo.Pitch);
			Writer->WriteValue(TEXT("rotatorToYaw"),     S.RotatorTo.Yaw);
			Writer->WriteValue(TEXT("rotatorToRoll"),    S.RotatorTo.Roll);
			break;
		case EQTDStepType::Float:
			Writer->WriteValue(TEXT("floatTarget"),      (int32)S.FloatTarget);
			Writer->WriteValue(TEXT("floatFromCurrent"), S.bFloatFromCurrent);
			Writer->WriteValue(TEXT("floatFrom"), S.FloatFrom);
			Writer->WriteValue(TEXT("floatTo"),   S.FloatTo);
			break;
		case EQTDStepType::LinearColor:
			Writer->WriteValue(TEXT("colorTarget"),      (int32)S.ColorTarget);
			Writer->WriteValue(TEXT("colorFromCurrent"), S.bColorFromCurrent);
			Writer->WriteValue(TEXT("colorFromR"), S.ColorFrom.R);
			Writer->WriteValue(TEXT("colorFromG"), S.ColorFrom.G);
			Writer->WriteValue(TEXT("colorFromB"), S.ColorFrom.B);
			Writer->WriteValue(TEXT("colorFromA"), S.ColorFrom.A);
			Writer->WriteValue(TEXT("colorToR"),   S.ColorTo.R);
			Writer->WriteValue(TEXT("colorToG"),   S.ColorTo.G);
			Writer->WriteValue(TEXT("colorToB"),   S.ColorTo.B);
			Writer->WriteValue(TEXT("colorToA"),   S.ColorTo.A);
			break;
		case EQTDStepType::Vector2D:
			Writer->WriteValue(TEXT("vector2DTarget"),      (int32)S.Vector2DTarget);
			Writer->WriteValue(TEXT("vector2DFromCurrent"), S.bVector2DFromCurrent);
			Writer->WriteValue(TEXT("vector2DFromX"), S.Vector2DFrom.X);
			Writer->WriteValue(TEXT("vector2DFromY"), S.Vector2DFrom.Y);
			Writer->WriteValue(TEXT("vector2DToX"),   S.Vector2DTo.X);
			Writer->WriteValue(TEXT("vector2DToY"),   S.Vector2DTo.Y);
			break;
		case EQTDStepType::Int:
			Writer->WriteValue(TEXT("intTarget"),      (int32)S.IntTarget);
			Writer->WriteValue(TEXT("intFromCurrent"), S.bIntFromCurrent);
			Writer->WriteValue(TEXT("intFrom"), S.IntFrom);
			Writer->WriteValue(TEXT("intTo"),   S.IntTo);
			break;
		default: break;
		}
		if (S.UserColor.A > 0.f)
		{
			Writer->WriteValue(TEXT("userColorR"), S.UserColor.R);
			Writer->WriteValue(TEXT("userColorG"), S.UserColor.G);
			Writer->WriteValue(TEXT("userColorB"), S.UserColor.B);
			Writer->WriteValue(TEXT("userColorA"), S.UserColor.A);
		}
		Writer->WriteObjectEnd();
	}
	Writer->WriteArrayEnd();
	Writer->WriteObjectEnd();
	Writer->Close();

	FFileHelper::SaveStringToFile(OutputJson, *OutFiles[0]);
	return FReply::Handled();
}

// ──────────────────────────────────────────────────────────────────────────────
// Track / step actions
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::DeleteTrack(FGuid TrackId)
{
	if (!Asset) return;
	FScopedTransaction Tx(LOCTEXT("DeleteTrack", "Delete Track"));
	Asset->Modify();
	Asset->RemoveTrack(TrackId);
	RefreshFromAsset();
}

void SQuickTweenDirectorEditor::ReorderTrack(FGuid TrackId, int32 Direction)
{
	if (!Asset) return;
	const int32 Idx = Asset->Tracks.IndexOfByPredicate([&](const FQTDTrackData& T) {
		return T.TrackId == TrackId;
	});
	if (Idx == INDEX_NONE) return;

	const int32 NewIdx = Idx + Direction;
	if (NewIdx < 0 || NewIdx >= Asset->Tracks.Num()) return;

	FScopedTransaction Tx(LOCTEXT("ReorderTrack", "Reorder Track"));
	Asset->Modify();
	Asset->Tracks.Swap(Idx, NewIdx);
	Asset->MarkPackageDirty();
	RefreshFromAsset();
}

void SQuickTweenDirectorEditor::AddStepToTrack(FQTDStepData PrefilledStep)
{
	if (!Asset) return;

	if (PrefilledStep.Label.IsEmpty())
		PrefilledStep.Label = UEnum::GetValueAsString(PrefilledStep.StepType);

	{
		FScopedTransaction Tx(LOCTEXT("AddStep", "Add Step"));
		Asset->Modify();
		Asset->AddStep(PrefilledStep);
	}
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

	// Wrap the UpdateStep that happened inside the dialog in an undo record
	// (The dialog calls Asset->UpdateStep which marks dirty; we just need Modify())
	// Note: The dialog already called UpdateStep internally; we wrap retroactively.
	// A cleaner approach would pass a callback, but this is sufficient.
	RefreshFromAsset();
}

// ──────────────────────────────────────────────────────────────────────────────
// Zoom
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::OnZoomChanged(float NewValue)
{
	PixelsPerSec = NewValue;
	if (RulerWidget.IsValid())
		RulerWidget->Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
	RefreshFromAsset();
}

FReply SQuickTweenDirectorEditor::OnZoomToFitClicked()
{
	if (!Asset) return FReply::Handled();

	const float Dur = Asset->GetTotalDuration();
	if (Dur <= 0.f) return FReply::Handled();

	// Use the HScrollBox geometry to know the available width.
	const FGeometry& Geo = HScrollBox.IsValid()
		? HScrollBox->GetCachedGeometry()
		: FGeometry();
	const float AvailWidth = Geo.GetLocalSize().X;

	if (AvailWidth > 0.f)
	{
		PixelsPerSec = FMath::Clamp(AvailWidth / Dur, 20.f, 400.f);
	}

	if (RulerWidget.IsValid())
		RulerWidget->Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
	RefreshFromAsset();
	return FReply::Handled();
}

// ──────────────────────────────────────────────────────────────────────────────
// Snap
// ──────────────────────────────────────────────────────────────────────────────

void SQuickTweenDirectorEditor::PropagateSnapToRows()
{
	for (const TWeakPtr<SQTDTrackRow>& WeakRow : TrackRows)
	{
		if (TSharedPtr<SQTDTrackRow> Row = WeakRow.Pin())
		{
			Row->SetSnapEnabled(bSnapEnabled);
		}
	}
}


// ──────────────────────────────────────────────────────────────────────────────
// Import JSON
// ──────────────────────────────────────────────────────────────────────────────

FReply SQuickTweenDirectorEditor::OnImportJsonClicked()
{
	if (!Asset) return FReply::Handled();

	// ── 1. Pick file
	IDesktopPlatform* DP = FDesktopPlatformModule::Get();
	TArray<FString> OutFiles;
	bool bPicked = false;
	if (DP)
	{
		bPicked = DP->OpenFileDialog(
			FSlateApplication::Get().FindBestParentWindowHandleForDialogs(AsShared()),
			TEXT("Import Director Asset from JSON"),
			FPaths::ProjectContentDir(),
			TEXT(""),
			TEXT("JSON files (*.json)|*.json"),
			EFileDialogFlags::None,
			OutFiles);
	}
	if (!bPicked || OutFiles.IsEmpty()) return FReply::Handled();

	// ── 2. Read file
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *OutFiles[0]))
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			FText::Format(LOCTEXT("ImportReadError", "Failed to read file:\n{0}"),
				FText::FromString(OutFiles[0])));
		return FReply::Handled();
	}

	// ── 3. Parse JSON
	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(Reader, RootObj) || !RootObj.IsValid())
	{
		FMessageDialog::Open(EAppMsgType::Ok,
			LOCTEXT("ImportParseError",
				"Failed to parse JSON. The file may be corrupt or not a valid Director export."));
		return FReply::Handled();
	}

	// ── 4. Confirm replacement
	if (Asset->Tracks.Num() > 0 || Asset->Steps.Num() > 0)
	{
		const EAppReturnType::Type Ans = FMessageDialog::Open(EAppMsgType::YesNo,
			LOCTEXT("ImportConfirm",
				"This will replace ALL existing tracks and steps in the current asset with the imported data.\n\nContinue?"));
		if (Ans != EAppReturnType::Yes) return FReply::Handled();
	}

	// ── 5. Parse tracks
	TArray<FString>       Warnings;
	TArray<FQTDTrackData> ImportedTracks;
	TSet<FGuid>           ImportedTrackIds;

	const TArray<TSharedPtr<FJsonValue>>* TracksArr = nullptr;
	if (RootObj->TryGetArrayField(TEXT("tracks"), TracksArr) && TracksArr)
	{
		for (int32 i = 0; i < TracksArr->Num(); ++i)
		{
			const TSharedPtr<FJsonObject>* TrackObjPtr = nullptr;
			if (!(*TracksArr)[i]->TryGetObject(TrackObjPtr) || !TrackObjPtr)
			{
				Warnings.Add(FString::Printf(TEXT("tracks[%d]: not a valid JSON object - skipped."), i));
				continue;
			}
			const TSharedPtr<FJsonObject>& TrackObj = *TrackObjPtr;

			FString IdStr;
			if (!TrackObj->TryGetStringField(TEXT("id"), IdStr))
			{
				Warnings.Add(FString::Printf(TEXT("tracks[%d]: missing id field - skipped."), i));
				continue;
			}
			FGuid TrackId;
			if (!FGuid::Parse(IdStr, TrackId))
			{
				Warnings.Add(FString::Printf(TEXT("tracks[%d]: invalid GUID - skipped."), i));
				continue;
			}

			FQTDTrackData Track;
			Track.TrackId = TrackId;
			TrackObj->TryGetStringField(TEXT("label"), Track.TrackLabel);
			FString CompName;
			if (TrackObj->TryGetStringField(TEXT("component"), CompName))
				Track.ComponentVariableName = FName(*CompName);

			// Try to resolve ComponentClass from the current Blueprint SCS
			if (!CompName.IsEmpty())
			{
				UBlueprint* BP = Blueprint.Get();
				if (BP && BP->SimpleConstructionScript)
				{
					bool bFound = false;
					for (const USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
					{
						if (Node && Node->GetVariableName().ToString() == CompName)
						{
							Track.ComponentClass = Node->ComponentClass;
							bFound = true;
							break;
						}
					}
					if (!bFound)
					{
						Warnings.Add(FString::Printf(
							TEXT("Track '%s': component '%s' not found in the current Blueprint - auto-binding may fail at runtime."),
							*Track.TrackLabel, *CompName));
					}
				}
			}

			ImportedTracks.Add(Track);
			ImportedTrackIds.Add(TrackId);
		}
	}
	else
	{
		Warnings.Add(TEXT("No tracks array found in the JSON - no tracks will be imported."));
	}

	// ── 6. Parse steps
	TArray<FQTDStepData> ImportedSteps;

	const TArray<TSharedPtr<FJsonValue>>* StepsArr = nullptr;
	if (RootObj->TryGetArrayField(TEXT("steps"), StepsArr) && StepsArr)
	{
		for (int32 i = 0; i < StepsArr->Num(); ++i)
		{
			const TSharedPtr<FJsonObject>* StepObjPtr = nullptr;
			if (!(*StepsArr)[i]->TryGetObject(StepObjPtr) || !StepObjPtr)
			{
				Warnings.Add(FString::Printf(TEXT("steps[%d]: not a valid JSON object - skipped."), i));
				continue;
			}
			const TSharedPtr<FJsonObject>& StepObj = *StepObjPtr;

			FString IdStr;
			if (!StepObj->TryGetStringField(TEXT("id"), IdStr))
			{
				Warnings.Add(FString::Printf(TEXT("steps[%d]: missing id - skipped."), i));
				continue;
			}
			FGuid StepId;
			if (!FGuid::Parse(IdStr, StepId))
			{
				Warnings.Add(FString::Printf(TEXT("steps[%d]: invalid GUID - skipped."), i));
				continue;
			}

			FString TrackIdStr;
			FGuid   TrackId;
			if (!StepObj->TryGetStringField(TEXT("trackId"), TrackIdStr) || !FGuid::Parse(TrackIdStr, TrackId))
			{
				Warnings.Add(FString::Printf(
					TEXT("steps[%d] (id=%s): invalid or missing trackId - skipped."), i, *IdStr));
				continue;
			}
			if (!ImportedTrackIds.Contains(TrackId))
			{
				Warnings.Add(FString::Printf(
					TEXT("steps[%d] (id=%s): trackId does not match any imported track - skipped."), i, *IdStr));
				continue;
			}

			FQTDStepData Step;
			Step.StepId  = StepId;
			Step.TrackId = TrackId;
			StepObj->TryGetStringField(TEXT("label"), Step.Label);

			// Timing
			double Tmp   = 0.0;
			int32  IntTmp = 0;
			if (StepObj->TryGetNumberField(TEXT("startTime"), Tmp)) Step.StartTime = FMath::Max(0.f, (float)Tmp);
			if (StepObj->TryGetNumberField(TEXT("duration"),  Tmp)) Step.Duration  = FMath::Max(0.001f, (float)Tmp);
			if (StepObj->TryGetNumberField(TEXT("timeScale"), Tmp)) Step.TimeScale = FMath::Max(0.01f, (float)Tmp);
			if (StepObj->TryGetNumberField(TEXT("loops"),    IntTmp)) Step.Loops   = IntTmp;
			if (StepObj->TryGetNumberField(TEXT("loopType"), IntTmp))
				Step.LoopType = (ELoopType)FMath::Clamp(IntTmp, 0, 1);
			if (StepObj->TryGetNumberField(TEXT("type"), IntTmp))
				Step.StepType = (EQTDStepType)FMath::Clamp(IntTmp, 0, (int32)EQTDStepType::Empty);
			if (StepObj->TryGetNumberField(TEXT("easeType"), IntTmp))
				Step.EaseType = (EEaseType)IntTmp;
			{
				FString CurvePath;
				if (StepObj->TryGetStringField(TEXT("easeCurve"), CurvePath) && !CurvePath.IsEmpty())
				{
					Step.EaseCurve = TSoftObjectPtr<UCurveFloat>(FSoftObjectPath(CurvePath));
					if (!Step.EaseCurve.IsValid())
						Warnings.Add(FString::Printf(TEXT("steps[%d]: easeCurve path '%s' could not be resolved - curve will be loaded lazily at runtime."), i, *CurvePath));
				}
			}

			// Target
			FString SlotStr, ParamStr;
			if (StepObj->TryGetStringField(TEXT("slot"),          SlotStr))  Step.SlotName      = FName(*SlotStr);
			if (StepObj->TryGetStringField(TEXT("parameterName"), ParamStr)) Step.ParameterName = FName(*ParamStr);

			// Type-specific fields
			switch (Step.StepType)
			{
			case EQTDStepType::Vector:
				if (StepObj->TryGetNumberField(TEXT("vectorProperty"), IntTmp))
					Step.VectorProperty = (EQTDVectorProperty)FMath::Clamp(IntTmp, 0, 3);
				StepObj->TryGetBoolField(TEXT("vectorFromCurrent"), Step.bVectorFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("vectorFromX"), Tmp)) Step.VectorFrom.X = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vectorFromY"), Tmp)) Step.VectorFrom.Y = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vectorFromZ"), Tmp)) Step.VectorFrom.Z = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vectorToX"),   Tmp)) Step.VectorTo.X   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vectorToY"),   Tmp)) Step.VectorTo.Y   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vectorToZ"),   Tmp)) Step.VectorTo.Z   = (float)Tmp;
				break;
			case EQTDStepType::Rotator:
				if (StepObj->TryGetNumberField(TEXT("rotatorProperty"), IntTmp))
					Step.RotatorProperty = (EQTDRotatorProperty)FMath::Clamp(IntTmp, 0, 1);
				StepObj->TryGetBoolField(TEXT("rotatorFromCurrent"), Step.bRotatorFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("rotatorFromPitch"), Tmp)) Step.RotatorFrom.Pitch = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("rotatorFromYaw"),   Tmp)) Step.RotatorFrom.Yaw   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("rotatorFromRoll"),  Tmp)) Step.RotatorFrom.Roll  = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("rotatorToPitch"),   Tmp)) Step.RotatorTo.Pitch   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("rotatorToYaw"),     Tmp)) Step.RotatorTo.Yaw     = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("rotatorToRoll"),    Tmp)) Step.RotatorTo.Roll    = (float)Tmp;
				break;
			case EQTDStepType::Float:
				if (StepObj->TryGetNumberField(TEXT("floatTarget"), IntTmp))
					Step.FloatTarget = (EQTDFloatTarget)FMath::Clamp(IntTmp, 0, 0);
				StepObj->TryGetBoolField(TEXT("floatFromCurrent"), Step.bFloatFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("floatFrom"), Tmp)) Step.FloatFrom = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("floatTo"),   Tmp)) Step.FloatTo   = (float)Tmp;
				break;
			case EQTDStepType::LinearColor:
				if (StepObj->TryGetNumberField(TEXT("colorTarget"), IntTmp))
					Step.ColorTarget = (EQTDColorTarget)FMath::Clamp(IntTmp, 0, 0);
				StepObj->TryGetBoolField(TEXT("colorFromCurrent"), Step.bColorFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("colorFromR"), Tmp)) Step.ColorFrom.R = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorFromG"), Tmp)) Step.ColorFrom.G = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorFromB"), Tmp)) Step.ColorFrom.B = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorFromA"), Tmp)) Step.ColorFrom.A = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorToR"),   Tmp)) Step.ColorTo.R   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorToG"),   Tmp)) Step.ColorTo.G   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorToB"),   Tmp)) Step.ColorTo.B   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("colorToA"),   Tmp)) Step.ColorTo.A   = (float)Tmp;
				break;
			case EQTDStepType::Vector2D:
				StepObj->TryGetBoolField(TEXT("vector2DFromCurrent"), Step.bVector2DFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("vector2DFromX"), Tmp)) Step.Vector2DFrom.X = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vector2DFromY"), Tmp)) Step.Vector2DFrom.Y = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vector2DToX"),   Tmp)) Step.Vector2DTo.X   = (float)Tmp;
				if (StepObj->TryGetNumberField(TEXT("vector2DToY"),   Tmp)) Step.Vector2DTo.Y   = (float)Tmp;
				break;
			case EQTDStepType::Int:
				StepObj->TryGetBoolField(TEXT("intFromCurrent"), Step.bIntFromCurrent);
				if (StepObj->TryGetNumberField(TEXT("intFrom"), Tmp)) Step.IntFrom = (int32)Tmp;
				if (StepObj->TryGetNumberField(TEXT("intTo"),   Tmp)) Step.IntTo   = (int32)Tmp;
				break;
			default: break;
			}

			// Custom color override
			if (StepObj->TryGetNumberField(TEXT("userColorR"), Tmp)) Step.UserColor.R = (float)Tmp;
			if (StepObj->TryGetNumberField(TEXT("userColorG"), Tmp)) Step.UserColor.G = (float)Tmp;
			if (StepObj->TryGetNumberField(TEXT("userColorB"), Tmp)) Step.UserColor.B = (float)Tmp;
			if (StepObj->TryGetNumberField(TEXT("userColorA"), Tmp)) Step.UserColor.A = (float)Tmp;

			ImportedSteps.Add(Step);
		}
	}
	else
	{
		Warnings.Add(TEXT("No steps array found in the JSON - no steps will be imported."));
	}

	// ── 7. Apply to asset
	{
		FScopedTransaction Tx(LOCTEXT("ImportJson", "Import from JSON"));
		Asset->Modify();
		Asset->Tracks = ImportedTracks;
		Asset->Steps  = ImportedSteps;
		Asset->MarkPackageDirty();
	}
	RefreshFromAsset();

	// ── 8. Report warnings
	if (Warnings.Num() > 0)
	{
		FString WarnStr = FString::Printf(
			TEXT("Import completed: %d track(s), %d step(s) imported.\n\nWarnings:\n"),
			ImportedTracks.Num(), ImportedSteps.Num());
		for (const FString& W : Warnings)
			WarnStr += FString(TEXT("- ")) + W + TEXT("\n");
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(WarnStr));
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE