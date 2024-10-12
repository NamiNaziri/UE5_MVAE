// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "Runtime/AnimGraphRuntime/Public/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_MVAE_Retargeter.generated.h"
/**
 * 
 */

USTRUCT(BlueprintType)
struct FBonesRotations
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BoneRotations")
	TArray<FRotator> NewLocalRotations;
};


USTRUCT(BlueprintType)

struct MVAE_API FAnimNode_MVAE_Retargeter: public FAnimNode_Base
{

	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose;
	 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinShownByDefault))
	TArray<FRotator> NewLocalRotations;

	//virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void Evaluate_AnyThread(FPoseContext& Output);
	//virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

	virtual void GatherDebugData(FNodeDebugData& DebugData)override{}

private:


};
  