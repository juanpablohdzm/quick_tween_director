// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQTDDirectorPanel.h"
#include "Widgets/SQuickTweenDirectorEditor.h"
#include "Widgets/SQTDComponentPickerDialog.h"
#include "Widgets/SQTDNewAnimationDialog.h"
#include "QuickTweenDirectorAsset.h"

#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraphSchema_K2.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "FileHelpers.h"
#include "Misc/MessageDialog.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"

#define LOCTEXT_NAMESPACE "SQTDDirectorPanel"

// ──────────────────────────────────────────────────────────────────────────────
// Construct
// ──────────────────────────────────────────────────────────────────────────────

void SQTDDirectorPanel::Construct(const FArguments& InArgs)
{
	// ── Empty state: no Blueprint focused ────────────────────────────────────
	SAssignNew(NoBlueprintHint, SBox)
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 12.f)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Sequencer.Tracks.CinematicShot"))
			.DesiredSizeOverride(FVector2D(40.f, 40.f))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.35f, 0.35f, 0.35f)))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBP", "No Actor Blueprint focused"))
			.Font(FAppStyle::GetFontStyle("NormalFont"))
			.ColorAndOpacity(FLinearColor(0.55f, 0.55f, 0.55f))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBPHint", "Open an Actor Blueprint and click\nthe Director button in the toolbar."))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(FLinearColor(0.32f, 0.32f, 0.32f))
			.Justification(ETextJustify::Center)
		]
	];

	// ── Empty state: no animation selected ───────────────────────────────────
	SAssignNew(EmptyTimelineHint, SBox)
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.f, 0.f, 0.f, 8.f)
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush("Sequencer.Timeline.ScrubHandleDown"))
			.DesiredSizeOverride(FVector2D(24.f, 24.f))
			.ColorAndOpacity(FSlateColor(FLinearColor(0.28f, 0.28f, 0.28f)))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoAnim", "Select or create an animation."))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(FLinearColor(0.32f, 0.32f, 0.32f))
		]
	];

	// ── Animation list (left column) ─────────────────────────────────────────
	SAssignNew(AnimListContainer, SVerticalBox);

	TSharedRef<SWidget> AnimListPanel =
		SNew(SVerticalBox)

		// Panel header
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
			.Padding(FMargin(0.f, 0.f, 0.f, 0.f))
			[
				SNew(SHorizontalBox)
				// Orange accent bar
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(3.f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FLinearColor(1.0f, 0.55f, 0.15f))
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AnimHeader", "ANIMATIONS"))
					.Font(FAppStyle::GetFontStyle("TinyText"))
					.ColorAndOpacity(FLinearColor(0.32f, 0.32f, 0.32f))
				]
			]
		]

		// "+ New Animation" button — full-width accent button
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 6.f, 6.f, 4.f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "FlatButton")
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(0.f, 5.f))
			.ToolTipText(LOCTEXT("NewAnimTip", "Create a new Director animation asset and add it as a Blueprint variable"))
			.OnClicked(this, &SQTDDirectorPanel::OnNewAnimationClicked)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 5.f, 0.f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Plus"))
					.DesiredSizeOverride(FVector2D(11.f, 11.f))
					.ColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.55f, 0.15f)))
				]
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("NewAnim", "New Animation"))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
					.ColorAndOpacity(FLinearColor(1.0f, 0.55f, 0.15f))
				]
			]
		]

		// Separator
		+ SVerticalBox::Slot().AutoHeight().Padding(6.f, 0.f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f))
			.Padding(FMargin(0.f))
			[ SNew(SBox).HeightOverride(1.f) ]
		]

		// Animation list (scrollable)
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
			.Padding(0.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					AnimListContainer.ToSharedRef()
				]
			]
		];

	// ── Timeline area (right column) ──────────────────────────────────────────
	SAssignNew(TimelineArea, SBox)
	[
		EmptyTimelineHint.ToSharedRef()
	];

	// ── Blueprint name header ─────────────────────────────────────────────────
	SAssignNew(BlueprintNameText, STextBlock)
		.Text(LOCTEXT("NoBPName", "No Blueprint"))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f));

	// ── Main content (shown when a Blueprint IS focused) ──────────────────────
	SAssignNew(MainContent, SVerticalBox)

	// Slim header bar showing the active Blueprint
	+ SVerticalBox::Slot().AutoHeight()
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
		.Padding(FMargin(10.f, 5.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("ClassIcon.Blueprint"))
				.DesiredSizeOverride(FVector2D(12.f, 12.f))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.38f, 0.38f, 0.38f)))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BPLabel", "Blueprint:"))
				.Font(FAppStyle::GetFontStyle("TinyText"))
				.ColorAndOpacity(FLinearColor(0.38f, 0.38f, 0.38f))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				BlueprintNameText.ToSharedRef()
			]
		]
	]

	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot().Value(0.22f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f))
			.Padding(0.f)
			[
				AnimListPanel
			]
		]
		+ SSplitter::Slot().Value(0.78f)
		[
			TimelineArea.ToSharedRef()
		]
	];

	SAssignNew(RootBorder, SBorder)
	.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
	.BorderBackgroundColor(FLinearColor(0.013f, 0.013f, 0.013f, 1.0f))
	.Padding(0.f)
	[
		NoBlueprintHint.ToSharedRef()
	];

	ChildSlot
	[
		RootBorder.ToSharedRef()
	];
}

