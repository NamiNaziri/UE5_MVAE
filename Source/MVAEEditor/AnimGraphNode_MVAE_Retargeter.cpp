// Fill out your copyright notice in the Description page of Project Settings.


#include "AnimGraphNode_MVAE_Retargeter.h"

UAnimGraphNode_MVAE_Retargeter::UAnimGraphNode_MVAE_Retargeter(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
}

FText UAnimGraphNode_MVAE_Retargeter::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("MVAE_Retargeter");
}
