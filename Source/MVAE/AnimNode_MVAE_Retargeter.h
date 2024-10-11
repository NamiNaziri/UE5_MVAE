// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "AnimNode_MVAE_Retargeter.generated.h"
/**
 * 
 */
USTRUCT(BlueprintType)

struct MVAE_API FAnimNode_MVAE_Retargeter: public FAnimNode_Base
{

	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose; 
	
    virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
    virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output) override;

	virtual void GatherDebugData(FNodeDebugData& DebugData)override{} 
};

inline void FAnimNode_MVAE_Retargeter::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
}

inline void FAnimNode_MVAE_Retargeter::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	FAnimNode_Base::CacheBones_AnyThread(Context);
}

inline void FAnimNode_MVAE_Retargeter::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	FAnimNode_Base::Update_AnyThread(Context);
}

inline void FAnimNode_MVAE_Retargeter::Evaluate_AnyThread(FPoseContext& Output)
{
	FAnimNode_Base::Evaluate_AnyThread(Output);
}
