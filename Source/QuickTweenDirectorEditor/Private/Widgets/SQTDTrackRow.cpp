// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQTDTrackRow.h"
#include "QuickTweenDirectorAsset.h"
#include "Widgets/SQuickTweenDirectorEditor.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"
#include "Input/Reply.h"

#define LOCTEXT_NAMESPACE "SQTDTrackRow"

// ──────────────────────────────────────────────────────────────────────────────
// SQTDStepContent — leaf widget: paints step boxes + handles mouse + context menu
// ──────────────────────────────────────────────────────────────────────────────

class SQTDStepContent : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SQTDStepContent) {}
		SLATE_ARGUMENT(FQTDTrackData,             Track)
		SLATE_ARGUMENT(UQuickTweenDirectorAsset*, Asset)
		SLATE_ARGUMENT(float,                     PixelsPerSec)
		SLATE_EVENT (FOnStepAdded,                OnStepAdded)
		SLATE_EVENT (FOnStepEdit,                 OnStepEdit)
		SLATE_EVENT (FOnStepMoved,                OnStepMoved)
		SLATE_EVENT (FOnStepDeleted,              OnStepDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Track         = InArgs._Track;
		Asset         = InArgs._Asset;
		PixelsPerSec  = InArgs._PixelsPerSec;
		OnStepAdded   = InArgs._OnStepAdded;
		OnStepEdit    = InArgs._OnStepEdit;
		OnStepMoved   = InArgs._OnStepMoved;
		OnStepDeleted = InArgs._OnStepDeleted;
	}

	void SetPixelsPerSec(float PPS) { PixelsPerSec = PPS; Invalidate(EInvalidateWidgetReason::Paint); }

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		const float Dur = Asset ? Asset->GetTotalDuration() : 1.0f;
		return FVector2D(FMath::Max(Dur, 1.0f) * PixelsPerSec + 120.0f, QTDEditorConstants::TrackHeight);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	                      const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	                      int32 LayerId, const FWidgetStyle&, bool bParentEnabled) const override
	{
		if (!Asset) return LayerId;

		// Track background
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppStyle::GetBrush("ToolPanel.GroupBorder"),
			ESlateDrawEffect::None, FLinearColor(0.12f, 0.12f, 0.12f));
		++LayerId;

		// Center line
		const float H = AllottedGeometry.GetLocalSize().Y;
		const float W = AllottedGeometry.GetLocalSize().X;
		TArray<FVector2D> CLPts = { FVector2D(0.0f, H * 0.5f), FVector2D(W, H * 0.5f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			CLPts, ESlateDrawEffect::None, FLinearColor(0.22f, 0.22f, 0.22f), true, 1.0f);
		++LayerId;

		// Step boxes
		TArray<FQTDStepData*> TrackSteps = Asset->GetStepsForTrack(Track.TrackId);
		for (const FQTDStepData* Step : TrackSteps)
		{
			if (!Step) continue;

			const float X    = Step->StartTime * PixelsPerSec;
			const float SW   = FMath::Max(Step->Duration * PixelsPerSec * FMath::Max(Step->Loops, 1),
			                              QTDEditorConstants::MinStepWidth);
			const float PadY = 4.0f;

			const FLinearColor Base = Step->GetTypeColor();
			const FLinearColor Dim  = Base * 0.7f;

			// Box fill
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(SW - 2.0f, H - PadY * 2.0f),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Dim.CopyWithNewOpacity(0.85f));

			// Left accent bar
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(3.0f, H - PadY * 2.0f),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, Base);

			// Label
			if (SW > 20.0f)
			{
				const FText LabelText = Step->Label.IsEmpty()
					? FText::FromString(UEnum::GetValueAsString(Step->StepType))
					: FText::FromString(Step->Label);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(SW - 10.0f, H - PadY * 2.0f - 4.0f),
						FSlateLayoutTransform(FVector2f(X + 6.0f, PadY + 2.0f))),
					LabelText, FAppStyle::GetFontStyle("SmallFont"),
					ESlateDrawEffect::None, FLinearColor::White);
			}

			// Duration label
			if (SW > 50.0f)
			{
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 2,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(40.0f, H - PadY * 2.0f - 4.0f),
						FSlateLayoutTransform(FVector2f(X + SW - 44.0f, PadY + 2.0f))),
					FText::FromString(FString::Printf(TEXT("%.2fs"), Step->Duration)),
					FAppStyle::GetFontStyle("TinyText"),
					ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, 0.6f));
			}
		}

		return LayerId + 3;
	}

	// ── Mouse ──────────────────────────────────────────────────────────────────

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			if (const FQTDStepData* Hit = HitTestStep(Local))
			{
				bIsDragging      = true;
				DraggedStepId    = Hit->StepId;
				DragStartMouseX  = Local.X;
				DragOrigStartTime = Hit->StartTime;
				return FReply::Handled().CaptureMouse(AsShared());
			}
		}

		if (Event.GetEffectingButton() == EKeys::RightMouseButton)
		{
			ShowContextMenu(Event.GetScreenSpacePosition(),
			                Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton && bIsDragging)
		{
			bIsDragging = false;
			DraggedStepId.Invalidate();
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (bIsDragging && DraggedStepId.IsValid())
		{
			const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			float NewStart = DragOrigStartTime + (Local.X - DragStartMouseX) / PixelsPerSec;
			NewStart = FMath::RoundToFloat(NewStart / QTDEditorConstants::SnapIncrement)
			           * QTDEditorConstants::SnapIncrement;
			NewStart = FMath::Max(0.0f, NewStart);
			OnStepMoved.ExecuteIfBound(DraggedStepId, NewStart);
			Invalidate(EInvalidateWidgetReason::Paint);
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	virtual void OnMouseLeave(const FPointerEvent&) override
	{
		bIsDragging = false;
		DraggedStepId.Invalidate();
	}

	virtual FReply OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			if (const FQTDStepData* Hit = HitTestStep(Local))
			{
				OnStepEdit.ExecuteIfBound(*Hit);
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	}

	virtual bool SupportsKeyboardFocus() const override { return false; }

private:

	// ── Context menu ──────────────────────────────────────────────────────────

	void ShowContextMenu(FVector2D ScreenPos, FVector2D LocalPos)
	{
		const float ClickTime     = LocalPos.X / PixelsPerSec;
		const FQTDStepData* HitStep = HitTestStep(LocalPos);

		FMenuBuilder Builder(true, nullptr);

		if (HitStep)
		{
			const FGuid StepId = HitStep->StepId;
			Builder.AddMenuEntry(
				LOCTEXT("DeleteStep", "Delete Step"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, StepId]() {
					OnStepDeleted.ExecuteIfBound(StepId);
				})));
		}
		else
		{
			// Build the "Add Step Here →" submenu filtered by component class
			Builder.AddSubMenu(
				LOCTEXT("AddStep", "Add Step Here"),
				LOCTEXT("AddStepTip", "Choose the animation type for this track"),
				FNewMenuDelegate::CreateSP(this, &SQTDStepContent::BuildAddStepSubmenu, ClickTime)
			);
		}

		FSlateApplication::Get().PushMenu(
			AsShared(), FWidgetPath(), Builder.MakeWidget(),
			ScreenPos, FPopupTransitionEffect::ContextMenu);
	}

	void BuildAddStepSubmenu(FMenuBuilder& SubBuilder, float ClickTime)
	{
		const TSubclassOf<UActorComponent> CompClass = Track.ComponentClass;
		const FName SlotName = Track.ComponentVariableName;

		const bool bIsScene     = CompClass && CompClass->IsChildOf<USceneComponent>();
		const bool bIsPrimitive = CompClass && CompClass->IsChildOf<UPrimitiveComponent>();

		// ── Scene component types ──────────────────────────────────────────
		if (bIsScene)
		{
			SubBuilder.BeginSection("Location", LOCTEXT("SecLocation", "Location"));
			SubBuilder.AddMenuEntry(
				LOCTEXT("RelLoc", "Relative Location"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::RelativeLocation, SlotName, ClickTime))));
			SubBuilder.AddMenuEntry(
				LOCTEXT("WorldLoc", "World Location"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::WorldLocation, SlotName, ClickTime))));
			SubBuilder.EndSection();

			SubBuilder.BeginSection("Rotation", LOCTEXT("SecRotation", "Rotation"));
			SubBuilder.AddMenuEntry(
				LOCTEXT("RelRot", "Relative Rotation"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeRotatorStep(EQTDRotatorProperty::RelativeRotation, SlotName, ClickTime))));
			SubBuilder.AddMenuEntry(
				LOCTEXT("WorldRot", "World Rotation"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeRotatorStep(EQTDRotatorProperty::WorldRotation, SlotName, ClickTime))));
			SubBuilder.EndSection();

			SubBuilder.BeginSection("Scale", LOCTEXT("SecScale", "Scale"));
			SubBuilder.AddMenuEntry(
				LOCTEXT("RelScale", "Relative Scale"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::RelativeScale3D, SlotName, ClickTime))));
			SubBuilder.AddMenuEntry(
				LOCTEXT("WorldScale", "World Scale"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::WorldScale3D, SlotName, ClickTime))));
			SubBuilder.EndSection();
		}

		// ── Primitive / material types ─────────────────────────────────────
		if (bIsPrimitive || !bIsScene)
		{
			SubBuilder.BeginSection("Material", LOCTEXT("SecMaterial", "Material"));
			SubBuilder.AddMenuEntry(
				LOCTEXT("MatFloat", "Material Scalar Parameter"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeFloatStep(EQTDFloatTarget::MaterialScalar, SlotName, ClickTime))));
			SubBuilder.AddMenuEntry(
				LOCTEXT("MatColor", "Material Vector Parameter (Color)"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeColorStep(EQTDColorTarget::MaterialVector, SlotName, ClickTime))));
			SubBuilder.EndSection();
		}

		// ── Custom / generic ──────────────────────────────────────────────
		SubBuilder.BeginSection("Custom", LOCTEXT("SecCustom", "Custom"));
		SubBuilder.AddMenuEntry(
			LOCTEXT("CustomFloat", "Custom Float"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
				MakeFloatStep(EQTDFloatTarget::Custom, SlotName, ClickTime))));
		SubBuilder.AddMenuEntry(
			LOCTEXT("CustomColor", "Custom Color"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
				MakeColorStep(EQTDColorTarget::Custom, SlotName, ClickTime))));
		SubBuilder.AddMenuEntry(
			LOCTEXT("Delay", "Delay (Empty)"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
				MakeEmptyStep(SlotName, ClickTime))));
		SubBuilder.EndSection();
	}

	void FireStepAdded(FQTDStepData Step) { OnStepAdded.ExecuteIfBound(Step); }

	// ── Step factory helpers ───────────────────────────────────────────────────

	FQTDStepData MakeBaseStep(EQTDStepType Type, FName SlotName, float StartTime) const
	{
		FQTDStepData S;
		S.StepId    = FGuid::NewGuid();
		S.TrackId   = Track.TrackId;
		S.StepType  = Type;
		S.SlotName  = SlotName;
		S.StartTime = FMath::Max(0.0f, StartTime);
		S.Duration  = 1.0f;
		return S;
	}

	FQTDStepData MakeVectorStep(EQTDVectorProperty Prop, FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::Vector, SlotName, T);
		S.VectorProperty = Prop;
		switch (Prop)
		{
		case EQTDVectorProperty::RelativeLocation: S.Label = TEXT("Relative Location"); break;
		case EQTDVectorProperty::WorldLocation:    S.Label = TEXT("World Location");    break;
		case EQTDVectorProperty::RelativeScale3D:  S.Label = TEXT("Relative Scale");    break;
		case EQTDVectorProperty::WorldScale3D:     S.Label = TEXT("World Scale");       break;
		}
		return S;
	}

	FQTDStepData MakeRotatorStep(EQTDRotatorProperty Prop, FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::Rotator, SlotName, T);
		S.RotatorProperty = Prop;
		S.Label = (Prop == EQTDRotatorProperty::RelativeRotation)
			? TEXT("Relative Rotation") : TEXT("World Rotation");
		return S;
	}

	FQTDStepData MakeFloatStep(EQTDFloatTarget Target, FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::Float, SlotName, T);
		S.FloatTarget = Target;
		S.Label = (Target == EQTDFloatTarget::MaterialScalar)
			? TEXT("Material Float") : TEXT("Custom Float");
		return S;
	}

	FQTDStepData MakeColorStep(EQTDColorTarget Target, FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::LinearColor, SlotName, T);
		S.ColorTarget = Target;
		S.Label = (Target == EQTDColorTarget::MaterialVector)
			? TEXT("Material Color") : TEXT("Custom Color");
		return S;
	}

	FQTDStepData MakeEmptyStep(FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::Empty, SlotName, T);
		S.Label = TEXT("Delay");
		return S;
	}

	// ── Hit test ──────────────────────────────────────────────────────────────

	const FQTDStepData* HitTestStep(FVector2D LocalPos) const
	{
		if (!Asset) return nullptr;
		TArray<FQTDStepData*> Steps = Asset->GetStepsForTrack(Track.TrackId);
		for (int32 i = Steps.Num() - 1; i >= 0; --i)
		{
			if (!Steps[i]) continue;
			const float X = Steps[i]->StartTime * PixelsPerSec;
			const float W = FMath::Max(Steps[i]->Duration * PixelsPerSec * FMath::Max(Steps[i]->Loops, 1),
			                           QTDEditorConstants::MinStepWidth);
			if (LocalPos.X >= X && LocalPos.X <= X + W) return Steps[i];
		}
		return nullptr;
	}

	// ── State ─────────────────────────────────────────────────────────────────

	FQTDTrackData             Track;
	UQuickTweenDirectorAsset* Asset        = nullptr;
	float                     PixelsPerSec = 80.0f;

	FOnStepAdded   OnStepAdded;
	FOnStepEdit    OnStepEdit;
	FOnStepMoved   OnStepMoved;
	FOnStepDeleted OnStepDeleted;

	bool  bIsDragging        = false;
	FGuid DraggedStepId;
	float DragStartMouseX    = 0.0f;
	float DragOrigStartTime  = 0.0f;
};

