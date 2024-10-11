// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"

/**
 * 
 */
USTRUCT(BlueprintType)

struct MVAE_API FAnimNode_MVAE_Retargeter: public FAnimNode_Base
{

	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose; 
	
    virtual void Initialize(const FAnimationInitializeContext& Context) override;
    virtual void CacheBones(const FAnimationCacheBonesContext& Context)  override;
    virtual void Update(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;

	virtual void GatherDebugData(FNodeDebugData& DebugData)override{} 
};
