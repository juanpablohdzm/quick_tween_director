// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

/** Provides Content Browser metadata (name, color, category) for Director assets.
 *  Double-clicking an asset no longer opens a standalone editor — use the Blueprint
 *  Director panel instead (toolbar button inside any Actor Blueprint editor). */
class FAssetTypeActions_QuickTweenDirector : public FAssetTypeActions_Base
{
public:
	virtual FText   GetName()           const override;
	virtual FColor  GetTypeColor()      const override;
	virtual UClass* GetSupportedClass() const override;
	virtual uint32  GetCategories()           override;
};
