// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "AssetTypeActions_QuickTweenDirector.h"
#include "QuickTweenDirectorAsset.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions_QuickTweenDirector"

FText FAssetTypeActions_QuickTweenDirector::GetName() const
{
	return LOCTEXT("Name", "Director Sequence");
}

FColor FAssetTypeActions_QuickTweenDirector::GetTypeColor() const
{
	return FColor(255, 120, 50);
}

UClass* FAssetTypeActions_QuickTweenDirector::GetSupportedClass() const
{
	return UQuickTweenDirectorAsset::StaticClass();
}

uint32 FAssetTypeActions_QuickTweenDirector::GetCategories()
{
	return EAssetTypeCategories::Animation;
}

#undef LOCTEXT_NAMESPACE
