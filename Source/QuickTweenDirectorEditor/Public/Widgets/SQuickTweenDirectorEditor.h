// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "QTDStepData.h"

class UBlueprint;
class UQuickTweenDirectorAsset;

// ──────────────────────────────────────────────────────────────────────────────
// Constants shared between the editor and its sub-widgets
// ──────────────────────────────────────────────────────────────────────────────

namespace QTDEditorConstants
{
	/** Left panel width (track labels). */
	static constexpr float TrackLabelWidth     = 200.0f;
	/** Pixels per second in the timeline content area. */
	static constexpr float DefaultPixelsPerSec = 80.0f;
	/** Height of each track row in pixels. */
	static constexpr float TrackHeight         = 40.0f;
	/** Height of the ruler header in pixels. */
	static constexpr float RulerHeight         = 24.0f;
	/** Minimum step width so very short steps are still clickable. */
	static constexpr float MinStepWidth        = 6.0f;
	/** Snap increment in seconds when dragging steps. */
	static constexpr float SnapIncrement       = 0.05f;
}

// ──────────────────────────────────────────────────────────────────────────────
// SQuickTweenDirectorEditor
// ──────────────────────────────────────────────────────────────────────────────

/**
 * Timeline editor widget for a single UQuickTweenDirectorAsset.
 * Hosted inside SQTDDirectorPanel — does not own its own window or toolkit.
 *
 * Layout:
 *   ┌─ Toolbar ──────────────────────────────────────────────────────┐
 *   │  [Save]  [+ Add Track]          Duration: X.XXs  Zoom: [──]   │
 *   ├─ Ruler ────────────────────────────────────────────────────────┤
 *   ├─ Tracks ───────────────────────────────────────────────────────┤
 *   │  Component label + type  │  step boxes (drag/drop)             │
 *   └────────────────────────────────────────────────────────────────┘
 */
class SQuickTweenDirectorEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuickTweenDirectorEditor) {}
		SLATE_ARGUMENT(UQuickTweenDirectorAsset*, Asset)
		/** Weak ref to the Blueprint owning this asset — used by the component picker. */
		SLATE_ARGUMENT(TWeakObjectPtr<UBlueprint>, Blueprint)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds the track list from the asset. Call after structural changes. */
	void RefreshFromAsset();

private:

	// ── Toolbar ───────────────────────────────────────────────────────────────
	FReply OnSaveClicked();
	FReply OnAddTrackClicked();

	// ── Track / step actions ──────────────────────────────────────────────────
	void DeleteTrack(FGuid TrackId);

	/**
	 * Called when the user picks an animation type from the context submenu.
	 * @param PrefilledStep  Step already configured with type, SlotName and sub-property.
	 *                       StartTime is set to the click position.
	 */
	void AddStepToTrack(FQTDStepData PrefilledStep);

	void OpenStepDialog(const FQTDStepData& Step);

	// ── Zoom ──────────────────────────────────────────────────────────────────
	float PixelsPerSec = QTDEditorConstants::DefaultPixelsPerSec;
	float OnGetZoomValue()     const { return PixelsPerSec; }
	void  OnZoomChanged(float NewValue);

	// ── Data ──────────────────────────────────────────────────────────────────
	UQuickTweenDirectorAsset*    Asset     = nullptr;
	TWeakObjectPtr<UBlueprint>   Blueprint;

	TSharedPtr<SScrollBox>       TrackScrollBox;
	TSharedPtr<SVerticalBox>     TrackContainer;
	TSharedPtr<SWidget>          RulerWidget;
	TSharedPtr<SScrollBar>       HScrollBar;
};
