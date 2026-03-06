// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#include "QuickTweenDirectorAsset.h"

float UQuickTweenDirectorAsset::GetTotalDuration() const
{
	float MaxEnd = 0.0f;
	for (const FQTDStepData& Step : Steps)
	{
		MaxEnd = FMath::Max(MaxEnd, Step.GetEndTime());
	}
	return MaxEnd;
}

FGuid UQuickTweenDirectorAsset::AddTrack(const FString& Label, EQTDStepType DefaultType)
{
	FQTDTrackData Track;
	Track.TrackId       = FGuid::NewGuid();
	Track.TrackLabel    = Label;
	Track.DefaultStepType = DefaultType;
	Tracks.Add(Track);
	MarkPackageDirty();
	return Track.TrackId;
}

void UQuickTweenDirectorAsset::RemoveTrack(const FGuid& InTrackId)
{
	// Remove all steps belonging to this track first.
	Steps.RemoveAll([&InTrackId](const FQTDStepData& S) { return S.TrackId == InTrackId; });
	Tracks.RemoveAll([&InTrackId](const FQTDTrackData& T) { return T.TrackId == InTrackId; });
	MarkPackageDirty();
}

int32 UQuickTweenDirectorAsset::AddStep(FQTDStepData InStep)
{
	if (!InStep.StepId.IsValid())
	{
		InStep.StepId = FGuid::NewGuid();
	}
	int32 Idx = Steps.Add(InStep);
	MarkPackageDirty();
	return Idx;
}

void UQuickTweenDirectorAsset::RemoveStep(const FGuid& InStepId)
{
	Steps.RemoveAll([&InStepId](const FQTDStepData& S) { return S.StepId == InStepId; });
	MarkPackageDirty();
}

void UQuickTweenDirectorAsset::UpdateStep(const FQTDStepData& InStep)
{
	FQTDStepData* Found = FindStep(InStep.StepId);
	if (Found)
	{
		*Found = InStep;
		MarkPackageDirty();
	}
}

TArray<FQTDStepData*> UQuickTweenDirectorAsset::GetStepsForTrack(const FGuid& InTrackId)
{
	TArray<FQTDStepData*> Result;
	for (FQTDStepData& Step : Steps)
	{
		if (Step.TrackId == InTrackId)
		{
			Result.Add(&Step);
		}
	}
	return Result;
}

FQTDStepData* UQuickTweenDirectorAsset::FindStep(const FGuid& InStepId)
{
	return Steps.FindByPredicate([&InStepId](const FQTDStepData& S) { return S.StepId == InStepId; });
}

FQTDTrackData* UQuickTweenDirectorAsset::FindTrack(const FGuid& InTrackId)
{
	return Tracks.FindByPredicate([&InTrackId](const FQTDTrackData& T) { return T.TrackId == InTrackId; });
}
