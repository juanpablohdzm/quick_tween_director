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
		SLATE_ARGUMENT(bool,                      IsSelected)
		SLATE_EVENT (FSimpleDelegate,             OnTrackSelected)
		SLATE_EVENT (FOnStepAdded,                OnStepAdded)
		SLATE_EVENT (FOnStepEdit,                 OnStepEdit)
		SLATE_EVENT (FOnStepMoved,                OnStepMoved)
		SLATE_EVENT (FOnStepDeleted,              OnStepDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Track            = InArgs._Track;
		Asset            = InArgs._Asset;
		PixelsPerSec     = InArgs._PixelsPerSec;
		bIsSelected      = InArgs._IsSelected;
		OnTrackSelected  = InArgs._OnTrackSelected;
		OnStepAdded      = InArgs._OnStepAdded;
		OnStepEdit       = InArgs._OnStepEdit;
		OnStepMoved      = InArgs._OnStepMoved;
		OnStepDeleted    = InArgs._OnStepDeleted;
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

		const float H = AllottedGeometry.GetLocalSize().Y;
		const float W = AllottedGeometry.GetLocalSize().X;

		// ── Track background ─────────────────────────────────────────────────
		const FLinearColor TrackBg = bIsSelected
			? FLinearColor(0.11f, 0.11f, 0.11f)
			: FLinearColor(0.09f, 0.09f, 0.09f);
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(),
			FAppStyle::GetBrush("WhiteBrush"),
			ESlateDrawEffect::None, TrackBg);
		++LayerId;

		// Bottom edge separator (1 px)
		TArray<FVector2D> BotPts = { FVector2D(0.f, H - 1.f), FVector2D(W, H - 1.f) };
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(),
			BotPts, ESlateDrawEffect::None, FLinearColor(0.16f, 0.16f, 0.16f), true, 1.f);
		++LayerId;

		// ── Step blocks ──────────────────────────────────────────────────────
		TArray<FQTDStepData*> TrackSteps = Asset->GetStepsForTrack(Track.TrackId);
		for (const FQTDStepData* Step : TrackSteps)
		{
			if (!Step) continue;

			const float PadY = 3.f;
			const float BH   = H - PadY * 2.f;
			const float X    = Step->StartTime * PixelsPerSec;
			const float SW   = FMath::Max(
				Step->Duration * PixelsPerSec * FMath::Max(Step->Loops, 1),
				QTDEditorConstants::MinStepWidth);

			const FLinearColor TypeColor = Step->GetTypeColor();
			const bool bHovered = (HoveredStepId == Step->StepId);
			const bool bPressed = (PressedStepId == Step->StepId);
			const float FillOpacity = bPressed ? 0.28f : (bHovered ? 0.22f : 0.16f);
			const float TopOpacity  = bPressed ? 0.60f : (bHovered ? 0.50f : 0.35f);

			// ── Tinted fill (semi-transparent) ────────────────────────────────
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(SW - 1.f, BH),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, TypeColor.CopyWithNewOpacity(FillOpacity));

			// ── Left accent stripe (4 px, full opacity) ───────────────────────
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(4.f, BH),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, TypeColor);

			// ── Top edge highlight ─────────────────────────────────────────────
			TArray<FVector2D> TopPts = { FVector2D(X + 4.f, PadY), FVector2D(X + SW - 1.f, PadY) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(), TopPts,
				ESlateDrawEffect::None, TypeColor.CopyWithNewOpacity(TopOpacity), true, 1.f);

			// ── Step label ────────────────────────────────────────────────────
			if (SW > 18.f)
			{
				const FText LabelText = FText::FromString(
					Step->Label.IsEmpty()
						? UEnum::GetDisplayValueAsText(Step->StepType).ToString()
						: Step->Label);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(FMath::Max(SW - 16.f, 4.f), BH - 4.f),
						FSlateLayoutTransform(FVector2f(X + 9.f, PadY + 4.f))),
					LabelText,
					FAppStyle::GetFontStyle("SmallFont"),
					ESlateDrawEffect::None,
					FLinearColor(1.f, 1.f, 1.f, 0.88f));
			}

			// ── Duration chip (bottom-right, accent-tinted) ───────────────────
			if (SW > 52.f)
			{
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(46.f, 12.f),
						FSlateLayoutTransform(FVector2f(X + SW - 50.f, PadY + BH - 13.f))),
					FText::FromString(FString::Printf(TEXT("%.2fs"), Step->Duration)),
					FAppStyle::GetFontStyle("TinyText"),
					ESlateDrawEffect::None,
					TypeColor.CopyWithNewOpacity(0.75f));
			}

			// ── Loop badge (bottom-left, when looping) ────────────────────────
			if (SW > 28.f && Step->Loops != 1)
			{
				const FString LoopStr = (Step->Loops < 0)
					? TEXT("∞") : FString::Printf(TEXT("×%d"), Step->Loops);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(30.f, 12.f),
						FSlateLayoutTransform(FVector2f(X + 9.f, PadY + BH - 13.f))),
					FText::FromString(LoopStr),
					FAppStyle::GetFontStyle("TinyText"),
					ESlateDrawEffect::None,
					TypeColor.CopyWithNewOpacity(0.65f));
			}
		}

		return LayerId + 4;
	}

	// ── Mouse ──────────────────────────────────────────────────────────────────

	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			OnTrackSelected.ExecuteIfBound();

			const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
			if (const FQTDStepData* Hit = HitTestStep(Local))
			{
				bIsDragging       = true;
				DraggedStepId     = Hit->StepId;
				PressedStepId     = Hit->StepId;
				DragStartMouseX   = Local.X;
				DragOrigStartTime = Hit->StartTime;
				Invalidate(EInvalidateWidgetReason::Paint);
				return FReply::Handled().CaptureMouse(AsShared());
			}
			return FReply::Handled();
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
		if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (bIsDragging)
			{
				bIsDragging = false;
				DraggedStepId.Invalidate();
			}
			if (PressedStepId.IsValid())
			{
				PressedStepId.Invalidate();
				Invalidate(EInvalidateWidgetReason::Paint);
			}
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());

		if (bIsDragging && DraggedStepId.IsValid())
		{
			float NewStart = DragOrigStartTime + (Local.X - DragStartMouseX) / PixelsPerSec;
			NewStart = FMath::RoundToFloat(NewStart / QTDEditorConstants::SnapIncrement)
			           * QTDEditorConstants::SnapIncrement;
			NewStart = FMath::Max(0.0f, NewStart);
			OnStepMoved.ExecuteIfBound(DraggedStepId, NewStart);
			Invalidate(EInvalidateWidgetReason::Paint);
			return FReply::Handled();
		}

		// Hover tracking (no capture needed — called whenever cursor is over widget)
		const FQTDStepData* Hit = HitTestStep(Local);
		const FGuid NewHovered  = Hit ? Hit->StepId : FGuid();
		if (NewHovered != HoveredStepId)
		{
			HoveredStepId = NewHovered;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		return FReply::Unhandled();
	}

	virtual void OnMouseLeave(const FPointerEvent&) override
	{
		bIsDragging = false;
		DraggedStepId.Invalidate();
		if (HoveredStepId.IsValid())
		{
			HoveredStepId.Invalidate();
			Invalidate(EInvalidateWidgetReason::Paint);
		}
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

		// ── Generic ───────────────────────────────────────────────────────
		SubBuilder.BeginSection("Generic", LOCTEXT("SecGeneric", "Generic"));
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
		S.Label = TEXT("Material Float");
		return S;
	}

	FQTDStepData MakeColorStep(EQTDColorTarget Target, FName SlotName, float T) const
	{
		FQTDStepData S = MakeBaseStep(EQTDStepType::LinearColor, SlotName, T);
		S.ColorTarget = Target;
		S.Label = TEXT("Material Color");
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
	bool                      bIsSelected  = false;

	FSimpleDelegate OnTrackSelected;
	FOnStepAdded    OnStepAdded;
	FOnStepEdit     OnStepEdit;
	FOnStepMoved    OnStepMoved;
	FOnStepDeleted  OnStepDeleted;

	bool  bIsDragging        = false;
	FGuid DraggedStepId;
	FGuid HoveredStepId;
	FGuid PressedStepId;
	float DragStartMouseX    = 0.0f;
	float DragOrigStartTime  = 0.0f;
};

