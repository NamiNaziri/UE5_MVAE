// Copyright Epic Games, Inc. All Rights Reserved.

#include "MVAECharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "NNE.h"
#include "NNERuntimeCPU.h"
#include "NNEModelData.h"
#include <random>

#include "UObject/WeakInterfacePtr.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMVAECharacter

AMVAECharacter::AMVAECharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    PoseableMesh = CreateDefaultSubobject<UPoseableMeshComponent>("Poseable Mesh");
    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

    // Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
    // instead of recompiling to adjust them
    GetCharacterMovement()->JumpZVelocity = 700.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom (pulls in towards the player if there is a collision)
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
    CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
    FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

    Permutations = {
    {0, 1, 2},
    {0, 2, 1},
    {1, 0, 2},
    {1, 2, 0},
    {2, 0, 1},
    {2, 1, 0}
    };

    // Define sign combinations
    SignCombinations = {
        {1, 1, 1},
        {-1, 1, 1},
        {1, -1, 1},
        {1, 1, -1},
        {-1, -1, 1},
        {-1, 1, -1},
        {1, -1, -1},
        {-1, -1, -1}
    };
    PermutationIndex = 3;
    SignIndex = 3;
    // Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
    // are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

/**
 * 
 */
void AMVAECharacter::BeginPlay()
{
    // Call the base class  
    Super::BeginPlay();

    //Add Input Mapping Context
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
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
                        //SetActorRotation(CurrentRotation);

                        //UE_LOG(LogClass, Log, TEXT("Names: %f"), rootYaw);
                        FVector Speed(OutputData[0], OutputData[1], 0);
                        Speed = Speed.RotateAngleAxis((rootYaw), FVector(0, 0, 1));
                        FVector ActorLocation = GetActorLocation();

                        ActorLocation.X += Speed[0] * 0.03333333333 * 100;
                        ActorLocation.Y += Speed[1] * 0.03333333333 * 100;
                        //SetActorLocation(ActorLocation);
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
	                        {1,TEXT("LeftUpLeg")},
	                        {2,TEXT("LeftLeg")},
	                        {3,TEXT("LeftFoot")},
	                        {4,TEXT("LeftToeBase")},
	                        {5,TEXT("RightUpLeg")},
	                        {6,TEXT("RightLeg")},
	                        {7,TEXT("RightFoot")},
	                        {8,TEXT("RightToeBase")},
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
                        //TArray<FQuat> ComponentRotation;
                        
                        TArray<int32> ParentIndices = { -1, 0, 1, 2, 3, 0, 5, 6, 7, 0, 9, 10, 11, 12, 11, 14, 15, 16,
                                17, 11, 19, 20, 21, 22 };
                      /*  FVector RootRotationVector((OutputData[3]), (OutputData[4]), (OutputData[5]));
                        float RootAngle = RootRotationVector.Size();
                        FVector RootAxis = RootRotationVector.GetSafeNormal();
                        FQuat RootQuat(RootAxis, RootAngle);*/
                        //ComponentRotation.Add(FQuat(FRotator(0,0,0)));
                        //GoalRotations[0] = RootQuat.Rotator();
                        //PoseableMesh->BoneSpaceTransforms[0].SetRotation(FQuat(-RootQuat.Z, RootQuat.Y, -RootQuat.X, RootQuat.W));
                        //PoseableMesh->SetBoneRotationByName("Hips", RootQuat.Rotator(), EBoneSpaces::ComponentSpace);
                        
                        PoseableMesh->SetBoneLocationByName("Hips", GetActorLocation(), EBoneSpaces::WorldSpace);

                        const TArray<int32>& Perm = Permutations[3];
                        const FVector& Sign = SignCombinations[5];

                        for (int i = 3; i < 72; i += 3)
                        {
                            
                            FVector RotationVector((OutputData[i]) , (OutputData[i+1]), (OutputData[i + 2]));

	                        //UE_LOG(LogClass, Log, TEXT("Names: %s"), ); 
	                        float Angle = RotationVector.Size();  // This gives the magnitude (angle in radians)
	                        FVector Axis = RotationVector.GetSafeNormal();  // Normalizes the vector to get the axis
	                        FQuat PoseQuat = FQuat(Axis, Angle);
                            TArray<double> Values{ PoseQuat.X, PoseQuat.Y, PoseQuat.Z };
                            FVector PermutedValues(
                                Values[Perm[0]] * Sign.X,
                                Values[Perm[1]] * Sign.Y,
                                Values[Perm[2]] * Sign.Z
                            );
                            FQuat NewQuat(PermutedValues.X, PermutedValues.Y, PermutedValues.Z, 1.0f);
                            UE_LOG(LogClass, Log, TEXT("p: %i, s: %i"), PermutationIndex, SignIndex);
                            counter++;
                            if(counter >= 3000)
                            {
                                counter = 0;
                                AdvanceIndices();
                            }
                            
	                        int32 index = int32((i - 3) / 3) + 1;
                            //UE_LOG(LogClass, Log, TEXT("Index: %i"), index);

	                        PoseQuat.Normalize(); 
                           //int32 ParentIndex = ParentIndices[index];
                           //FQuat BoneRotation = ComponentRotation[ParentIndex] * NewQuat;
                           //ComponentRotation.Add(BoneRotation);
                           if (MixamoBodyPartMap.Contains(index)) 
                           {
                               //PoseableMesh->SetBoneRotationByName(MixamoBodyPartMap[index], BoneRotation.Rotator(), EBoneSpaces::WorldSpace);
                               //PoseableMesh->SetBoneRotationByName(MixamoBodyPartMap[index], FRotator(0,0,0), EBoneSpaces::ComponentSpace);
                               
                          /*     const FTransform refTransform = PoseableMesh->GetSkinnedAsset()->GetRefSkeleton().GetRefBonePose()[PoseableMesh->GetBoneIndex(MixamoBodyPartMap[index])];
                               FQuat temp = refTransform.GetRotation();*/
                               
                               //PoseableMesh->BoneSpaceTransforms[PoseableMesh->GetBoneIndex(MixamoBodyPartMap[index])] = refTransform;
                               //PoseableMesh->SetBoneTransformByName(MixamoBodyPartMap[index], refTransform, EBoneSpaces::ComponentSpace);
                           	   PoseableMesh->SetBoneRotationByName(MixamoBodyPartMap[index], NewQuat.Rotator(), EBoneSpaces::ComponentSpace);
                               //PoseableMesh->BoneSpaceTransforms[PoseableMesh->GetBoneIndex(MixamoBodyPartMap[index])].SetRotation(NewQuat);
                               //UE_LOG(LogClass, Log, TEXT("Names: %s, Rotation: %s"), *MixamoBodyPartMap[index].ToString(), *NewQuat.Rotator().ToString());
                           }

                            GoalRotations[int32((i - 3) / 3) + 1] = NewQuat.Rotator();
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

//////////////////////////////////////////////////////////////////////////
// Input

void AMVAECharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Set up action bindings
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

        // Jumping
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

        // Moving
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMVAECharacter::Move);

        // Looking
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMVAECharacter::Look);
    }
    else
    {
        UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
    }
}


void AMVAECharacter::Move(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // find out which way is forward
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        // get forward vector
        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

        // get right vector 
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        // add movement 
        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
}

void AMVAECharacter::Look(const FInputActionValue& Value)
{
    // input is a Vector2D
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        // add yaw and pitch input to controller
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}