// ──────────────────────────────────────────────────────────────────────────────
// SetBlueprint — called by the editor module when a Blueprint gets focus
// ──────────────────────────────────────────────────────────────────────────────

void SQTDDirectorPanel::SetBlueprint(UBlueprint* NewBlueprint)
{
	Blueprint = NewBlueprint;
	SelectedAnimIndex = INDEX_NONE;
	TimelineEditor.Reset();

	if (!NewBlueprint)
	{
		RootBorder->SetContent(NoBlueprintHint.ToSharedRef());
		return;
	}

	BlueprintNameText->SetText(FText::FromString(NewBlueprint->GetName()));
	RootBorder->SetContent(MainContent.ToSharedRef());

	RefreshAnimationList();
}

// ──────────────────────────────────────────────────────────────────────────────
// Animation list
// ──────────────────────────────────────────────────────────────────────────────

void SQTDDirectorPanel::RefreshAnimationList()
{
	if (!AnimListContainer) return;
	AnimListContainer->ClearChildren();
	AnimVarNames.Empty();

	if (!Blueprint.IsValid()) return;

	TArray<TPair<FName, UQuickTweenDirectorAsset*>> Animations = GetDirectorAnimations();

	for (int32 i = 0; i < Animations.Num(); ++i)
	{
		const FName VarName  = Animations[i].Key;
		const int32 CapturedIndex = i;
		AnimVarNames.Add(VarName);

		const bool bSelected = (i == SelectedAnimIndex);
		const FLinearColor RowBg     = bSelected
			? FLinearColor(0.025f, 0.025f, 0.025f) : FLinearColor(0.013f, 0.013f, 0.013f);
		const FLinearColor AccentBar = bSelected
			? FLinearColor(1.0f, 0.55f, 0.15f)  : FLinearColor(1.0f, 0.55f, 0.15f, 0.0f);
		const FLinearColor NameColor = bSelected
			? FLinearColor(0.95f, 0.95f, 0.95f) : FLinearColor(0.65f, 0.65f, 0.65f);

		AnimListContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.OnClicked_Lambda([this, CapturedIndex]() -> FReply
			{
				SelectAnimation(CapturedIndex);
				return FReply::Handled();
			})
			.ContentPadding(FMargin(0.f))
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(RowBg)
				.Padding(FMargin(0.f))
				[
					SNew(SHorizontalBox)

					// Left accent bar (orange when selected)
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(3.f)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(AccentBar)
						]
					]

					// Animation name (inline-editable)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(8.f, 5.f, 4.f, 5.f)
					[
						SNew(SInlineEditableTextBlock)
						.Text(FText::FromName(VarName))
						.Font(FAppStyle::GetFontStyle("SmallFont"))
						.ColorAndOpacity(NameColor)
						.IsReadOnly(false)
						.OnTextCommitted_Lambda([this, CapturedIndex](const FText& NewText, ETextCommit::Type CommitType)
						{
							OnRenameAnimation(CapturedIndex, NewText, CommitType);
						})
						.OnVerifyTextChanged_Lambda([](const FText& Text, FText& OutError) -> bool
						{
							if (Text.IsEmpty())
							{
								OutError = LOCTEXT("EmptyName", "Name cannot be empty");
								return false;
							}
							return true;
						})
					]

					// Delete button (X icon)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 4.f, 0.f)
					[
						SNew(SButton)
						.ButtonStyle(FAppStyle::Get(), "SimpleButton")
						.ToolTipText(LOCTEXT("DeleteAnim", "Remove this animation variable"))
						.OnClicked_Lambda([this, CapturedIndex]() -> FReply
						{
							OnDeleteAnimation(CapturedIndex);
							return FReply::Handled();
						})
						.ContentPadding(FMargin(3.f, 2.f))
						[
							SNew(SImage)
							.Image(FAppStyle::GetBrush("Icons.X"))
							.DesiredSizeOverride(FVector2D(9.f, 9.f))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.18f, 0.18f)))
						]
					]
				]
			]
		];
	}

	// Re-select if index is still valid
	if (SelectedAnimIndex >= 0 && SelectedAnimIndex < Animations.Num())
	{
		// Timeline already set — just redraw the list
	}
	else
	{
		// Clear timeline
		SelectedAnimIndex = INDEX_NONE;
		if (TimelineArea)
		{
			TimelineArea->SetContent(EmptyTimelineHint.ToSharedRef());
		}
		TimelineEditor.Reset();
	}
}

