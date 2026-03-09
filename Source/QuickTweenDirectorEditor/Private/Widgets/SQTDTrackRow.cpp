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
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "SQTDTrackRow"

// ──────────────────────────────────────────────────────────────────────────────
// Clipboard — module-static so copy/paste works across tracks
// ──────────────────────────────────────────────────────────────────────────────

static TOptional<FQTDStepData> GQTDStepClipboard;

// ──────────────────────────────────────────────────────────────────────────────
// Easing helper — evaluate an ease function for the curve preview
// ──────────────────────────────────────────────────────────────────────────────

static float EvalEase(EEaseType Type, float t)
{
	t = FMath::Clamp(t, 0.f, 1.f);
	switch (Type)
	{
		case EEaseType::InSine:     return 1.f - FMath::Cos(t * UE_PI * 0.5f);
		case EEaseType::OutSine:    return FMath::Sin(t * UE_PI * 0.5f);
		case EEaseType::InOutSine:  return -(FMath::Cos(UE_PI * t) - 1.f) * 0.5f;
		case EEaseType::InQuad:     return t * t;
		case EEaseType::OutQuad:    return 1.f - (1.f - t) * (1.f - t);
		case EEaseType::InOutQuad:  return t < 0.5f ? 2.f * t * t : 1.f - FMath::Pow(-2.f * t + 2.f, 2.f) * 0.5f;
		case EEaseType::InCubic:    return t * t * t;
		case EEaseType::OutCubic:   return 1.f - FMath::Pow(1.f - t, 3.f);
		case EEaseType::InOutCubic: return t < 0.5f ? 4.f * t * t * t : 1.f - FMath::Pow(-2.f * t + 2.f, 3.f) * 0.5f;
		case EEaseType::InQuart:    return t * t * t * t;
		case EEaseType::OutQuart:   return 1.f - FMath::Pow(1.f - t, 4.f);
		case EEaseType::InOutQuart: return t < 0.5f ? 8.f * t * t * t * t : 1.f - FMath::Pow(-2.f * t + 2.f, 4.f) * 0.5f;
		case EEaseType::InQuint:    return t * t * t * t * t;
		case EEaseType::OutQuint:   return 1.f - FMath::Pow(1.f - t, 5.f);
		case EEaseType::InOutQuint: return t < 0.5f ? 16.f * t * t * t * t * t : 1.f - FMath::Pow(-2.f * t + 2.f, 5.f) * 0.5f;
		default:                    return t; // Linear and everything else
	}
}

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
		SLATE_ARGUMENT(bool,                      SnapEnabled)
		SLATE_EVENT (FSimpleDelegate,             OnTrackSelected)
		SLATE_EVENT (FOnStepAdded,                OnStepAdded)
		SLATE_EVENT (FOnStepEdit,                 OnStepEdit)
		SLATE_EVENT (FOnStepMoved,                OnStepMoved)
		SLATE_EVENT (FOnStepDeleted,              OnStepDeleted)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		Track           = InArgs._Track;
		Asset           = InArgs._Asset;
		PixelsPerSec    = InArgs._PixelsPerSec;
		bIsSelected     = InArgs._IsSelected;
		bSnapEnabled    = InArgs._SnapEnabled;
		OnTrackSelected = InArgs._OnTrackSelected;
		OnStepAdded     = InArgs._OnStepAdded;
		OnStepEdit      = InArgs._OnStepEdit;
		OnStepMoved     = InArgs._OnStepMoved;
		OnStepDeleted   = InArgs._OnStepDeleted;
	}

	void SetPixelsPerSec(float PPS) { PixelsPerSec = PPS; Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout); }
	void SetSnapEnabled(bool bEnabled) { bSnapEnabled = bEnabled; }

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
		for (const FStepRenderInfo& Info : BuildRenderList())
		{
			if (!Info.Step) continue;

			const float PadY = 3.f;
			const float BH   = H - PadY * 2.f;
			const float X    = Info.X;
			const float SW   = Info.W;

			const FLinearColor TypeColor = Info.Step->GetDisplayColor();
			const bool bHovered    = (HoveredStepId == Info.Step->StepId);
			const bool bPressed    = (PressedStepId  == Info.Step->StepId);
			const bool bDragged    = Info.bIsDragged;
			const bool bMultiSel   = SelectedStepIds.Contains(Info.Step->StepId);
			const float FillOpacity = bDragged ? 0.36f : (bPressed ? 0.28f : (bHovered ? 0.22f : 0.16f));
			const float TopOpacity  = bDragged ? 0.90f : (bPressed ? 0.60f : (bHovered ? 0.50f : 0.35f));
			const float TopLineW    = (bDragged || bMultiSel) ? 2.f : 1.f;

			// ── Tinted fill ────────────────────────────────────────────────────
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(SW - 1.f, BH),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, TypeColor.CopyWithNewOpacity(FillOpacity));

			// ── Left accent stripe ─────────────────────────────────────────────
			FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(4.f, BH),
					FSlateLayoutTransform(FVector2f(X, PadY))),
				FAppStyle::GetBrush("WhiteBrush"),
				ESlateDrawEffect::None, TypeColor);

			// ── Multi-select outline ───────────────────────────────────────────
			if (bMultiSel)
			{
				TArray<FVector2D> Outline = {
					FVector2D(X, PadY), FVector2D(X + SW - 1.f, PadY),
					FVector2D(X + SW - 1.f, PadY + BH), FVector2D(X, PadY + BH), FVector2D(X, PadY)
				};
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
					AllottedGeometry.ToPaintGeometry(), Outline,
					ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.60f), true, 1.5f);
			}

			// ── Top edge highlight ─────────────────────────────────────────────
			TArray<FVector2D> TopPts = { FVector2D(X + 4.f, PadY), FVector2D(X + SW - 1.f, PadY) };
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2,
				AllottedGeometry.ToPaintGeometry(), TopPts,
				ESlateDrawEffect::None, TypeColor.CopyWithNewOpacity(TopOpacity), true, TopLineW);

			// ── Easing curve preview (when block is wide enough) ───────────────
			if (SW > 80.f && Info.Step->EaseType != EEaseType::Linear)
			{
				const int32 Samples = 20;
				TArray<FVector2D> CurvePts;
				CurvePts.Reserve(Samples + 1);
				for (int32 s = 0; s <= Samples; ++s)
				{
					const float T    = (float)s / Samples;
					const float EVal = EvalEase(Info.Step->EaseType, T);
					const float CX   = X + 4.f + T * (SW - 8.f);
					// Invert Y: 0=bottom, 1=top within the block
					const float CY   = PadY + BH * (1.f - EVal * 0.65f);
					CurvePts.Add(FVector2D(CX, CY));
				}
				FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(), CurvePts,
					ESlateDrawEffect::None, TypeColor.CopyWithNewOpacity(0.40f), true, 1.f);
			}

			// ── Resize handle indicator (right edge, when hovered) ─────────────
			if (bHovered && !bIsDragging && !bIsResizing)
			{
				FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(3.f, BH),
						FSlateLayoutTransform(FVector2f(X + SW - 4.f, PadY))),
					FAppStyle::GetBrush("WhiteBrush"),
					ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, 0.20f));
			}

			// ── Step label ────────────────────────────────────────────────────
			if (SW > 18.f)
			{
				const FText LabelText = FText::FromString(
					Info.Step->Label.IsEmpty()
						? UEnum::GetDisplayValueAsText(Info.Step->StepType).ToString()
						: Info.Step->Label);
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(FMath::Max(SW - 16.f, 4.f), BH - 4.f),
						FSlateLayoutTransform(FVector2f(X + 9.f, PadY + 4.f))),
					LabelText,
					FAppStyle::GetFontStyle("SmallFont"),
					ESlateDrawEffect::None,
					FLinearColor(1.f, 1.f, 1.f, bDragged ? 1.0f : 0.88f));
			}

			// ── Duration chip (bottom-right) ───────────────────────────────────
			if (SW > 52.f)
			{
				FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
					AllottedGeometry.ToPaintGeometry(
						FVector2f(46.f, 12.f),
						FSlateLayoutTransform(FVector2f(X + SW - 50.f, PadY + BH - 13.f))),
					FText::FromString(FString::Printf(TEXT("%.2fs"), Info.Step->Duration)),
					FAppStyle::GetFontStyle("TinyText"),
					ESlateDrawEffect::None,
					TypeColor.CopyWithNewOpacity(0.75f));
			}

			// ── Loop badge ────────────────────────────────────────────────────
			if (SW > 28.f && Info.Step->Loops != 1)
			{
				const FString LoopStr = (Info.Step->Loops < 0)
					? TEXT("∞") : FString::Printf(TEXT("×%d"), Info.Step->Loops);
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
			const bool bCtrl = Event.IsControlDown();

			if (const FQTDStepData* Hit = HitTestStep(Local))
			{
				// Check if cursor is within resize zone (right 6 px of step)
				const float StepX = Hit->StartTime * PixelsPerSec;
				const float StepW = FMath::Max(Hit->Duration * PixelsPerSec * FMath::Max(Hit->Loops, 1),
				                               QTDEditorConstants::MinStepWidth);
				const bool bNearRightEdge = (Local.X >= StepX + StepW - 6.f);

				if (bNearRightEdge)
				{
					// Begin resize
					bIsResizing            = true;
					ResizedStepId          = Hit->StepId;
					ResizeOriginalDuration = Hit->Duration;
					ResizeStartMouseX      = Local.X;
					Invalidate(EInvalidateWidgetReason::Paint);
					return FReply::Handled().CaptureMouse(AsShared());
				}

				// Multi-select toggle (Ctrl+click)
				if (bCtrl)
				{
					if (SelectedStepIds.Contains(Hit->StepId))
						SelectedStepIds.Remove(Hit->StepId);
					else
						SelectedStepIds.Add(Hit->StepId);
					Invalidate(EInvalidateWidgetReason::Paint);
					return FReply::Handled();
				}

				// Clear multi-select if clicking a non-selected step without Ctrl
				if (!SelectedStepIds.Contains(Hit->StepId))
					SelectedStepIds.Empty();

				// Begin drag-move
				bIsDragging   = true;
				DraggedStepId = Hit->StepId;
				PressedStepId = Hit->StepId;

				const TArray<FQTDStepData*> Sorted = GetSortedTrackSteps();
				DragTargetIndex = 0;
				for (int32 i = 0; i < Sorted.Num(); ++i)
				{
					if (Sorted[i] && Sorted[i]->StepId == Hit->StepId)
					{
						DragTargetIndex = i;
						break;
					}
				}

				Invalidate(EInvalidateWidgetReason::Paint);
				return FReply::Handled().CaptureMouse(AsShared());
			}

			// Clicked on empty area — clear selection
			if (!bCtrl) SelectedStepIds.Empty();
			Invalidate(EInvalidateWidgetReason::Paint);
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
			// Commit resize
			if (bIsResizing && ResizedStepId.IsValid() && Asset)
			{
				if (FQTDStepData* Step = Asset->FindStep(ResizedStepId))
				{
					const float MinDur = FMath::Max(0.05f, QTDEditorConstants::MinStepWidth / PixelsPerSec);
					Step->Duration = FMath::Max(Step->Duration, MinDur);
					FScopedTransaction Tx(LOCTEXT("ResizeStep", "Resize Step"));
					Asset->Modify();
					Asset->MarkPackageDirty();
				}
			}

			// Commit reorder drag
			if (bIsDragging && DraggedStepId.IsValid() && Asset)
			{
				FScopedTransaction Tx(LOCTEXT("MoveStep", "Move Step"));
				Asset->Modify();

				TArray<FQTDStepData*> Sorted = GetSortedTrackSteps();
				TArray<FQTDStepData*> OtherSteps;
				FQTDStepData* DraggedStep = nullptr;
				for (FQTDStepData* S : Sorted)
				{
					if (S && S->StepId == DraggedStepId) DraggedStep = S;
					else if (S) OtherSteps.Add(S);
				}
				if (DraggedStep)
				{
					const int32 SafeTarget = FMath::Clamp(DragTargetIndex, 0, OtherSteps.Num());
					TArray<FQTDStepData*> FinalOrder;
					FinalOrder.Reserve(Sorted.Num());
					for (int32 i = 0; i < OtherSteps.Num(); ++i)
					{
						if (i == SafeTarget) FinalOrder.Add(DraggedStep);
						FinalOrder.Add(OtherSteps[i]);
					}
					if (SafeTarget >= OtherSteps.Num()) FinalOrder.Add(DraggedStep);

					float Time = 0.f;
					for (FQTDStepData* S : FinalOrder)
					{
						S->StartTime = Time;
						Time += S->Duration * FMath::Max(S->Loops, 1);
					}
					Asset->MarkPackageDirty();
				}
			}

			bIsResizing   = false;
			bIsDragging   = false;
			DraggedStepId.Invalidate();
			ResizedStepId.Invalidate();
			PressedStepId.Invalidate();
			Invalidate(EInvalidateWidgetReason::Paint);
			return FReply::Handled().ReleaseMouseCapture();
		}
		return FReply::Unhandled();
	}

	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
	{
		const FVector2D Local = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());

		// Resize drag
		if (bIsResizing && ResizedStepId.IsValid() && Asset)
		{
			if (FQTDStepData* Step = Asset->FindStep(ResizedStepId))
			{
				const float DeltaX = Local.X - ResizeStartMouseX;
				float NewDuration   = ResizeOriginalDuration + DeltaX / PixelsPerSec;
				if (bSnapEnabled)
					NewDuration = FMath::RoundToFloat(NewDuration / QTDEditorConstants::SnapIncrement) * QTDEditorConstants::SnapIncrement;
				NewDuration = FMath::Max(NewDuration, QTDEditorConstants::MinStepWidth / PixelsPerSec);
				Step->Duration = NewDuration;
				Invalidate(EInvalidateWidgetReason::Paint | EInvalidateWidgetReason::Layout);
			}
			return FReply::Handled();
		}

		// Move drag
		if (bIsDragging && DraggedStepId.IsValid())
		{
			const int32 NewTarget = ComputeDragTargetIndex(Local.X);
			if (NewTarget != DragTargetIndex)
			{
				DragTargetIndex = NewTarget;
			}
			Invalidate(EInvalidateWidgetReason::Paint);
			return FReply::Handled();
		}

		// Hover + resize cursor
		const FQTDStepData* Hit = HitTestStep(Local);
		const FGuid NewHovered  = Hit ? Hit->StepId : FGuid();
		if (NewHovered != HoveredStepId)
		{
			HoveredStepId = NewHovered;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		// Show resize cursor when near a step's right edge
		if (Hit)
		{
			const float StepX = Hit->StartTime * PixelsPerSec;
			const float StepW = FMath::Max(Hit->Duration * PixelsPerSec * FMath::Max(Hit->Loops, 1),
			                               QTDEditorConstants::MinStepWidth);
			if (Local.X >= StepX + StepW - 6.f)
			{
				return FReply::Handled().SetMouseCursor(EMouseCursor::ResizeLeftRight);
			}
		}

		return FReply::Unhandled();
	}

	virtual void OnMouseLeave(const FPointerEvent&) override
	{
		bIsDragging = false;
		bIsResizing = false;
		DraggedStepId.Invalidate();
		ResizedStepId.Invalidate();
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

	// ── Snapping ───────────────────────────────────────────────────────────────

	float SnapTime(float T) const
	{
		if (!bSnapEnabled) return T;
		return FMath::RoundToFloat(T / QTDEditorConstants::SnapIncrement) * QTDEditorConstants::SnapIncrement;
	}

	// ── Render helpers ─────────────────────────────────────────────────────────

	struct FStepRenderInfo
	{
		const FQTDStepData* Step       = nullptr;
		float               X          = 0.f;
		float               W          = 0.f;
		bool                bIsDragged = false;
	};

	TArray<FQTDStepData*> GetSortedTrackSteps() const
	{
		if (!Asset) return {};
		TArray<FQTDStepData*> Steps = Asset->GetStepsForTrack(Track.TrackId);
		Steps.Sort([](const FQTDStepData& A, const FQTDStepData& B) {
			return A.StartTime < B.StartTime;
		});
		return Steps;
	}

	int32 ComputeDragTargetIndex(float CursorX) const
	{
		float T = 0.f;
		int32 Target = 0;
		for (FQTDStepData* S : GetSortedTrackSteps())
		{
			if (!S || S->StepId == DraggedStepId) continue;
			const float Dur  = S->Duration * FMath::Max(S->Loops, 1);
			const float MidX = (T + Dur * 0.5f) * PixelsPerSec;
			if (CursorX > MidX) ++Target;
			T += Dur;
		}
		return Target;
	}

	TArray<FStepRenderInfo> BuildRenderList() const
	{
		TArray<FStepRenderInfo> List;
		if (!Asset) return List;

		TArray<FQTDStepData*> Sorted = GetSortedTrackSteps();

		if (!bIsDragging || !DraggedStepId.IsValid())
		{
			for (const FQTDStepData* Step : Sorted)
			{
				if (!Step) continue;
				FStepRenderInfo Info;
				Info.Step = Step;
				Info.X    = Step->StartTime * PixelsPerSec;
				Info.W    = FMath::Max(Step->Duration * PixelsPerSec * FMath::Max(Step->Loops, 1),
				                       QTDEditorConstants::MinStepWidth);
				List.Add(Info);
			}
			return List;
		}

		// During drag: show preview reorder
		TArray<FQTDStepData*> OtherSteps;
		FQTDStepData* DraggedStep = nullptr;
		for (FQTDStepData* S : Sorted)
		{
			if (!S) continue;
			if (S->StepId == DraggedStepId) DraggedStep = S;
			else OtherSteps.Add(S);
		}
		if (!DraggedStep) return List;

		const int32 SafeTarget = FMath::Clamp(DragTargetIndex, 0, OtherSteps.Num());
		TArray<FQTDStepData*> FinalOrder;
		FinalOrder.Reserve(Sorted.Num());
		for (int32 i = 0; i < OtherSteps.Num(); ++i)
		{
			if (i == SafeTarget) FinalOrder.Add(DraggedStep);
			FinalOrder.Add(OtherSteps[i]);
		}
		if (SafeTarget >= OtherSteps.Num()) FinalOrder.Add(DraggedStep);

		float T = 0.f;
		for (FQTDStepData* S : FinalOrder)
		{
			const float Dur = S->Duration * FMath::Max(S->Loops, 1);
			FStepRenderInfo Info;
			Info.Step       = S;
			Info.X          = T * PixelsPerSec;
			Info.W          = FMath::Max(Dur * PixelsPerSec, QTDEditorConstants::MinStepWidth);
			Info.bIsDragged = (S->StepId == DraggedStepId);
			List.Add(Info);
			T += Dur;
		}
		return List;
	}

	// ── Context menu ──────────────────────────────────────────────────────────

	void ShowContextMenu(FVector2D ScreenPos, FVector2D LocalPos)
	{
		const FQTDStepData* HitStep = HitTestStep(LocalPos);
		const bool bHasSelection    = SelectedStepIds.Num() > 1;

		float NewStepStart = 0.f;
		if (Asset)
		{
			for (const FQTDStepData* S : Asset->GetStepsForTrack(Track.TrackId))
			{
				if (S)
				{
					const float StepEnd = S->StartTime + S->Duration * FMath::Max(S->Loops, 1);
					NewStepStart = FMath::Max(NewStepStart, StepEnd);
				}
			}
		}

		FMenuBuilder Builder(true, nullptr);

		if (HitStep)
		{
			const FGuid StepId = HitStep->StepId;

			Builder.AddMenuEntry(
				LOCTEXT("CopyStep", "Copy Step"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, HitStep]()
				{
					GQTDStepClipboard = *HitStep;
				})));

			if (bHasSelection)
			{
				Builder.AddMenuEntry(
					LOCTEXT("DeleteSelected", "Delete Selected"),
					FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this]()
					{
						if (!Asset) return;
						FScopedTransaction Tx(LOCTEXT("DeleteSelectedSteps", "Delete Selected Steps"));
						Asset->Modify();
						for (const FGuid& Id : SelectedStepIds)
						{
							Asset->RemoveStep(Id);
						}
						SelectedStepIds.Empty();
						Asset->MarkPackageDirty();
					})));
			}

			Builder.AddMenuEntry(
				LOCTEXT("DeleteStep", "Delete Step"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([this, StepId]() {
					OnStepDeleted.ExecuteIfBound(StepId);
				})));
		}
		else
		{
			Builder.AddSubMenu(
				LOCTEXT("AddStep", "Add Step"),
				LOCTEXT("AddStepTip", "Add a new step at the end of this track"),
				FNewMenuDelegate::CreateSP(this, &SQTDStepContent::BuildAddStepSubmenu, NewStepStart)
			);

			if (GQTDStepClipboard.IsSet())
			{
				const float PasteTime = SnapTime(LocalPos.X / PixelsPerSec);
				Builder.AddMenuEntry(
					LOCTEXT("PasteStep", "Paste Step"),
					FText::GetEmpty(), FSlateIcon(),
					FUIAction(FExecuteAction::CreateLambda([this, PasteTime]()
					{
						if (!Asset || !GQTDStepClipboard.IsSet()) return;
						FQTDStepData NewStep = GQTDStepClipboard.GetValue();
						NewStep.StepId    = FGuid::NewGuid();
						NewStep.TrackId   = Track.TrackId;
						NewStep.SlotName  = Track.ComponentVariableName;
						NewStep.StartTime = FMath::Max(0.f, PasteTime);
						FScopedTransaction Tx(LOCTEXT("PasteStepTx", "Paste Step"));
						Asset->Modify();
						OnStepAdded.ExecuteIfBound(NewStep);
					})));
			}
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

		if (bIsScene)
		{
			SubBuilder.BeginSection("Location", LOCTEXT("SecLocation", "Location"));
			SubBuilder.AddMenuEntry(LOCTEXT("RelLoc", "Relative Location"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::RelativeLocation, SlotName, SnapTime(ClickTime)))));
			SubBuilder.AddMenuEntry(LOCTEXT("WorldLoc", "World Location"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::WorldLocation, SlotName, SnapTime(ClickTime)))));
			SubBuilder.EndSection();

			SubBuilder.BeginSection("Rotation", LOCTEXT("SecRotation", "Rotation"));
			SubBuilder.AddMenuEntry(LOCTEXT("RelRot", "Relative Rotation"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeRotatorStep(EQTDRotatorProperty::RelativeRotation, SlotName, SnapTime(ClickTime)))));
			SubBuilder.AddMenuEntry(LOCTEXT("WorldRot", "World Rotation"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeRotatorStep(EQTDRotatorProperty::WorldRotation, SlotName, SnapTime(ClickTime)))));
			SubBuilder.EndSection();

			SubBuilder.BeginSection("Scale", LOCTEXT("SecScale", "Scale"));
			SubBuilder.AddMenuEntry(LOCTEXT("RelScale", "Relative Scale"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::RelativeScale3D, SlotName, SnapTime(ClickTime)))));
			SubBuilder.AddMenuEntry(LOCTEXT("WorldScale", "World Scale"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeVectorStep(EQTDVectorProperty::WorldScale3D, SlotName, SnapTime(ClickTime)))));
			SubBuilder.EndSection();
		}

		if (bIsPrimitive || !bIsScene)
		{
			SubBuilder.BeginSection("Material", LOCTEXT("SecMaterial", "Material"));
			SubBuilder.AddMenuEntry(LOCTEXT("MatFloat", "Material Scalar Parameter"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeFloatStep(EQTDFloatTarget::MaterialScalar, SlotName, SnapTime(ClickTime)))));
			SubBuilder.AddMenuEntry(LOCTEXT("MatColor", "Material Vector Parameter (Color)"), FText::GetEmpty(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
					MakeColorStep(EQTDColorTarget::MaterialVector, SlotName, SnapTime(ClickTime)))));
			SubBuilder.EndSection();
		}

		SubBuilder.BeginSection("Generic", LOCTEXT("SecGeneric", "Generic"));
		SubBuilder.AddMenuEntry(LOCTEXT("Delay", "Delay (Empty)"), FText::GetEmpty(), FSlateIcon(),
			FUIAction(FExecuteAction::CreateSP(this, &SQTDStepContent::FireStepAdded,
				MakeEmptyStep(SlotName, SnapTime(ClickTime)))));
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
	bool                      bSnapEnabled = false;

	FSimpleDelegate OnTrackSelected;
	FOnStepAdded    OnStepAdded;
	FOnStepEdit     OnStepEdit;
	FOnStepMoved    OnStepMoved;
	FOnStepDeleted  OnStepDeleted;

	// Drag-move
	bool  bIsDragging    = false;
	FGuid DraggedStepId;
	FGuid HoveredStepId;
	FGuid PressedStepId;
	int32 DragTargetIndex = 0;

	// Resize
	bool  bIsResizing           = false;
	FGuid ResizedStepId;
	float ResizeOriginalDuration = 1.f;
	float ResizeStartMouseX      = 0.f;

	// Multi-select
	TSet<FGuid> SelectedStepIds;
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
	OnTrackReorder  = InArgs._OnTrackReorder;
	OnStepAdded     = InArgs._OnStepAdded;
	OnStepEdit      = InArgs._OnStepEdit;
	OnStepMoved     = InArgs._OnStepMoved;
	OnStepDeleted   = InArgs._OnStepDeleted;

	SAssignNew(StepContent, SQTDStepContent)
		.Track(Track)
		.Asset(Asset)
		.PixelsPerSec(PixelsPerSec)
		.IsSelected(bIsSelected)
		.SnapEnabled(false)
		.OnTrackSelected(OnTrackSelected)
		.OnStepAdded(OnStepAdded)
		.OnStepEdit(OnStepEdit)
		.OnStepMoved(OnStepMoved)
		.OnStepDeleted(OnStepDeleted);

	const FString ClassShortName = Track.ComponentClass
		? Track.ComponentClass->GetName().Replace(TEXT("Component"), TEXT(""))
		: FString();

	const float LabelBg     = bIsSelected ? 0.115f : 0.095f;
	const float AccentAlpha = bIsSelected ? 1.00f  : 0.50f;

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

					// Orange accent bar
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

					// Reorder arrows (up / down)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(2.f, 1.f))
							.ToolTipText(LOCTEXT("MoveUp", "Move track up"))
							.OnClicked_Lambda([this]() -> FReply {
								OnTrackReorder.ExecuteIfBound(Track.TrackId, -1);
								return FReply::Handled();
							})
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.ChevronUp"))
								.DesiredSizeOverride(FVector2D(8.f, 8.f))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), "SimpleButton")
							.ContentPadding(FMargin(2.f, 1.f))
							.ToolTipText(LOCTEXT("MoveDown", "Move track down"))
							.OnClicked_Lambda([this]() -> FReply {
								OnTrackReorder.ExecuteIfBound(Track.TrackId, +1);
								return FReply::Handled();
							})
							[
								SNew(SImage)
								.Image(FAppStyle::GetBrush("Icons.ChevronDown"))
								.DesiredSizeOverride(FVector2D(8.f, 8.f))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.5f, 0.5f)))
							]
						]
					]

					// Delete button
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

void SQTDTrackRow::SetSnapEnabled(bool bEnabled)
{
	if (StepContent.IsValid()) StepContent->SetSnapEnabled(bEnabled);
}

TSharedRef<SWidget> SQTDTrackRow::GetStepContentWidget() const
{
	return StepContent.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE
