// Fill out your copyright notice in the Description page of Project Settings.


#include "MVAE_VisualizerActor.h"
#include "Components/StaticMeshComponent.h"
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"

AMVAE_VisualizerActor::AMVAE_VisualizerActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneComponent = CreateDefaultSubobject<USceneComponent>("Root Component");
    SetRootComponent(SceneComponent);

}

// Called when the game starts or when spawned
void AMVAE_VisualizerActor::BeginPlay()
{
	Super::BeginPlay();

    RootComponent->SetWorldScale3D(FVector(1.f, 1.f, 1.f));
    for (int32 i = 0; i < 24; i++)
    {
        AActor* SphereActor = GetWorld()->SpawnActor<AActor>(ActorToSpawn);
        SphereActors.Add(SphereActor);

        float XOffset = (i % 5) * 100.0f; // 5 spheres in each row
        float YOffset = (i / 5) * 100.0f; // 4 rows in total
        SphereActor->SetActorLocation(FVector(XOffset, YOffset, 0.0f));
    }
    

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(
        TimerHandle,  // The timer handle
        [this]()      // The Lambda expression
        {
            

            TWeakInterfacePtr<INNERuntimeCPU> Runtime = UE::NNE::GetRuntime<INNERuntimeCPU>(FString("NNERuntimeORTCpu"));
            if (Runtime.IsValid())
            {
                TUniquePtr<UE::NNE::IModelCPU> Model = Runtime->CreateModel(PreLoadedModelData);
                if (Model.IsValid())
                {
                    TUniquePtr<UE::NNE::IModelInstanceCPU> ModelInstance = Model->CreateModelInstance();
                    if (ModelInstance.IsValid())
                    {
                        TConstArrayView<UE::NNE::FTensorDesc> InputTensorDescs = ModelInstance->GetInputTensorDescs();

                        UE::NNE::FSymbolicTensorShape SymbolicInputTensorShape = InputTensorDescs[0].GetShape();
                        UE::NNE::FSymbolicTensorShape SymbolicInputTensorShape1 = InputTensorDescs[1].GetShape();
                        checkf(SymbolicInputTensorShape.IsConcrete(), TEXT("The current example supports only models without variable input tensor dimensions"));
                        TArray<UE::NNE::FTensorShape> InputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape),
                        UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape1) };

                        ModelInstance->SetInputTensorShapes(InputTensorShapes);

                        TConstArrayView<UE::NNE::FTensorDesc> OutputTensorDescs = ModelInstance->GetOutputTensorDescs();
                        checkf(OutputTensorDescs.Num() == 1, TEXT("The current example supports only models with a single output tensor"));
                        UE::NNE::FSymbolicTensorShape SymbolicOutputTensorShape = OutputTensorDescs[0].GetShape();
                        checkf(SymbolicOutputTensorShape.IsConcrete(), TEXT("The current example supports only models without variable output tensor dimensions"));
                        TArray<UE::NNE::FTensorShape> OutputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicOutputTensorShape) };

                        TArray<float> ZInputData;
                        TArray<float> CondInputData;
                        TArray<float> OutputData;
                        TArray<UE::NNE::FTensorBindingCPU> InputBindings;
                        TArray<UE::NNE::FTensorBindingCPU> OutputBindings;

                        ZInputData.SetNumZeroed(InputTensorShapes[0].Volume());
                        for(auto& value : ZInputData)
                        {
                            value = FMath::RandRange(-3.0, 3.0);
                        }
                        InputBindings.SetNumZeroed(2);
                        InputBindings[0].Data = ZInputData.GetData();
                        InputBindings[0].SizeInBytes = ZInputData.Num() * sizeof(float);
                        if(HistoryCondInputData.Num() == 0)
                        {
                            HistoryCondInputData.SetNumZeroed(InputTensorShapes[1].Volume());
                            for (auto& value : HistoryCondInputData)
                            {
                                value = FMath::RandRange(-1.0, 1.0);
                            }
                        }
                    	CondInputData.SetNumZeroed(InputTensorShapes[1].Volume());
                        CondInputData = HistoryCondInputData;
                        InputBindings[1].Data = CondInputData.GetData();
                        InputBindings[1].SizeInBytes = CondInputData.Num() * sizeof(float);

                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        if (ModelInstance->RunSync(InputBindings, OutputBindings) != 0)
                        {
                            UE_LOG(LogTemp, Error, TEXT("Failed to run the model"));
                        }

						//history for the next frame
                        HistoryCondInputData = (OutputData);
                        
                        for (int i = 141; i < 213; i += 3)
                        {
                            FVector NewLocation(OutputData[i], OutputData[i + 1], OutputData[i + 2]);
                            NewLocation.RotateAngleAxis(FMath::RadiansToDegrees(OutputData[2]), FVector(0, 0, 1));
                            SphereActors[int32((i - 141) / 3)]->SetActorLocation(NewLocation * 100, false);

                        }
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("Failed to create the model instance"));
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to create the model"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Cannot find runtime NNERuntimeORTCpu, please enable the corresponding plugin"));
            }
            
        },.35f,
        true
    ); 
}

// Called every frame
void AMVAE_VisualizerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
