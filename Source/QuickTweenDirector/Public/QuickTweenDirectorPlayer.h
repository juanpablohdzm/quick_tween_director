// Copyright 2025 Juan Pablo Hernandez Mosti. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "QuickTweenable.h"
#include "Utils/CommonValues.h"
#include "Tweens/QuickTweenBase.h"
#include "QTDStepData.h"
#include "QuickTweenDirectorPlayer.generated.h"

class UQuickTweenDirectorAsset;
class UQuickTweenBase;
class AActor;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTDPlayerEvent, UQuickTweenDirectorPlayer*, Player);

/**
 * Internal entry pairing a built tween with its absolute timeline position.
 * Not exposed to Blueprint — managed internally by UQuickTweenDirectorPlayer.
 */
USTRUCT()
struct FQTDBuiltStep
{
	GENERATED_BODY()

	UPROPERTY()
	UQuickTweenable* Tween = nullptr;

	float StartTime = 0.0f;

	/** Total wall-clock duration of the step (Duration * Loops / TimeScale). */
	float TotalDuration = 0.0f;
};

/**
 * Runtime player that instantiates and plays a UQuickTweenDirectorAsset.
 *
 * Workflow (Blueprint):
 *   1. Call UQuickTweenDirectorLibrary::CreateDirectorPlayer(Asset, self)
 *   2. Call BindSlot(Name, Actor/Component) for every named slot in the asset.
 *   3. Call Play() — the player builds the tweens on first play.
 *   4. Optionally add the player to another sequence with UQuickTweenLibrary
 *      functions, just like any UQuickTweenable.
 *
 * Implements the full UQuickTweenable interface so it behaves identically to
 * a UQuickTweenSequence and can be nested inside other sequences.
 */
UCLASS(BlueprintType)
class QUICKTWEENDIRECTOR_API UQuickTweenDirectorPlayer : public UQuickTweenable
{
	GENERATED_BODY()

	friend class UQuickTweenDirectorLibrary;

public:

	// ── Slot binding ──────────────────────────────────────────────────────────

	/**
	 * Bind a named slot to a USceneComponent (or AActor — its root component is used).
	 * Must be called before Play() / Build().
	 * @param SlotName  The name referenced in FQTDStepData::SlotName.
	 * @param Object    The live object to animate (Actor or SceneComponent).
	 */
	UFUNCTION(BlueprintCallable, Category = "Director|Binding")
	void BindSlot(FName SlotName, UObject* Object);

	/**
	 * Explicitly (re-)build the sequence from the asset and current bindings.
	 * Called automatically on the first Play() if the player has not been built yet.
	 * @param WorldContext  Needed to resolve material / curve assets.
	 * @return              True if the build succeeded.
	 */
	UFUNCTION(BlueprintCallable, Category = "Director")
	bool Build(UObject* WorldContext);

	// ── Tween accessors ───────────────────────────────────────────────────────

	/**
	 * Return the tween for the step with the given ID, or nullptr if not found.
	 * The player must have been built (Play() or Build() called) before calling this.
	 * Cast the result to UQuickTweenBase to assign per-tween callbacks.
	 */
	UFUNCTION(BlueprintCallable, Category = "Director")
	UQuickTweenBase* GetTweenByStepId(FGuid StepId) const;

	/**
	 * Return the first tween whose step label matches Label, or nullptr if not found.
	 * The player must have been built (Play() or Build() called) before calling this.
	 * Cast the result to UQuickTweenBase to assign per-tween callbacks.
	 */
	UFUNCTION(BlueprintCallable, Category = "Director")
	UQuickTweenBase* GetTweenByLabel(const FString& Label) const;

	// ── UQuickTweenable interface ─────────────────────────────────────────────

	virtual void SetOwner(UQuickTweenable* InOwner) override { Owner = InOwner; }

	virtual void Play()                    override;
	virtual void Pause()                   override;
	virtual void Reverse()                 override;
	virtual void Restart()                 override;
	virtual void Complete(bool bSnapToEnd = true) override;
	virtual void Kill()                    override;

	virtual void Update(float DeltaTime)   override;
	virtual void Evaluate(const FQuickTweenEvaluatePayload& Payload, const UQuickTweenable* Instigator) override;

