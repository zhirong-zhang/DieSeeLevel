// Copyright Epic Games, Inc. All Rights Reserved.

#include "G_004Character.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "G_004.h"

AG_004Character::AG_004Character()
{
    // enable Tick
    PrimaryActorTick.bCanEverTick = true;

    // Set size for collision capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    // Don't rotate when the controller rotates. Let that just affect the camera.
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = 500.f;
    GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

    // Create a camera boom
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 400.0f;
    CameraBoom->bUsePawnControlRotation = true;

    // Create a follow camera
    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    // initialize gravity direction arrow
    GravityArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("GravityArrow"));
    GravityArrow->SetupAttachment(RootComponent);
    GravityArrow->ArrowSize = 3.0f;
    GravityArrow->SetHiddenInGame(false);

    bIsAimingGravity = false;

    // initialize gyro state
    CurrentGyroRotation = FRotator::ZeroRotator;
    StartGyroRotation = FRotator::ZeroRotator;
}

void AG_004Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {

       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
       EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
       EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AG_004Character::Move);
       EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AG_004Character::Look);
       EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AG_004Character::Look);

       if (GyroDataAction)
       {
           EnhancedInputComponent->BindAction(GyroDataAction, ETriggerEvent::Triggered, this, &AG_004Character::InputGyroData);
       }
       if (GyroGravityAction)
       {
           EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Started, this, &AG_004Character::GyroGravityStarted);
           EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Completed, this, &AG_004Character::GyroGravityCompleted);
       }
    }
    else
    {
       UE_LOG(LogG_004, Error, TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
    }
}

void AG_004Character::InputGyroData(const FInputActionValue& Value)
{
    FVector GyroVector = Value.Get<FVector>();
    CurrentGyroRotation = FRotator(GyroVector.Y, GyroVector.Z, GyroVector.X);
}

void AG_004Character::GyroGravityStarted()
{
    StartGyroRotation = CurrentGyroRotation;
    bIsAimingGravity = true;

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("Button pressed — gravity aim mode active."));
    }
}

void AG_004Character::GyroGravityCompleted()
{
    bIsAimingGravity = false;

    // 1. compute relative offset and normalize
    FRotator DeltaRotation = CurrentGyroRotation - StartGyroRotation;
    DeltaRotation.Normalize();

    // 2. rotate the default gravity vector
    FVector DefaultGravityDir(0.0f, 0.0f, -1.0f);
    FVector RawNewGravity = DeltaRotation.RotateVector(DefaultGravityDir);
    FVector SnappedGravity = FVector::ZeroVector;
    float MaxAxisValue = -1.0f;

    // 3. snap to nearest 90-degree axis
    if (FMath::Abs(RawNewGravity.X) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.X); SnappedGravity = FVector(FMath::Sign(RawNewGravity.X), 0.0f, 0.0f); }
    if (FMath::Abs(RawNewGravity.Y) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Y); SnappedGravity = FVector(0.0f, FMath::Sign(RawNewGravity.Y), 0.0f); }
    if (FMath::Abs(RawNewGravity.Z) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Z); SnappedGravity = FVector(0.0f, 0.0f, FMath::Sign(RawNewGravity.Z)); }

    // 4. apply the new gravity direction
    GetCharacterMovement()->SetGravityDirection(SnappedGravity);

    // 5. realign camera up to the new gravity's opposite
    if (AController* PC = GetController())
    {
        FVector NewUpDirection = -SnappedGravity;
        FVector CurrentCameraForward = PC->GetControlRotation().Vector();
        FRotator NewCameraRotation = FRotationMatrix::MakeFromZX(NewUpDirection, CurrentCameraForward).Rotator();
        PC->SetControlRotation(NewCameraRotation);
    }

    if (GEngine)
    {
        FString DebugMsg = FString::Printf(TEXT("Released — gravity switched to: %s"), *SnappedGravity.ToString());
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, DebugMsg);
    }
}

void AG_004Character::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void AG_004Character::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AG_004Character::DoMove(float Right, float Forward)
{
    if (GetController() != nullptr)
    {
       const FRotator Rotation = GetController()->GetControlRotation();
       const FRotator YawRotation(0, Rotation.Yaw, 0);

       const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
       const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

       AddMovementInput(ForwardDirection, Forward);
       AddMovementInput(RightDirection, Right);
    }
}

void AG_004Character::DoLook(float Yaw, float Pitch)
{
    if (GetController() != nullptr)
    {
       AddControllerYawInput(Yaw);
       AddControllerPitchInput(Pitch);
    }
}

void AG_004Character::DoJumpStart()
{
    Jump();
}

void AG_004Character::DoJumpEnd()
{
    StopJumping();
}

void AG_004Character::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsAimingGravity && GravityArrow)
    {
        // aim mode: preview the gravity direction in real time

        // 1. compute the gravity direction that would be applied on release
        FRotator DeltaRotation = CurrentGyroRotation - StartGyroRotation;
        DeltaRotation.Normalize();

        FVector DefaultGravityDir(0.0f, 0.0f, -1.0f);
        FVector RawNewGravity = DeltaRotation.RotateVector(DefaultGravityDir);
        FVector SnappedGravity = FVector::ZeroVector;
        float MaxAxisValue = -1.0f;

        if (FMath::Abs(RawNewGravity.X) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.X); SnappedGravity = FVector(FMath::Sign(RawNewGravity.X), 0.0f, 0.0f); }
        if (FMath::Abs(RawNewGravity.Y) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Y); SnappedGravity = FVector(0.0f, FMath::Sign(RawNewGravity.Y), 0.0f); }
        if (FMath::Abs(RawNewGravity.Z) > MaxAxisValue) { MaxAxisValue = FMath::Abs(RawNewGravity.Z); SnappedGravity = FVector(0.0f, 0.0f, FMath::Sign(RawNewGravity.Z)); }

        // 2. predict the character's forward direction after landing on the new surface
        FVector NewUpDirection = -SnappedGravity;         // new "ceiling" direction
        FVector CurrentForward = GetActorForwardVector(); // current facing direction

        // project current forward onto the new gravity plane to get post-landing forward
        FVector PredictedForward = FVector::VectorPlaneProject(CurrentForward, NewUpDirection).GetSafeNormal();

        // fallback: if facing exactly perpendicular to the new surface, use right vector instead
        if (PredictedForward.IsNearlyZero())
        {
            PredictedForward = FVector::VectorPlaneProject(GetActorRightVector(), NewUpDirection).GetSafeNormal();
        }

        GravityArrow->SetWorldRotation(PredictedForward.Rotation());
        GravityArrow->SetArrowColor(FColor::Yellow); // preview color
    }
    else if (GravityArrow)
    {
        // normal mode: show current actual facing direction
        FVector CurrentForward = GetActorForwardVector();
        GravityArrow->SetWorldRotation(CurrentForward.Rotation());
        GravityArrow->SetArrowColor(FColor::Green); // locked color
    }
}
