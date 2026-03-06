// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

class UBlueprint;

/**
 * Modal dialog that shows the component hierarchy of an Actor Blueprint and
 * lets the user pick one component to animate. Opened when "Add Track" is clicked
 * in the Director panel timeline.
 */
class SQTDComponentPickerDialog : public SCompoundWidget
{
public:

	/** Result written when the dialog is dismissed. */
	struct FResult
	{
		/** The Blueprint variable name of the chosen component (e.g. "StaticMeshComponent0"). */
		FName VariableName;
		/** The component's class (used to filter valid animation types). */
		TSubclassOf<UActorComponent> ComponentClass;
		/** Display name shown in the track label. */
		FString DisplayName;
		/** True when the user hit OK, false when they cancelled. */
		bool bConfirmed = false;
	};

	SLATE_BEGIN_ARGS(SQTDComponentPickerDialog) {}
		SLATE_ARGUMENT(UBlueprint*, Blueprint)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	const FResult& GetResult() const { return Result; }

private:

	// ── Component tree node ───────────────────────────────────────────────────

	struct FComponentNode
	{
		FName   VariableName;
		TSubclassOf<UActorComponent> ComponentClass;
		FString DisplayName;
		TArray<TSharedPtr<FComponentNode>> Children;
	};

	// ── Tree view helpers ─────────────────────────────────────────────────────

	void BuildComponentTree();

	TSharedRef<ITableRow> GenerateRow(TSharedPtr<FComponentNode> Item,
	                                  const TSharedRef<STableViewBase>& OwnerTable);

	void GetChildren(TSharedPtr<FComponentNode> Item,
	                 TArray<TSharedPtr<FComponentNode>>& OutChildren);

	void OnSelectionChanged(TSharedPtr<FComponentNode> Item,
	                        ESelectInfo::Type SelectInfo);

	void OnItemDoubleClicked(TSharedPtr<FComponentNode> Item);

	// ── Buttons ───────────────────────────────────────────────────────────────

	FReply OnConfirm();
	FReply OnCancel();
	bool   CanConfirm() const { return SelectedNode.IsValid(); }

	// ── Data ──────────────────────────────────────────────────────────────────

	UBlueprint* Blueprint = nullptr;
	TWeakPtr<SWindow> WeakParentWindow;

	TArray<TSharedPtr<FComponentNode>> RootNodes;
	TSharedPtr<STreeView<TSharedPtr<FComponentNode>>> TreeView;
	TSharedPtr<FComponentNode> SelectedNode;

	FResult Result;
};
