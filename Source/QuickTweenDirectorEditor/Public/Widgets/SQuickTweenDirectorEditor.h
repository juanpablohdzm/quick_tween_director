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
	static constexpr float TrackHeight         = 36.0f;
	/** Height of the ruler header in pixels. */
	static constexpr float RulerHeight         = 28.0f;
	/** Minimum step width so very short steps are still clickable. */
	static constexpr float MinStepWidth        = 8.0f;
	/** Snap increment in seconds when dragging steps. */
	static constexpr float SnapIncrement       = 0.05f;
	/** Fixed toolbar height. */
	static constexpr float ToolbarHeight       = 32.0f;
	/** Label column left border accent width (px). */
	static constexpr float LabelAccentWidth    = 2.0f;
}

// ──────────────────────────────────────────────────────────────────────────────
// SQuickTweenDirectorEditor
// ──────────────────────────────────────────────────────────────────────────────

class SQuickTweenDirectorEditor : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQuickTweenDirectorEditor) {}
		SLATE_ARGUMENT(UQuickTweenDirectorAsset*, Asset)
		SLATE_ARGUMENT(TWeakObjectPtr<UBlueprint>, Blueprint)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Rebuilds the track list from the asset. Call after structural changes. */
	void RefreshFromAsset();

private:

	// ── Toolbar ───────────────────────────────────────────────────────────────
	FReply OnSaveClicked();
	FReply OnAddTrackClicked();
	FReply OnExportJsonClicked();

	// ── Track / step actions ──────────────────────────────────────────────────
	void DeleteTrack(FGuid TrackId);
	void ReorderTrack(FGuid TrackId, int32 Direction);
	void AddStepToTrack(FQTDStepData PrefilledStep);
	void OpenStepDialog(const FQTDStepData& Step);

	// ── Zoom ──────────────────────────────────────────────────────────────────
	float PixelsPerSec = QTDEditorConstants::DefaultPixelsPerSec;
	float OnGetZoomValue() const { return PixelsPerSec; }
	void  OnZoomChanged(float NewValue);
	FReply OnZoomToFitClicked();

	// ── Snap ──────────────────────────────────────────────────────────────────
	bool bSnapEnabled = false;
	void PropagateSnapToRows();

	// ── Playback ──────────────────────────────────────────────────────────────
	float PlayheadTime = 0.f;
	bool  bIsPlaying   = false;

	FReply OnPlayClicked();
	FReply OnPauseClicked();
	FReply OnStopClicked();

	EActiveTimerReturnType HandlePlaybackTick(double InCurrentTime, float InDeltaTime);

	// ── Data ──────────────────────────────────────────────────────────────────
	UQuickTweenDirectorAsset*    Asset           = nullptr;
	TWeakObjectPtr<UBlueprint>   Blueprint;
	FGuid                        SelectedTrackId;

	TSharedPtr<SVerticalBox>     LabelContainer;
	TSharedPtr<SVerticalBox>     ContentContainer;
	TSharedPtr<SScrollBox>       HScrollBox;
	TSharedPtr<SWidget>          RulerWidget;

	/** Weak pointers to the live track row widgets — for propagating snap changes. */
	TArray<TWeakPtr<class SQTDTrackRow>> TrackRows;
};
