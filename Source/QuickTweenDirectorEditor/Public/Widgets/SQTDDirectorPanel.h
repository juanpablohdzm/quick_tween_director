// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

class UBlueprint;
class UQuickTweenDirectorAsset;
class SQuickTweenDirectorEditor;
class SBorder;
class SBox;
class SVerticalBox;
class STextBlock;
class SScrollBox;

/**
 * Main content widget for the "QuickTween Director" nomad tab.
 *
 * Layout:
 *   ┌─ Header ─── Blueprint: MyActor ────────────────────────────────┐
 *   ├─ Left: Animation list ──┬─ Right: Timeline ────────────────────┤
 *   │  [+ New Animation]      │  (SQuickTweenDirectorEditor)          │
 *   │  ○ NewAnimation         │                                       │
 *   │  ● NewAnimation_1       │                                       │
 *   └─────────────────────────┴───────────────────────────────────────┘
 *
 * Call SetBlueprint() whenever the user focuses a different Actor Blueprint editor.
 */
class SQTDDirectorPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SQTDDirectorPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Called by the editor module when the active Actor Blueprint changes. */
	void SetBlueprint(UBlueprint* NewBlueprint);

private:

	// ── Animation list helpers ────────────────────────────────────────────────

	/** Rebuilds the left-side animation list from Blueprint->NewVariables. */
	void RefreshAnimationList();

	/** Selects an animation by index and updates the timeline panel. */
	void SelectAnimation(int32 Index);

	FReply OnNewAnimationClicked();
	void   OnDeleteAnimation(int32 Index);
	void   OnRenameAnimation(int32 Index, const FText& NewText, ETextCommit::Type CommitType);

	// ── Asset / variable management ───────────────────────────────────────────

	/**
	 * Collects all UQuickTweenDirectorAsset* Blueprint variables from the current Blueprint.
	 * Returns pairs of (VariableName, Asset pointer — may be null if unset).
	 */
	TArray<TPair<FName, UQuickTweenDirectorAsset*>> GetDirectorAnimations() const;

	/**
	 * Creates a new UQuickTweenDirectorAsset package alongside the Blueprint,
	 * adds it as a variable in the Blueprint, and compiles the Blueprint.
	 * Returns the created asset, or nullptr on failure.
	 */
	UQuickTweenDirectorAsset* CreateNewAnimation(const FString& VarName);

	/** Generates a unique variable name like "NewAnimation", "NewAnimation_1", … */
	FString MakeUniqueAnimationName() const;

	// ── Data ──────────────────────────────────────────────────────────────────

	TWeakObjectPtr<UBlueprint> Blueprint;
	int32 SelectedAnimIndex = INDEX_NONE;

	/** Ordered list matching the current animation variable list. */
	TArray<FName> AnimVarNames;

	// ── Widgets ───────────────────────────────────────────────────────────────

	TSharedPtr<SBorder>                  RootBorder;
	TSharedPtr<STextBlock>               BlueprintNameText;
	TSharedPtr<SVerticalBox>             AnimListContainer;
	TSharedPtr<SBox>                     TimelineArea;
	TSharedPtr<SQuickTweenDirectorEditor> TimelineEditor;
	TSharedPtr<SWidget>                  EmptyTimelineHint;
	TSharedPtr<SWidget>                  NoBlueprintHint;
	TSharedPtr<SWidget>                  MainContent;
};