	// ── State queries ─────────────────────────────────────────────────────────

	virtual bool  GetIsPendingKill()        const override { return PlayerState == EQuickTweenState::Kill; }
	virtual float GetLoopDuration()         const override;
	virtual float GetTotalDuration()        const override;
	virtual float GetElapsedTime()          const override { return ElapsedTime; }
	virtual float GetTimeScale()            const override { return 1.0f; }
	virtual bool  GetIsPlaying()            const override { return PlayerState == EQuickTweenState::Play; }
	virtual bool  GetIsCompleted()          const override { return PlayerState == EQuickTweenState::Complete; }
	virtual bool  GetIsReversed()           const override { return bIsReversed; }
	virtual EEaseType     GetEaseType()     const override { return EEaseType::Linear; }
	virtual UCurveFloat*  GetEaseCurve()    const override { return nullptr; }
	virtual int32 GetLoops()               const override;
	virtual ELoopType GetLoopType()         const override;
	virtual FString GetTweenTag()           const override { return TEXT(""); }
	virtual int32 GetCurrentLoop()          const override { return CurrentLoop; }
	virtual bool  GetAutoKill()             const override;
	virtual bool  GetShouldPlayWhilePaused() const override;

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Director|Events")
	FOnQTDPlayerEvent OnStart;

	UPROPERTY(BlueprintAssignable, Category = "Director|Events")
	FOnQTDPlayerEvent OnUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Director|Events")
	FOnQTDPlayerEvent OnComplete;

	UPROPERTY(BlueprintAssignable, Category = "Director|Events")
	FOnQTDPlayerEvent OnKilled;

	UPROPERTY(BlueprintAssignable, Category = "Director|Events")
	FOnQTDPlayerEvent OnLoop;

private:

	// ── Factory (Blueprint library only) ──────────────────────────────────────

	static UQuickTweenDirectorPlayer* Create(UObject* WorldContext, UQuickTweenDirectorAsset* Asset);

	// ── Internals ─────────────────────────────────────────────────────────────

	bool HasOwner()             const { return Owner != nullptr; }
	bool InstigatorIsOwner(const UQuickTweenable* Instigator) const { return Instigator == Owner; }

	bool RequestStateTransition(EQuickTweenState NewState);
	void HandleOnStart();
	void HandleOnComplete();
	void HandleOnKill();

	/**
	 * Advance all child tweens to the given local loop time.
	 * Handles arbitrary per-step start times (fully parallel or offset).
	 */
	void SeekTime(float LoopLocalTime);

	/** Build a single UQuickTweenBase from a step descriptor. Returns nullptr on failure. */
	UQuickTweenBase* CreateTweenForStep(const FQTDStepData& Step);

	/** Resolve a slot name to a USceneComponent (may cast from Actor). */
	USceneComponent* ResolveComponentSlot(FName SlotName) const;

	// ── Data ──────────────────────────────────────────────────────────────────

	UPROPERTY()
	UQuickTweenDirectorAsset* Asset = nullptr;

	UPROPERTY()
	const UObject* WorldContextObject = nullptr;

	UPROPERTY()
	UQuickTweenable* Owner = nullptr;

	/** Slot-name → weak object pointer. Actors are resolved to their root component. */
	TMap<FName, TWeakObjectPtr<UObject>> SlotBindings;

	/** Child tweens kept alive by UPROPERTY. */
	UPROPERTY()
	TArray<UQuickTweenable*> OwnedTweens;

	/** Step data parallel to OwnedTweens — same index. Used by GetTweenByXxx. */
	TArray<FQTDStepData> BuiltStepData;

	/** Timeline-mapped steps. Sorted ascending by StartTime after Build(). */
	TArray<FQTDBuiltStep> BuiltSteps;

	bool bIsBuilt = false;

	// ── Playback state ────────────────────────────────────────────────────────

	EQuickTweenState PlayerState  = EQuickTweenState::Idle;
	float ElapsedTime             = 0.0f;
	float PreviousLoopLocalTime   = 0.0f;
	bool  bIsReversed             = false;
	bool  bWasActive              = false;  // used when owned by a parent sequence
	bool  bTriggerEvents          = true;
	bool  bSnapToEndOnComplete    = true;
	int32 CurrentLoop             = 0;
};
