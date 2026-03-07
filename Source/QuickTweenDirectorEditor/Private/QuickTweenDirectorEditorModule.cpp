// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QuickTweenDirectorEditorModule.h"
#include "AssetTypeActions_QuickTweenDirector.h"
#include "Widgets/SQTDDirectorPanel.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FQuickTweenDirectorEditorModule"

static const FName DirectorPanelTabName("QuickTweenDirectorPanel");

// ──────────────────────────────────────────────────────────────────────────────
// Startup / Shutdown
// ──────────────────────────────────────────────────────────────────────────────

void FQuickTweenDirectorEditorModule::StartupModule()
{
	// ── Asset type registration ────────────────────────────────────────────────
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AssetTypeActions = MakeShared<FAssetTypeActions_QuickTweenDirector>();
	AssetTools.RegisterAssetTypeActions(AssetTypeActions.ToSharedRef());

	// ── Nomad tab ─────────────────────────────────────────────────────────────
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		DirectorPanelTabName,
		FOnSpawnTab::CreateRaw(this, &FQuickTweenDirectorEditorModule::SpawnDirectorPanel))
		.SetDisplayName(LOCTEXT("DirectorTab", "QuickTween Director"))
		.SetTooltipText(LOCTEXT("DirectorTabTip", "Open the QuickTween Director animation panel"))
		.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tabs.SequencerMain"))
		.SetMenuType(ETabSpawnerMenuType::Enabled);  // Visible under Window menu

	// ── Blueprint editor toolbar button via UToolMenus ────────────────────────
	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		UToolMenu* ToolBar = ToolMenus->ExtendMenu("AssetEditor.BlueprintEditor.ToolBar");
		FToolMenuSection& Section = ToolBar->FindOrAddSection("QuickTweenDirector");
		Section.AddEntry(FToolMenuEntry::InitToolBarButton(
			"QuickTweenDirectorButton",
			FUIAction(FExecuteAction::CreateRaw(this, &FQuickTweenDirectorEditorModule::OpenDirectorPanel)),
			LOCTEXT("DirectorButton", "QuickTweenDirector"),
			LOCTEXT("DirectorButtonTip", "Open the QuickTween Director animation panel for this Blueprint"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Sequencer.Tabs.SequencerMain")
		));
	}

	// ── Track which Actor Blueprint is being edited ────────────────────────────
	if (GEditor)
	{
		UAssetEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (Sub)
		{
			AssetOpenedHandle = Sub->OnAssetOpenedInEditor().AddRaw(
				this, &FQuickTweenDirectorEditorModule::OnAssetOpenedInEditor);
		}
	}
}

void FQuickTweenDirectorEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		AssetTools.UnregisterAssetTypeActions(AssetTypeActions.ToSharedRef());
	}

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(DirectorPanelTabName);

	if (UToolMenus* ToolMenus = UToolMenus::Get())
	{
		ToolMenus->RemoveSection("AssetEditor.BlueprintEditor.ToolBar", "QuickTweenDirector");
	}

	if (GEditor && AssetOpenedHandle.IsValid())
	{
		UAssetEditorSubsystem* Sub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (Sub) Sub->OnAssetOpenedInEditor().Remove(AssetOpenedHandle);
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// SetActiveBlueprint
// ──────────────────────────────────────────────────────────────────────────────

void FQuickTweenDirectorEditorModule::SetActiveBlueprint(UBlueprint* BP)
{
	ActiveBlueprint = BP;
	if (TSharedPtr<SQTDDirectorPanel> Panel = DirectorPanel.Pin())
	{
		Panel->SetBlueprint(BP);
	}
}

// ──────────────────────────────────────────────────────────────────────────────
// Asset editor tracking
// ──────────────────────────────────────────────────────────────────────────────

void FQuickTweenDirectorEditorModule::OnAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance*)
{
	UBlueprint* BP = Cast<UBlueprint>(Asset);
	if (!BP) return;
	if (!BP->GeneratedClass || !BP->GeneratedClass->IsChildOf<AActor>()) return;

	SetActiveBlueprint(BP);

	// Auto-open the Director panel the first time an Actor Blueprint is opened.
	FGlobalTabmanager::Get()->TryInvokeTab(DirectorPanelTabName);
}

// ──────────────────────────────────────────────────────────────────────────────
// Nomad tab spawner
// ──────────────────────────────────────────────────────────────────────────────

TSharedRef<SDockTab> FQuickTweenDirectorEditorModule::SpawnDirectorPanel(const FSpawnTabArgs&)
{
	TSharedRef<SQTDDirectorPanel> Panel = SNew(SQTDDirectorPanel);
	DirectorPanel = Panel;

	if (ActiveBlueprint.IsValid())
	{
		Panel->SetBlueprint(ActiveBlueprint.Get());
	}

	return SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		.Label(LOCTEXT("DirectorTab", "QuickTween Director"))
		[
			Panel
		];
}

// ──────────────────────────────────────────────────────────────────────────────
// Toolbar extension (Blueprint editor)
// ──────────────────────────────────────────────────────────────────────────────

void FQuickTweenDirectorEditorModule::OpenDirectorPanel()
{
	FGlobalTabmanager::Get()->TryInvokeTab(DirectorPanelTabName);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FQuickTweenDirectorEditorModule, QuickTweenDirectorEditor)
