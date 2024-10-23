// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimNode_MVAE_Retargeter.h"

#include "Kismet/KismetMathLibrary.h"
/*
void FAnimNode_MVAE_Retargeter::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(ModifyBone, !IsInGameThread());

	check(OutBoneTransforms.Num() == 0);


	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	
	FReferenceSkeleton RefSkeleton = BoneContainer.GetReferenceSkeleton();
	for(int32 idx = 0; idx< RefSkeleton.GetNum(); idx++)
	{
		FName BoneName = RefSkeleton.GetBoneName(idx);
		//TODO check if the bone name is one of the bones we want to retarget
		FCompactPoseBoneIndex CompactBoneIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(idx);
		Output.Pose.GetComponentSpaceTransform(CompactBoneIdx)
		UE_LOG(LogTemp, Warning, TEXT("%s"), *(BoneName.ToString()));
	}
}


void FAnimNode_MVAE_Retargeter::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	FAnimNode_SkeletalControlBase::InitializeBoneReferences(RequiredBones);
}
*/

void FAnimNode_MVAE_Retargeter::Evaluate_AnyThread(FPoseContext& Output)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	if(NewLocalRotations.Num() == 0)
	{
		BasePose.Evaluate(Output);
		return;
	}
	const FBoneContainer& BoneContainer = Output.Pose.GetBoneContainer();
	TArrayView<FTransform> LocalBoneTransforms = Output.Pose.GetMutableBones();
	
	BasePose.Evaluate(Output);
	FReferenceSkeleton RefSkeleton = BoneContainer.GetReferenceSkeleton();

	
	/*TMap<FString, int32> BodyPartMap = {
		{TEXT("root"), 0},
			{TEXT("thigh_l"), 1},
			{TEXT("calf_l"), 2},
			{TEXT("foot_l"), 3},
			{TEXT("ball_l"), 4},
			{TEXT("thigh_r"), 5},
			{TEXT("calf_r"), 6},
			{TEXT("foot_r"), 7},
			{TEXT("ball_r"), 8},
			{TEXT("spine_01"), 9},
			{TEXT("spine_03"), 10},
			{TEXT("spine_05"), 11},
			{TEXT("neck_01"), 12},
			{TEXT("head"), 13},
			{TEXT("clavicle_l"), 14},
			{TEXT("upperarm_l"), 15},
			{TEXT("lowerarm_l"), 16},
			{TEXT("hand_l"), 17},
			{TEXT("hh1"), 18},

		
			{TEXT("clavicle_r"), 19},
			{TEXT("upperarm_r"), 20},
			{TEXT("lowerarm_r"), 21},
			{TEXT("hand_r"), 22},
		{ TEXT("hh2"), 23},
	};*/


	TMap<FString, int32> MixamoBodyPartMap = {
		{TEXT("Hips"), 0},
			{TEXT("LeftUpLeg"), 1},
			{TEXT("LeftLeg"), 2},
			{TEXT("LeftFoot"), 3},
			{TEXT("LeftToeBase"), 4},
			{TEXT("RightUpLeg"), 5},
			{TEXT("RightLeg"), 6},
			{TEXT("RightFoot"), 7},
			{TEXT("RightToeBase"), 8},
			{TEXT("Spine"), 9},
			{TEXT("Spine1"), 10},
			{TEXT("Spine2"), 11},
			{TEXT("Neck"), 12},
			{TEXT("Head"), 13},
			{TEXT("LeftShoulder"), 14},
			{TEXT("LeftArm"), 15},
			{TEXT("LeftForeArm"), 16},
			{TEXT("LeftHand"), 17},
			{TEXT("hh1"), 18},


			{TEXT("RightShoulder"), 19},
			{TEXT("RightArm"), 20},
			{TEXT("RightForeArm"), 21},
			{TEXT("RightHand"), 22},
		{ TEXT("hh2"), 23},
	};
	TArray<FTransform>  RefTransform = RefSkeleton.GetRefBonePose();
	for (int32 idx = 0; idx < RefSkeleton.GetNum(); idx++)
	{
		FName BoneName = RefSkeleton.GetBoneName(idx);
		if(MixamoBodyPartMap.Contains(BoneName.ToString()))
		{
			FCompactPoseBoneIndex CompactBoneIdx = BoneContainer.GetCompactPoseIndexFromSkeletonIndex(idx);
			FTransform& LocalBonePose = Output.Pose[FCompactPoseBoneIndex(idx)];
			LocalBonePose = RefTransform[idx];
			//TODO get this based on the num of the bone
			//const int32 NewLocalRotationsIdx = 0;
			FRotator BoneRotator = NewLocalRotations[MixamoBodyPartMap[BoneName.ToString()]];
			//BoneRotator.Roll *= -1;
			//BoneRotator.Yaw *= 1;
			//BoneRotator.Pitch *=  1;
			const FQuat BoneQuat(BoneRotator);
			
			const FQuat BaseQuat(RefTransform[idx].GetRotation());
			//UE_LOG(LogClass, Log, TEXT("%s"), *(BaseQuat.ToString()));
			LocalBonePose.SetRotation(BoneQuat);
			// TODO remove the * 0
		}

		
	}
}
