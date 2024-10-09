// Fill out your copyright notice in the Description page of Project Settings.


#include "MVAE_VisualizerActor.h"
#include "Components/StaticMeshComponent.h"
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#include <random>

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
                TSharedPtr<UE::NNE::IModelCPU> Model = Runtime->CreateModelCPU(PreLoadedModelData);
                if (Model.IsValid())
                {
                    TSharedPtr<UE::NNE::IModelInstanceCPU> ModelInstance = Model->CreateModelInstanceCPU();
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
                            std::random_device rd{};
                            std::mt19937 gen{ rd() };
                            std::normal_distribution<float> d{ 0.f, 1.0f };
                            value =d(gen);
                        }
                        InputBindings.SetNum(2);
                        InputBindings[0].Data = ZInputData.GetData();
                        InputBindings[0].SizeInBytes = ZInputData.Num() * sizeof(float);
                        if(HistoryCondInputData.Num() == 0)
                        {
                            UE_LOG(LogTemp,Warning,TEXT("init"));
                            HistoryCondInputData.SetNumZeroed(InputTensorShapes[1].Volume());


                        }

                        for (int32 b = 0; b < 1; b++)
                        {
                            UE_LOG(LogClass, Log, TEXT("Names: %f"), ZInputData[b]); 
                        }
                        InputBindings[1].Data = HistoryCondInputData.GetData();
                        InputBindings[1].SizeInBytes = HistoryCondInputData.Num() * sizeof(float);

                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        ModelInstance->RunSync(InputBindings, OutputBindings);

						//history for the next frame
                        HistoryCondInputData = (OutputData);
                        rootYaw += OutputData[2] * 0.03333333333;
                        FVector Speed(OutputData[0],OutputData[1] ,0);
                        Speed = Speed.RotateAngleAxis(FMath::RadiansToDegrees(rootYaw), FVector(0, 0, 1));
                        CurrentRootPos.X += Speed[0] * 0.03333333333 ;
                        CurrentRootPos.Y += Speed[1] * 0.03333333333 ;
                        for (int i = 141; i < 213; i += 3)
                        {
                            FVector NewLocation(OutputData[i] , OutputData[i + 1] , OutputData[i + 2]);
                            FTransform BodyTransform = FTransform();
                            BodyTransform.SetLocation(NewLocation);
                            FRotator RootRotation(0.f,0.f,rootYaw);

                            FTransform YawTransform = FTransform();
                            YawTransform.SetRotation(RootRotation.Quaternion());
                            FTransform TransformedBodyPose = YawTransform * BodyTransform;


                            NewLocation = TransformedBodyPose.GetLocation();//NewLocation.RotateAngleAxis(rootYaw, FVector(0, 0, 1));
                            NewLocation.X += CurrentRootPos.X;
                             NewLocation.Y += CurrentRootPos.Y;
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
            
        },.0666666666f,
        true
    ); 
}

// Called every frame
void AMVAE_VisualizerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
