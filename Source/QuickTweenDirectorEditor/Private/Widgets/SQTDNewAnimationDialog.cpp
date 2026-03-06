// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQTDNewAnimationDialog.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SQTDNewAnimationDialog"

namespace
{
	TSharedRef<SWidget> MakeLabeledRow(const FText& Label, TSharedRef<SWidget> Value)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(140.0f)
				[
					SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle("SmallFont"))
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)[ Value ];
	}
}

void SQTDNewAnimationDialog::Construct(const FArguments& InArgs)
{
	WeakParentWindow = InArgs._ParentWindow;
	Result.Name = InArgs._DefaultName;

	SAssignNew(NameBox, SEditableTextBox)
		.Text(FText::FromString(Result.Name))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.HintText(LOCTEXT("NameHint", "Animation name"));

	SAssignNew(LoopsBox, SEditableTextBox)
		.Text(FText::AsNumber(1))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.HintText(LOCTEXT("LoopsHint", "-1 for infinite"));

	LoopTypeOptions.Add(MakeShared<FString>(TEXT("Restart")));
	LoopTypeOptions.Add(MakeShared<FString>(TEXT("Ping Pong")));
	SelectedLoopTypeIndex = 0;

	SAssignNew(LoopTypeDisplay, STextBlock)
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.Text_Lambda([this]() {
			return LoopTypeOptions.IsValidIndex(SelectedLoopTypeIndex)
				? FText::FromString(*LoopTypeOptions[SelectedLoopTypeIndex])
				: FText::GetEmpty();
		});

	ChildSlot
	[
		SNew(SBox).MinDesiredWidth(340.0f)
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 8.0f, 8.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Title", "New Animation"))
				.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			]
			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]

			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 6.0f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[ MakeLabeledRow(LOCTEXT("Name", "Name"), NameBox.ToSharedRef()) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[ MakeLabeledRow(LOCTEXT("Loops", "Loops (-1=infinite)"), LoopsBox.ToSharedRef()) ]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					MakeLabeledRow(LOCTEXT("LoopType", "Loop Type"),
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&LoopTypeOptions)
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> Item, ESelectInfo::Type)
						{
							if (Item.IsValid())
								SelectedLoopTypeIndex = LoopTypeOptions.IndexOfByKey(Item);
						})
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) -> TSharedRef<SWidget>
						{
							return SNew(STextBlock).Text(FText::FromString(*Item))
								.Font(FAppStyle::GetFontStyle("SmallFont"));
						})
						.InitiallySelectedItem(LoopTypeOptions[0])
						[ LoopTypeDisplay.ToSharedRef() ])
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					MakeLabeledRow(LOCTEXT("AutoKill", "Auto Kill"),
						SNew(SCheckBox)
						.IsChecked(ECheckBoxState::Checked)
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
							Result.bAutoKill = (S == ECheckBoxState::Checked);
						}))
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)
				[
					MakeLabeledRow(LOCTEXT("PlayWhilePaused", "Play While Paused"),
						SNew(SCheckBox)
						.IsChecked(ECheckBoxState::Unchecked)
						.OnCheckStateChanged_Lambda([this](ECheckBoxState S) {
							Result.bPlayWhilePaused = (S == ECheckBoxState::Checked);
						}))
				]
			]

			+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]

			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OK", "Create"))
					.OnClicked(this, &SQTDNewAnimationDialog::OnConfirm)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("Cancel", "Cancel"))
					.OnClicked(this, &SQTDNewAnimationDialog::OnCancel)
				]
			]
		]
	];
}

FReply SQTDNewAnimationDialog::OnConfirm()
{
	Result.bConfirmed = true;
	Result.Name = NameBox.IsValid() ? NameBox->GetText().ToString() : Result.Name;
	if (Result.Name.IsEmpty()) Result.Name = TEXT("NewAnimation");

	int32 L = 1;
	if (LoopsBox.IsValid())
	{
		LexFromString(L, *LoopsBox->GetText().ToString());
	}
	Result.Loops    = L;
	Result.LoopType = (ELoopType)FMath::Clamp(SelectedLoopTypeIndex, 0, LoopTypeOptions.Num() - 1);

	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin())
		Win->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SQTDNewAnimationDialog::OnCancel()
{
	Result.bConfirmed = false;
	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin())
		Win->RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
