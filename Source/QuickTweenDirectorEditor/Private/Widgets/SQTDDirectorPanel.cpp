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
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBP", "No Actor Blueprint focused"))
			.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
			.ColorAndOpacity(FLinearColor(0.6f, 0.6f, 0.6f))
		]
		+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NoBPHint", "Open an Actor Blueprint and click the\n\"Director\" button in the toolbar."))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(FLinearColor(0.45f, 0.45f, 0.45f))
			.Justification(ETextJustify::Center)
		]
	];

	// ── Empty state: no animation selected ───────────────────────────────────
	SAssignNew(EmptyTimelineHint, SBox)
	.HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("NoAnim", "Select or create an animation on the left."))
		.Font(FAppStyle::GetFontStyle("SmallFont"))
		.ColorAndOpacity(FLinearColor(0.45f, 0.45f, 0.45f))
	];

	// ── Animation list (left column) ─────────────────────────────────────────
	SAssignNew(AnimListContainer, SVerticalBox);

	TSharedRef<SWidget> AnimListPanel =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.0f, 4.0f, 4.0f, 2.0f)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.Text(LOCTEXT("NewAnim", "+ New Animation"))
			.ToolTipText(LOCTEXT("NewAnimTip", "Create a new Director animation and add it as a Blueprint variable"))
			.OnClicked(this, &SQTDDirectorPanel::OnNewAnimationClicked)
		]
		+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				AnimListContainer.ToSharedRef()
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
		.ColorAndOpacity(FLinearColor(0.7f, 0.7f, 0.7f));

	// ── Main content (shown when a Blueprint IS focused) ──────────────────────
	SAssignNew(MainContent, SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().Padding(6.0f, 4.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("BPLabel", "Blueprint:"))
			.Font(FAppStyle::GetFontStyle("SmallFont"))
			.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
		[
			BlueprintNameText.ToSharedRef()
		]
	]
	+ SVerticalBox::Slot().AutoHeight()[ SNew(SSeparator) ]
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot().Value(0.22f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.0f)
			[
				AnimListPanel
			]
		]
		+ SSplitter::Slot().Value(0.78f)
		[
			TimelineArea.ToSharedRef()
		]
	];

	ChildSlot
	[
		NoBlueprintHint.ToSharedRef()
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
		ChildSlot[ NoBlueprintHint.ToSharedRef() ];
		return;
	}

	BlueprintNameText->SetText(FText::FromString(NewBlueprint->GetName()));
	ChildSlot[ MainContent.ToSharedRef() ];

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

		AnimListContainer->AddSlot()
		.AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(bSelected
				? "DetailsView.CategoryTop_Hovered"
				: "NoBorder"))
			.Padding(FMargin(4.0f, 2.0f))
			.OnMouseButtonDown_Lambda([this, CapturedIndex](const FGeometry&, const FPointerEvent& Event) -> FReply
			{
				if (Event.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					SelectAnimation(CapturedIndex);
					return FReply::Handled();
				}
				return FReply::Unhandled();
			})
			[
				SNew(SHorizontalBox)

				// Selection indicator dot
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(bSelected ? TEXT("●") : TEXT("○")))
					.Font(FAppStyle::GetFontStyle("TinyText"))
					.ColorAndOpacity(bSelected
						? FLinearColor(1.0f, 0.55f, 0.25f)
						: FLinearColor(0.4f, 0.4f, 0.4f))
				]

				// Inline-editable name
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SInlineEditableTextBlock)
					.Text(FText::FromName(VarName))
					.Font(FAppStyle::GetFontStyle("SmallFont"))
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

				// Delete button
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "NoBorder")
					.ToolTipText(LOCTEXT("DeleteAnim", "Delete this animation"))
					.OnClicked_Lambda([this, CapturedIndex]() -> FReply
					{
						OnDeleteAnimation(CapturedIndex);
						return FReply::Handled();
					})
					.ContentPadding(2.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("✕")))
						.Font(FAppStyle::GetFontStyle("TinyText"))
						.ColorAndOpacity(FLinearColor(0.8f, 0.25f, 0.25f))
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
