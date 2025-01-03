// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"


#include "MVAECharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class AMVAECharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

public:
	AMVAECharacter(const FObjectInitializer& ObjectInitializer);


protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);


protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// To add mapping context
	virtual void BeginPlay();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<AActor*> SphereActors;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UNNEModelData> PreLoadedModelData;

	UPROPERTY(BlueprintReadWrite)
	TArray<FVector> GoalPositions;

	UPROPERTY(BlueprintReadWrite)
	TArray<FRotator> GoalRotations;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UPoseableMeshComponent* PoseableMesh;
	TArray<float> HistoryCondInputData;
	float rootYaw = 0;
	FVector CurrentRootPos;


	TArray<TArray<int32>> Permutations;
	TArray<FVector> SignCombinations;
	int32 PermutationIndex;
	int32 SignIndex;

	int counter = 0;

	void AdvanceIndices()
	{
		SignIndex++;
		if (SignIndex >= SignCombinations.Num())
		{
			SignIndex = 0;
			PermutationIndex++;

			if (PermutationIndex >= Permutations.Num())
			{
				PermutationIndex = 0;  // Reset to the beginning when all combinations are used
			}
		}
	}
};