// ──────────────────────────────────────────────────────────────────────────────
// SQTDTrackRow
// ──────────────────────────────────────────────────────────────────────────────

void SQTDTrackRow::Construct(const FArguments& InArgs)
{
	Track           = InArgs._Track;
	Asset           = InArgs._Asset;
	PixelsPerSec    = InArgs._PixelsPerSec;
	bIsSelected     = InArgs._IsSelected;
	OnTrackSelected = InArgs._OnTrackSelected;
	OnTrackDelete   = InArgs._OnTrackDelete;
	OnStepAdded     = InArgs._OnStepAdded;
	OnStepEdit      = InArgs._OnStepEdit;
	OnStepMoved     = InArgs._OnStepMoved;
	OnStepDeleted   = InArgs._OnStepDeleted;

	// Build the step content widget (placed externally in the shared H-scroll box).
	SAssignNew(StepContent, SQTDStepContent)
		.Track(Track)
		.Asset(Asset)
		.PixelsPerSec(PixelsPerSec)
		.IsSelected(bIsSelected)
		.OnTrackSelected(OnTrackSelected)
		.OnStepAdded(OnStepAdded)
		.OnStepEdit(OnStepEdit)
		.OnStepMoved(OnStepMoved)
		.OnStepDeleted(OnStepDeleted);

	const FString ClassShortName = Track.ComponentClass
		? Track.ComponentClass->GetName().Replace(TEXT("Component"), TEXT(""))
		: FString();

	const float LabelBg      = bIsSelected ? 0.115f : 0.095f;
	const float AccentAlpha  = bIsSelected ? 1.00f  : 0.50f;

	// Label column widget — placed in the fixed left column (no horizontal scroll).
	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(QTDEditorConstants::TrackLabelWidth)
		.HeightOverride(QTDEditorConstants::TrackHeight)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(0.f))
			.OnClicked_Lambda([this]() -> FReply {
				OnTrackSelected.ExecuteIfBound();
				return FReply::Handled();
			})
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(LabelBg, LabelBg, LabelBg))
				.Padding(FMargin(0.f))
				[
					SNew(SHorizontalBox)

					// Orange accent bar (left edge)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(QTDEditorConstants::LabelAccentWidth)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(FLinearColor(1.0f, 0.55f, 0.15f, AccentAlpha))
						]
					]

				// Icon
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 5.f, 0.f)
				[
					SNew(SImage)
					.Image(FSlateIconFinder::FindIconBrushForClass(
						Track.ComponentClass.Get(), TEXT("SCS.Component")))
					.DesiredSizeOverride(FVector2D(14.f, 14.f))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
					.Visibility(Track.ComponentClass ? EVisibility::Visible : EVisibility::Collapsed)
				]

				// Track name + class subtitle
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Track.TrackLabel))
						.Font(FAppStyle::GetFontStyle("SmallFont"))
						.ColorAndOpacity(FLinearColor(0.85f, 0.85f, 0.85f))
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(ClassShortName))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.38f, 0.38f, 0.38f))
						.Visibility(ClassShortName.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
					]
				]

				// Delete button (X icon)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ToolTipText(LOCTEXT("DeleteTrack", "Remove this track"))
					.OnClicked_Lambda([this]() -> FReply {
						OnTrackSelected.ExecuteIfBound();
						OnTrackDelete.ExecuteIfBound(Track.TrackId);
						return FReply::Handled();
					})
					.ContentPadding(FMargin(3.f, 2.f))
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush("Icons.X"))
						.DesiredSizeOverride(FVector2D(10.f, 10.f))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.22f, 0.22f)))
					]
				]
			]
		]  // SBorder
		]  // SButton
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
