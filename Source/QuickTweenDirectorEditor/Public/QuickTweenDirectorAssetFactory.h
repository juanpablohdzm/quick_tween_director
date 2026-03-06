// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.
// Factory is kept for internal programmatic asset creation only.
// It is NOT exposed in the Content Browser right-click menu.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "QuickTweenDirectorAssetFactory.generated.h"

UCLASS()
class UQuickTweenDirectorAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UQuickTweenDirectorAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName,
	                                  EObjectFlags Flags, UObject* Context,
	                                  FFeedbackContext* Warn) override;

	virtual bool ShouldShowInNewMenu() const override { return false; }
};