void SQTDDirectorPanel::SelectAnimation(int32 Index)
{
	TArray<TPair<FName, UQuickTweenDirectorAsset*>> Animations = GetDirectorAnimations();
	if (!Animations.IsValidIndex(Index)) return;

	SelectedAnimIndex = Index;
	UQuickTweenDirectorAsset* Asset = Animations[Index].Value;

	if (Asset && TimelineArea)
	{
		SAssignNew(TimelineEditor, SQuickTweenDirectorEditor)
			.Asset(Asset)
			.Blueprint(Blueprint);

		TimelineArea->SetContent(TimelineEditor.ToSharedRef());
	}
	else if (TimelineArea)
	{
		TimelineArea->SetContent(EmptyTimelineHint.ToSharedRef());
		TimelineEditor.Reset();
	}

	// Rebuild list to update selection indicator
	RefreshAnimationList();
}

// ──────────────────────────────────────────────────────────────────────────────
// New animation
// ──────────────────────────────────────────────────────────────────────────────

FReply SQTDDirectorPanel::OnNewAnimationClicked()
{
	if (!Blueprint.IsValid()) return FReply::Handled();

	// Show the dialog to collect name + playback settings
	TSharedRef<SWindow> DialogWindow = SNew(SWindow)
		.Title(LOCTEXT("NewAnimTitle", "New Animation"))
		.SizingRule(ESizingRule::Autosized)
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	TSharedRef<SQTDNewAnimationDialog> Dialog = SNew(SQTDNewAnimationDialog)
		.DefaultName(MakeUniqueAnimationName())
		.ParentWindow(DialogWindow);

	DialogWindow->SetContent(Dialog);
	FSlateApplication::Get().AddModalWindow(DialogWindow, AsShared());

	const SQTDNewAnimationDialog::FResult& R = Dialog->GetResult();
	if (!R.bConfirmed) return FReply::Handled();

	// Sanitize the name to be a valid identifier
	FString VarName = R.Name.Replace(TEXT(" "), TEXT("_"));
	if (VarName.IsEmpty()) VarName = MakeUniqueAnimationName();

	UQuickTweenDirectorAsset* NewAsset = CreateNewAnimation(VarName);
	if (!NewAsset) return FReply::Handled();

	// Apply the user's playback settings to the asset
	NewAsset->Loops            = R.Loops;
	NewAsset->LoopType         = R.LoopType;
	NewAsset->bAutoKill        = R.bAutoKill;
	NewAsset->bPlayWhilePaused = R.bPlayWhilePaused;
	NewAsset->MarkPackageDirty();

	// Select the newly created animation
	TArray<TPair<FName, UQuickTweenDirectorAsset*>> Animations = GetDirectorAnimations();
	for (int32 i = 0; i < Animations.Num(); ++i)
	{
		if (Animations[i].Key == FName(*VarName))
		{
			SelectAnimation(i);
			break;
		}
	}

	RefreshAnimationList();
	return FReply::Handled();
}

void SQTDDirectorPanel::OnDeleteAnimation(int32 Index)
{
	if (!Blueprint.IsValid()) return;
	TArray<TPair<FName, UQuickTweenDirectorAsset*>> Animations = GetDirectorAnimations();
	if (!Animations.IsValidIndex(Index)) return;

	const FName VarName = Animations[Index].Key;

	// Confirm before deleting
	const FText Msg = FText::Format(
		LOCTEXT("DeleteConfirm", "Delete animation \"{0}\"?\nThe asset file will not be deleted."),
		FText::FromName(VarName));

	if (FMessageDialog::Open(EAppMsgType::YesNo, Msg) != EAppReturnType::Yes) return;

	FBlueprintEditorUtils::RemoveMemberVariable(Blueprint.Get(), VarName);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint.Get());
	FKismetEditorUtilities::CompileBlueprint(Blueprint.Get());

	if (SelectedAnimIndex == Index)      SelectedAnimIndex = INDEX_NONE;
	else if (SelectedAnimIndex > Index)  --SelectedAnimIndex;

	RefreshAnimationList();

	if (SelectedAnimIndex == INDEX_NONE && TimelineArea)
	{
		TimelineArea->SetContent(EmptyTimelineHint.ToSharedRef());
		TimelineEditor.Reset();
	}
}

void SQTDDirectorPanel::OnRenameAnimation(int32 Index, const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnCleared || !Blueprint.IsValid()) return;
	if (!AnimVarNames.IsValidIndex(Index)) return;

	const FName OldName = AnimVarNames[Index];
	const FName NewName = FName(*NewText.ToString());
	if (OldName == NewName) return;

	FBlueprintEditorUtils::RenameMemberVariable(Blueprint.Get(), OldName, NewName);
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint.Get());
	FKismetEditorUtilities::CompileBlueprint(Blueprint.Get());

	RefreshAnimationList();
}

