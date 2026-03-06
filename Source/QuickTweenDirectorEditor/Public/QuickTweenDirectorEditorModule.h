// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
class UBlueprint;
class SQTDDirectorPanel;
class SDockTab;

/**
 * Editor module for QuickTweenDirector.
 *
 * Responsibilities:
 *  - Registers the "QuickTween Director" nomad tab in the global tab manager.
 *  - Extends the Blueprint editor toolbar with a "Director" button.
 *  - Tracks which Actor Blueprint is currently being edited so the Director
 *    panel can show the right animations and component hierarchy.
 */
class FQuickTweenDirectorEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule()  override;
	virtual void ShutdownModule() override;

	/** Updates the active Blueprint and refreshes the Director panel if open. */
	void SetActiveBlueprint(UBlueprint* BP);

	UBlueprint* GetActiveBlueprint() const { return ActiveBlueprint.Get(); }

	static FQuickTweenDirectorEditorModule& Get()
	{
		return FModuleManager::GetModuleChecked<FQuickTweenDirectorEditorModule>("QuickTweenDirectorEditor");
	}

private:

	// ── Blueprint editor tracking ─────────────────────────────────────────────
	void OnAssetOpenedInEditor(UObject* Asset, IAssetEditorInstance* EditorInstance);

	// ── Nomad tab ─────────────────────────────────────────────────────────────
	TSharedRef<SDockTab> SpawnDirectorPanel(const FSpawnTabArgs& Args);

	// ── Blueprint toolbar extension ───────────────────────────────────────────
	void AddToolbarButton(FToolBarBuilder& Builder);
	void OpenDirectorPanel();

	// ── Data ──────────────────────────────────────────────────────────────────
	TSharedPtr<IAssetTypeActions> AssetTypeActions;
	TSharedPtr<FExtender>         ToolBarExtender;

	TWeakObjectPtr<UBlueprint>    ActiveBlueprint;
	TWeakPtr<SQTDDirectorPanel>   DirectorPanel;

	FDelegateHandle AssetOpenedHandle;
};
