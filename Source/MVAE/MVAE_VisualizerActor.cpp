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
    PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>("Poseable Mesh");
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
                        for (auto& value : ZInputData)
                        {
                            std::random_device rd{};
                            std::mt19937 gen{ rd() };
                            std::normal_distribution<float> d{ 0.f, 1.0f };
                            value = d(gen);
                        }
                        InputBindings.SetNum(2);
                        InputBindings[0].Data = ZInputData.GetData();
                        InputBindings[0].SizeInBytes = ZInputData.Num() * sizeof(float);
                        if (HistoryCondInputData.Num() == 0)
                        {
                            UE_LOG(LogTemp, Warning, TEXT("init"));
                            HistoryCondInputData.SetNumZeroed(InputTensorShapes[1].Volume());
                            GoalPositions.SetNumZeroed(24);
                            GoalRotations.SetNumZeroed(24);

                        }

                        // for (int32 b = 0; b < 1; b++)
                        // {
                        //     UE_LOG(LogClass, Log, TEXT("Names: %f"), ZInputData[b]); 
                        // }
                        InputBindings[1].Data = HistoryCondInputData.GetData();
                        InputBindings[1].SizeInBytes = HistoryCondInputData.Num() * sizeof(float);

                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        ModelInstance->RunSync(InputBindings, OutputBindings);

                        //history for the next frame
                        HistoryCondInputData = (OutputData);
                        //rootYaw += OutputData[2] * 0.03333333333;


                        rootYaw = GetActorRotation().Yaw;
                        rootYaw += FMath::RadiansToDegrees(OutputData[2]) * 0.03333333333;

                        FRotator CurrentRotation = GetActorRotation();
                        CurrentRotation.Yaw = CurrentRotation.Yaw + FMath::RadiansToDegrees(OutputData[2]) * 0.03333333333;
                        SetActorRotation(CurrentRotation);
                        GoalRotations[0] = CurrentRotation;
                        GoalRotations[0].Yaw = rootYaw;
                        GoalRotations[0].Pitch = 0;
                        GoalRotations[0].Roll = 0;

                        //UE_LOG(LogClass, Log, TEXT("Names: %f"), rootYaw);
                        FVector Speed(OutputData[0], OutputData[1], 0);
                        Speed = Speed.RotateAngleAxis((rootYaw), FVector(0, 0, 1));
                        FVector ActorLocation = GetActorLocation();

                        ActorLocation.X += Speed[0] * 0.03333333333 * 100;
                        ActorLocation.Y += Speed[1] * 0.03333333333 * 100;
                        SetActorLocation(ActorLocation);
                        ActorLocation.Z = 0;
                        for (int i = 141; i < 213; i += 3)
                        {
                            FVector NewLocation(OutputData[i], OutputData[i + 1], OutputData[i + 2]);
                            NewLocation = NewLocation.RotateAngleAxis((rootYaw), FVector(0, 0, 1));

                            GoalPositions[int32((i - 141) / 3)] = ActorLocation + (NewLocation * 110);
                            SphereActors[int32((i - 141) / 3)]->SetActorLocation(ActorLocation + (NewLocation * 110), false);

                        }
                        TMap<int32, FName> MixamoBodyPartMap = {
                        {0,TEXT("Hips1")},
                        {1,TEXT("rightUpLeg")},
                        {2,TEXT("rightLeg")},
                        {3,TEXT("rightFoot")},
                        {4,TEXT("rightToeBase")},
                        {5,TEXT("LeftUpLeg")},
                        {6,TEXT("LeftLeg")},
                        {7,TEXT("LeftFoot")},
                        {8,TEXT("LeftToeBase")},
                        {9,TEXT("Spine")},
                        {10,TEXT("Spine1")},
                        {11,TEXT("Spine2")},
                        {12,TEXT("Neck")},
                        {13,TEXT("Head")},
                        {14,TEXT("LeftShoulder")},
                        {15,TEXT("LeftArm")},
                        {16,TEXT("LeftForeArm")},
                        {17,TEXT("LeftHand")},
                        {-1,TEXT("hh1")},

                        {19,TEXT("RightShoulder")},
                        {20,TEXT("RightArm")},
                        {21,TEXT("RightForeArm")},
                        {22,TEXT("RightHand")},
                        {-1, TEXT("hh2")},
                        };
                        TArray<FQuat> ComponentRotation;
                        ComponentRotation.Add(FQuat(GoalRotations[0]));
                        TArray<int32> ParentIndices = { -1, 0, 1, 2, 3, 0, 5, 6, 7, 0, 9, 10, 11, 12, 11, 14, 15, 16,
                                17, 11, 19, 20, 21, 22 };
                        for (int i = 3; i < 72; i += 3)
                        {
                            UE_LOG(LogClass, Log, TEXT("Names: %i"), int32((i - 3) / 3) + 1);
                            FRotator NewRotation(FMath::RadiansToDegrees(OutputData[i + 2]), FMath::RadiansToDegrees(OutputData[i]), FMath::RadiansToDegrees(OutputData[i + 1]));
                            int32 index = int32((i - 3) / 3) + 1;

                            int32 ParentIndex = ParentIndices[index];
                            ComponentRotation.Add(ComponentRotation[ParentIndex] * FQuat(NewRotation));
                            if (MixamoBodyPartMap.Contains(index))
                            {
                                PoseableMesh->SetBoneRotationByName(MixamoBodyPartMap[index], ComponentRotation[index].Rotator(), EBoneSpaces::ComponentSpace);
                            }

                            GoalRotations[int32((i - 3) / 3) + 1] = NewRotation;
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

        }, .0666666666f,
        true
            );
}

// Called every frame
void AMVAE_VisualizerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
