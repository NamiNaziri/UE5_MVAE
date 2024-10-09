// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MVAE_VisualizerActor.generated.h"

UCLASS()
class MVAE_API AMVAE_VisualizerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMVAE_VisualizerActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditDefaultsOnly)
	USceneComponent* SceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<AActor*> SphereActors;

	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AActor> ActorToSpawn;

	UPROPERTY(EditAnywhere)
		TObjectPtr<class UNNEModelData> PreLoadedModelData;


		TArray<float> HistoryCondInputData;
		float rootYaw = 0;
		FVector CurrentRootPos;
};
