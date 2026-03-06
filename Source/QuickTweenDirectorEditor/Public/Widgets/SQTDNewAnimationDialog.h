// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Utils/LoopType.h"

class SWindow;

/**
 * Modal dialog shown when the user clicks "+ New Animation".
 * Collects the animation name and global playback settings.
 */
class SQTDNewAnimationDialog : public SCompoundWidget
{
public:
	struct FResult
	{
		FString   Name;
		int32     Loops           = 1;
		ELoopType LoopType        = ELoopType::Restart;
		bool      bAutoKill       = true;
		bool      bPlayWhilePaused = false;
		bool      bConfirmed      = false;
	};

	SLATE_BEGIN_ARGS(SQTDNewAnimationDialog) {}
		SLATE_ARGUMENT(FString,             DefaultName)
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	const FResult& GetResult() const { return Result; }

private:
	FReply OnConfirm();
	FReply OnCancel();

	FResult Result;
	TWeakPtr<SWindow> WeakParentWindow;

	TSharedPtr<SEditableTextBox>        NameBox;
	TSharedPtr<SEditableTextBox>        LoopsBox;
	TArray<TSharedPtr<FString>>         LoopTypeOptions;
	int32                               SelectedLoopTypeIndex = 0;
	TSharedPtr<STextBlock>              LoopTypeDisplay;
};
