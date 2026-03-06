// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QuickTweenDirectorAssetFactory.h"
#include "QuickTweenDirectorAsset.h"

#define LOCTEXT_NAMESPACE "QuickTweenDirectorAssetFactory"

UQuickTweenDirectorAssetFactory::UQuickTweenDirectorAssetFactory()
{
	SupportedClass = UQuickTweenDirectorAsset::StaticClass();
	bCreateNew     = true;
	bEditAfterNew  = true;
}

UObject* UQuickTweenDirectorAssetFactory::FactoryCreateNew(
	UClass* InClass, UObject* InParent, FName InName,
	EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UQuickTweenDirectorAsset* Asset =
		NewObject<UQuickTweenDirectorAsset>(InParent, InClass, InName, Flags);
	return Asset;
}

#undef LOCTEXT_NAMESPACE
