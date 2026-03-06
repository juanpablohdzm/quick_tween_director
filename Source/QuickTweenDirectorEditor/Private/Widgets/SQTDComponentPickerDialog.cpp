// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "Widgets/SQTDComponentPickerDialog.h"

#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"

#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Views/STableRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateIconFinder.h"

#define LOCTEXT_NAMESPACE "SQTDComponentPickerDialog"

// ──────────────────────────────────────────────────────────────────────────────
// Construct
// ──────────────────────────────────────────────────────────────────────────────

void SQTDComponentPickerDialog::Construct(const FArguments& InArgs)
{
	Blueprint       = InArgs._Blueprint;
	WeakParentWindow = InArgs._ParentWindow;

	BuildComponentTree();

	SAssignNew(TreeView, STreeView<TSharedPtr<FComponentNode>>)
		.TreeItemsSource(&RootNodes)
		.OnGenerateRow(this, &SQTDComponentPickerDialog::GenerateRow)
		.OnGetChildren(this, &SQTDComponentPickerDialog::GetChildren)
		.OnSelectionChanged(this, &SQTDComponentPickerDialog::OnSelectionChanged)
		.OnMouseButtonDoubleClick(this, &SQTDComponentPickerDialog::OnItemDoubleClicked)
		.SelectionMode(ESelectionMode::Single);

	// Expand all root nodes so the hierarchy is visible by default.
	for (const TSharedPtr<FComponentNode>& Root : RootNodes)
	{
		TreeView->SetItemExpansion(Root, true);
	}

	ChildSlot
	[
		SNew(SBox)
		.MinDesiredWidth(340.0f)
		.MinDesiredHeight(300.0f)
		[
			SNew(SVerticalBox)

			// Header
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.055f, 0.055f, 0.055f))
				.Padding(FMargin(0.f))
				[
					SNew(SHorizontalBox)
					// Orange accent bar
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(4.f)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
							.BorderBackgroundColor(FLinearColor(1.0f, 0.55f, 0.15f))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center).Padding(12.f, 10.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "Select Component to Animate"))
						.Font(FAppStyle::GetFontStyle("NormalFont"))
						.ColorAndOpacity(FLinearColor(0.90f, 0.90f, 0.90f))
					]
				]
			]

			// 1px separator
			+ SVerticalBox::Slot().AutoHeight().MaxHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.06f))
				.Padding(0.f)
			]

			// Tree
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(4.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.07f, 0.07f, 0.07f))
				[
					TreeView.ToSharedRef()
				]
			]

			// 1px separator
			+ SVerticalBox::Slot().AutoHeight().MaxHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.06f))
				.Padding(0.f)
			]

			// Buttons
			+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 6.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f)[ SNew(SSpacer) ]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "FlatButton")
					.Text(LOCTEXT("Cancel", "Cancel"))
					.TextStyle(FAppStyle::Get(), "SmallText")
					.OnClicked(this, &SQTDComponentPickerDialog::OnCancel)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(4.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "PrimaryButton")
					.Text(LOCTEXT("OK", "Add Track"))
					.TextStyle(FAppStyle::Get(), "SmallText")
					.IsEnabled_Lambda([this]() { return CanConfirm(); })
					.OnClicked(this, &SQTDComponentPickerDialog::OnConfirm)
				]
			]
		]
	];
}

// ──────────────────────────────────────────────────────────────────────────────
// Build component tree from Blueprint SCS
// ──────────────────────────────────────────────────────────────────────────────

void SQTDComponentPickerDialog::BuildComponentTree()
{
	RootNodes.Empty();
	if (!Blueprint || !Blueprint->SimpleConstructionScript) return;

	// Recursive lambda to build nodes from SCS_Node tree.
	TFunction<TSharedPtr<FComponentNode>(USCS_Node*)> BuildNode = [&](USCS_Node* SCSNode) -> TSharedPtr<FComponentNode>
	{
		if (!SCSNode || !SCSNode->ComponentClass) return nullptr;

		auto Node         = MakeShared<FComponentNode>();
		Node->VariableName  = SCSNode->GetVariableName();
		Node->ComponentClass = SCSNode->ComponentClass;
		Node->DisplayName    = Node->VariableName.ToString();

		// Use the component class display name if the variable name is a generated ID.
		if (Node->DisplayName.IsEmpty() || Node->DisplayName.StartsWith(TEXT("Component_")))
		{
			Node->DisplayName = SCSNode->ComponentClass->GetName();
		}

		for (USCS_Node* Child : SCSNode->GetChildNodes())
		{
			TSharedPtr<FComponentNode> ChildNode = BuildNode(Child);
			if (ChildNode.IsValid()) Node->Children.Add(ChildNode);
		}

		return Node;
	};

	for (USCS_Node* RootNode : Blueprint->SimpleConstructionScript->GetRootNodes())
	{
		TSharedPtr<FComponentNode> Node = BuildNode(RootNode);
		if (Node.IsValid()) RootNodes.Add(Node);
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Tree view callbacks
// ──────────────────────────────────────────────────────────────────────────────

TSharedRef<ITableRow> SQTDComponentPickerDialog::GenerateRow(
	TSharedPtr<FComponentNode> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const bool bIsScene = Item->ComponentClass && Item->ComponentClass->IsChildOf<USceneComponent>();
	const FText TypeLabel = Item->ComponentClass
		? FText::FromString(TEXT("(") + Item->ComponentClass->GetName() + TEXT(")"))
		: FText::GetEmpty();

	return SNew(STableRow<TSharedPtr<FComponentNode>>, OwnerTable)
		.Padding(FMargin(4.0f, 2.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SImage)
				.Image(FSlateIconFinder::FindIconBrushForClass(
					Item->ComponentClass.Get(),
					TEXT("SCS.Component")))
				.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(FText::FromString(Item->DisplayName))
				.Font(FAppStyle::GetFontStyle("SmallFont"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(TypeLabel)
				.Font(FAppStyle::GetFontStyle("TinyText"))
				.ColorAndOpacity(FLinearColor(0.5f, 0.5f, 0.5f))
			]
		];
}

void SQTDComponentPickerDialog::GetChildren(
	TSharedPtr<FComponentNode> Item, TArray<TSharedPtr<FComponentNode>>& OutChildren)
{
	OutChildren = Item->Children;
}

void SQTDComponentPickerDialog::OnSelectionChanged(
	TSharedPtr<FComponentNode> Item, ESelectInfo::Type /*SelectInfo*/)
{
	SelectedNode = Item;
}

void SQTDComponentPickerDialog::OnItemDoubleClicked(TSharedPtr<FComponentNode> Item)
{
	SelectedNode = Item;
	OnConfirm();
}

// ──────────────────────────────────────────────────────────────────────────────
// Confirm / Cancel
// ──────────────────────────────────────────────────────────────────────────────

FReply SQTDComponentPickerDialog::OnConfirm()
{
	if (SelectedNode.IsValid())
	{
		Result.VariableName   = SelectedNode->VariableName;
		Result.ComponentClass = SelectedNode->ComponentClass;
		Result.DisplayName    = SelectedNode->DisplayName;
		Result.bConfirmed     = true;
	}
	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin()) Win->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SQTDComponentPickerDialog::OnCancel()
{
	if (TSharedPtr<SWindow> Win = WeakParentWindow.Pin()) Win->RequestDestroyWindow();
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