// ──────────────────────────────────────────────────────────────────────────────
// SQTDTrackRow
// ──────────────────────────────────────────────────────────────────────────────

void SQTDTrackRow::Construct(const FArguments& InArgs)
{
	Track         = InArgs._Track;
	Asset         = InArgs._Asset;
	PixelsPerSec  = InArgs._PixelsPerSec;
	OnTrackDelete = InArgs._OnTrackDelete;
	OnStepAdded   = InArgs._OnStepAdded;
	OnStepEdit    = InArgs._OnStepEdit;
	OnStepMoved   = InArgs._OnStepMoved;
	OnStepDeleted = InArgs._OnStepDeleted;

	// Build the step content widget (placed externally in the shared H-scroll box).
	SAssignNew(StepContent, SQTDStepContent)
		.Track(Track)
		.Asset(Asset)
		.PixelsPerSec(PixelsPerSec)
		.OnStepAdded(OnStepAdded)
		.OnStepEdit(OnStepEdit)
		.OnStepMoved(OnStepMoved)
		.OnStepDeleted(OnStepDeleted);

	// Build a small component-type chip to display next to the label
	const FString TypeChip = (Track.ComponentClass)
		? (FString(TEXT(" [")) + Track.ComponentClass->GetName() + TEXT("]"))
		: FString();

	// This widget shows ONLY the label column.
	// The step content (StepContent) is placed separately by SQuickTweenDirectorEditor
	// inside the single shared horizontal scroll box so ruler + all steps scroll together.
	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(QTDEditorConstants::TrackLabelWidth)
		.HeightOverride(QTDEditorConstants::TrackHeight)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryTop"))
			.Padding(FMargin(6.0f, 0.0f))
			[
				SNew(SHorizontalBox)

				// Component class icon
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FSlateIconFinder::FindIconBrushForClass(
						Track.ComponentClass.Get(), TEXT("SCS.Component")))
					.DesiredSizeOverride(FVector2D(14.0f, 14.0f))
					.Visibility(Track.ComponentClass ? EVisibility::Visible : EVisibility::Collapsed)
				]

				// Component variable name + class chip
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this]() { return FText::FromString(Track.TrackLabel); })
						.Font(FAppStyle::GetFontStyle("SmallFont"))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(TypeChip))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
						.Visibility(Track.ComponentClass ? EVisibility::Visible : EVisibility::Collapsed)
					]
				]

				// Delete button
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ToolTipText(LOCTEXT("DeleteTrack", "Delete this track"))
					.OnClicked_Lambda([this]() -> FReply {
						OnTrackDelete.ExecuteIfBound(Track.TrackId);
						return FReply::Handled();
					})
					.ContentPadding(2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("✕")))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(1.0f, 0.3f, 0.3f))
					]
				]
			]
		]
	];
}

void SQTDTrackRow::SetPixelsPerSec(float NewPPS)
{
	PixelsPerSec = NewPPS;
	if (StepContent.IsValid()) StepContent->SetPixelsPerSec(NewPPS);
}

TSharedRef<SWidget> SQTDTrackRow::GetStepContentWidget() const
{
	return StepContent.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