// ──────────────────────────────────────────────────────────────────────────────
// Asset / variable management
// ──────────────────────────────────────────────────────────────────────────────

TArray<TPair<FName, UQuickTweenDirectorAsset*>> SQTDDirectorPanel::GetDirectorAnimations() const
{
	TArray<TPair<FName, UQuickTweenDirectorAsset*>> Result;
	if (!Blueprint.IsValid()) return Result;

	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		if (Var.VarType.PinCategory != UEdGraphSchema_K2::PC_Object) continue;
		UObject* SubCatObj = Var.VarType.PinSubCategoryObject.Get();
		if (SubCatObj != UQuickTweenDirectorAsset::StaticClass()) continue;

		UQuickTweenDirectorAsset* Asset = nullptr;

		// Try to get from the CDO after compilation
		if (Blueprint->GeneratedClass)
		{
			UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject(false);
			if (CDO)
			{
				FObjectProperty* Prop = FindFProperty<FObjectProperty>(Blueprint->GeneratedClass, Var.VarName);
				if (Prop) Asset = Cast<UQuickTweenDirectorAsset>(Prop->GetObjectPropertyValue_InContainer(CDO));
			}
		}

		// Fallback: load from path stored in DefaultValue
		if (!Asset && !Var.DefaultValue.IsEmpty())
		{
			Asset = Cast<UQuickTweenDirectorAsset>(
				StaticLoadObject(UQuickTweenDirectorAsset::StaticClass(), nullptr, *Var.DefaultValue));
		}

		Result.Add({ Var.VarName, Asset });
	}

	return Result;
}

UQuickTweenDirectorAsset* SQTDDirectorPanel::CreateNewAnimation(const FString& VarName)
{
	if (!Blueprint.IsValid()) return nullptr;

	// Build the package path alongside the Blueprint
	const FString BPPath    = FPackageName::GetLongPackagePath(Blueprint->GetPackage()->GetName());
	const FString AssetPath = FString::Printf(TEXT("%s/%s"), *BPPath, *VarName);
	const FString AssetName = VarName;

	// Create the package
	UPackage* NewPkg = CreatePackage(*AssetPath);
	if (!NewPkg) return nullptr;

	NewPkg->FullyLoad();

	UQuickTweenDirectorAsset* NewAsset = NewObject<UQuickTweenDirectorAsset>(
		NewPkg, *AssetName, RF_Public | RF_Standalone | RF_Transactional);

	FAssetRegistryModule::AssetCreated(NewAsset);
	NewPkg->MarkPackageDirty();

	// Add a Blueprint variable of type UQuickTweenDirectorAsset*
	FEdGraphPinType PinType;
	PinType.PinCategory          = UEdGraphSchema_K2::PC_Object;
	PinType.PinSubCategoryObject = UQuickTweenDirectorAsset::StaticClass();

	FBlueprintEditorUtils::AddMemberVariable(Blueprint.Get(), FName(*VarName), PinType);

	// Store the asset path in the variable's default value
	for (FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		if (VarDesc.VarName == FName(*VarName))
		{
			VarDesc.DefaultValue = NewAsset->GetPathName();
			break;
		}
	}

	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint.Get());
	FKismetEditorUtilities::CompileBlueprint(Blueprint.Get());

	// Assign on CDO after compile
	if (Blueprint->GeneratedClass)
	{
		UObject* CDO = Blueprint->GeneratedClass->GetDefaultObject(false);
		if (CDO)
		{
			FObjectProperty* Prop = FindFProperty<FObjectProperty>(Blueprint->GeneratedClass, FName(*VarName));
			if (Prop)
			{
				Prop->SetObjectPropertyValue_InContainer(CDO, NewAsset);
				CDO->MarkPackageDirty();
				FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint.Get());
				FKismetEditorUtilities::CompileBlueprint(Blueprint.Get());
			}
		}
	}

	return NewAsset;
}

FString SQTDDirectorPanel::MakeUniqueAnimationName() const
{
	if (!Blueprint.IsValid()) return TEXT("NewAnimation");

	// Collect existing variable names
	TSet<FName> Existing;
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		Existing.Add(Var.VarName);
	}

	const FString Base = TEXT("NewAnimation");
	if (!Existing.Contains(FName(*Base))) return Base;

	for (int32 i = 1; i < 1000; ++i)
	{
		const FString Candidate = FString::Printf(TEXT("%s_%d"), *Base, i);
		if (!Existing.Contains(FName(*Candidate))) return Candidate;
	}

	return Base;
}

#undef LOCTEXT_NAMESPACE
