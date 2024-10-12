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

AMVAECharacter::AMVAECharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
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

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

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
                        //checkf(SymbolicInputTensorShape.IsConcrete(), TEXT("The current example supports only models without variable input tensor dimensions"));
                        TArray<UE::NNE::FTensorShape> InputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape),
							UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicInputTensorShape1),
	                        UE::NNE::FTensorShape::MakeFromSymbolic(InputTensorDescs[2].GetShape())
                        };

                        ModelInstance->SetInputTensorShapes(InputTensorShapes);

                        TConstArrayView<UE::NNE::FTensorDesc> OutputTensorDescs = ModelInstance->GetOutputTensorDescs();
                        checkf(OutputTensorDescs.Num() == 1, TEXT("The current example supports only models with a single output tensor"));
                        UE::NNE::FSymbolicTensorShape SymbolicOutputTensorShape = OutputTensorDescs[0].GetShape();
                        checkf(SymbolicOutputTensorShape.IsConcrete(), TEXT("The current example supports only models without variable output tensor dimensions"));
                        TArray<UE::NNE::FTensorShape> OutputTensorShapes = { UE::NNE::FTensorShape::MakeFromSymbolic(SymbolicOutputTensorShape) };

                        TArray<float> ZInputData;

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

                        InputBindings.SetNum(3);
                        InputBindings[0].Data = ZInputData.GetData();
                        InputBindings[0].SizeInBytes = ZInputData.Num() * sizeof(float);
                        if(HistoryCondInputData.Num() == 0)
                        {
                            UE_LOG(LogTemp,Warning,TEXT("init"));
                            HistoryCondInputData.SetNumZeroed(InputTensorShapes[1].Volume());
							GoalPositions.SetNumZeroed(24);
                            GoalRotations.SetNumZeroed(24);

                            RootFullInputData.SetNumZeroed(InputTensorShapes[2].Volume());
                            FQuat InitRootRotation = FQuat(GetActorRotation());
                            RootFullInputData[0] = InitRootRotation.X;
                            RootFullInputData[1] = InitRootRotation.Y;
                            RootFullInputData[2] = InitRootRotation.Z;
                            RootFullInputData[3] = InitRootRotation.W;

                        	FVector InitRootPosition = GetActorLocation();
                            RootFullInputData[4] = InitRootPosition.X;
                            RootFullInputData[5] = InitRootPosition.Y;
                            RootFullInputData[6] = InitRootPosition.Z;
                        }

                        // for (int32 b = 0; b < 1; b++)
                        // {
                        //     UE_LOG(LogClass, Log, TEXT("Names: %f"), ZInputData[b]); 
                        // }
                        InputBindings[1].Data = HistoryCondInputData.GetData();
                        InputBindings[1].SizeInBytes = HistoryCondInputData.Num() * sizeof(float);

                        InputBindings[2].Data = RootFullInputData.GetData();
                        InputBindings[2].SizeInBytes = RootFullInputData.Num() * sizeof(float);


                        OutputData.SetNumZeroed(OutputTensorShapes[0].Volume());
                        OutputBindings.SetNumZeroed(1);
                        OutputBindings[0].Data = OutputData.GetData();
                        OutputBindings[0].SizeInBytes = OutputData.Num() * sizeof(float);

                        ModelInstance->RunSync(InputBindings, OutputBindings);

						//history for the next frame
                        for(int i =0 ; i< 213; i++)
                        {
                            HistoryCondInputData[i] = OutputData[i];
                        }
                        
                        //rootYaw += OutputData[2] * 0.03333333333;

                        for (int i = 213; i < 213 + 72; i += 3)
                        {
                            FVector NewLocation(OutputData[i], OutputData[i + 1], OutputData[i + 2]);
                            if(i==213)
                            {
                                //SetActorLocation(NewLocation);
                                RootFullInputData[4] = OutputData[i];
                                RootFullInputData[5] = OutputData[i + 1];
                                RootFullInputData[6] = OutputData[i + 2];

                            }

                            GoalPositions[int32((i - 213) / 3)] = (NewLocation * 110);
                            SphereActors[int32((i - 213) / 3)]->SetActorLocation((NewLocation * 110), false);

                        }


                        for (int i = 285; i < 285 + 96; i += 4)
                        {
                            FQuat NewRotation(OutputData[i], OutputData[i + 1], OutputData[i + 2], OutputData[i + 3]);

                            if (i == 285)
                            {
                                //SetActorRotation(NewRotation.Rotator());
                                RootFullInputData[0] = OutputData[i];
                                RootFullInputData[1] = OutputData[i+1];
                                RootFullInputData[2] = OutputData[i+2];
                                RootFullInputData[3] = OutputData[i+3];
                            }

                            GoalRotations[(i - 285) / 4] = NewRotation;
                        }
                        /*
                        rootYaw = GetActorRotation().Yaw - 45;
                        rootYaw += OutputData[2] * 0.03333333333;

                        FRotator CurrentRotation = GetActorRotation();
                        CurrentRotation.Yaw = CurrentRotation.Yaw + FMath::DegreesToRadians(OutputData[2] * 0.03333333333);
                        SetActorRotation(CurrentRotation);
                        GoalRotations[0] = CurrentRotation;
                        GoalRotations[0].Yaw = rootYaw;
                        GoalRotations[0].Pitch = 0;
                        GoalRotations[0].Roll = 0;

                        //UE_LOG(LogClass, Warning, TEXT("Names: %f"), rootYaw); 
                        FVector Speed(OutputData[0],OutputData[1] ,0);
                        Speed = Speed.RotateAngleAxis(FMath::RadiansToDegrees(rootYaw), FVector(0, 0, 1));
                        FVector ActorLocation = GetActorLocation();

                        ActorLocation.X += Speed[0] * 0.03333333333 * 100;
                        ActorLocation.Y += Speed[1] * 0.03333333333 * 100;
                        SetActorLocation(ActorLocation);
                        ActorLocation.Z = 0;
                        for (int i = 141; i < 213; i += 3)
                        {
                            FVector NewLocation(OutputData[i] , OutputData[i + 1] , OutputData[i + 2]);
                            NewLocation = NewLocation.RotateAngleAxis(FMath::RadiansToDegrees(rootYaw), FVector(0, 0, 1));

                            // FTransform BodyTransform = FTransform();
                            // BodyTransform.SetLocation(NewLocation);
                            // FRotator RootRotation(0.f,0.f,rootYaw);

                            // FTransform YawTransform = FTransform();
                            // YawTransform.SetRotation(RootRotation.Quaternion());
                            // FTransform TransformedBodyPose = YawTransform * BodyTransform;


                            // NewLocation = TransformedBodyPose.GetLocation();//NewLocation.RotateAngleAxis(rootYaw, FVector(0, 0, 1));
                            // NewLocation.X += CurrentRootPos.X;
                            // NewLocation.Y += CurrentRootPos.Y;
                            GoalPositions[int32((i - 141) / 3)] = ActorLocation + (NewLocation * 110);
                            SphereActors[int32((i - 141) / 3)]->SetActorLocation(ActorLocation + (NewLocation * 110), false);

                        }

                        for (int i = 3; i < 72; i += 3)
                        {
                            //FRotator NewRotation(FMath::RadiansToDegrees(OutputData[i ]) ,FMath::RadiansToDegrees(OutputData[i + 1])   , FMath::RadiansToDegrees(OutputData[i + 2]));
                            FRotator NewRotation(FMath::RadiansToDegrees(OutputData[i+1]), FMath::RadiansToDegrees(OutputData[i]), FMath::RadiansToDegrees(OutputData[i+2]));
                            //UE_LOG(LogClass, Log, TEXT("Names: %f"), FMath::RadiansToDegrees(OutputData[i + 2])); 
                            GoalRotations[int32((i-3) / 3) + 1] = NewRotation;
                        }
                        */
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