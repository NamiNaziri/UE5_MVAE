// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AnimNode_MVAE_Retargeter.h"
#include "AnimGraphNode_Base.h"
#include "AnimGraphDefinitions.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "CoreMinimal.h"
#include "AnimGraphNode_MVAE_Retargeter.generated.h"

/**
 * 
 */
UCLASS()
class MVAE_API UAnimGraphNode_MVAE_Retargeter : public UAnimGraphNode_Base
{

public:
    // Override this function to provide node title in the Animation Blueprint Editor
    virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

    // Override this function to return your custom animation node
    virtual FAnimNode_Base* GetNode() override { return &Node; }

protected:
    // This holds the instance of your custom anim node (defined earlier)
    UPROPERTY(EditAnywhere, Category = "Settings")
    FAnimNode_MVAE_Retargeter Node;
};
