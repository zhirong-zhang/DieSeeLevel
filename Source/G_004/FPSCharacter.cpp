// Copyright Epic Games, Inc. All Rights Reserved.

#include "FPSCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "Kismet/GameplayStatics.h"

AFPSCharacter::AFPSCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetCapsuleComponent()->InitCapsuleSize(40.f, 96.f);

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));

    FirstPersonCamera->bUsePawnControlRotation = false;

    CurrentCameraPitch = 0.0f;

    GravityPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GravityPlane"));
    GravityPlane->SetupAttachment(GetMesh());
    GravityPlane->SetRelativeLocation(FVector(100.f, 0.f, 50.f));

    GravityArrow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GravityArrow"));
    GravityArrow->SetupAttachment(GravityPlane);
    GravityArrow->SetRelativeLocation(FVector(0.f, 0.f, 10.f));

    bIsShiftingGravity = false;
    CurrentGyroRotation = FRotator::ZeroRotator;
    AccumulatedPitch = 0.0f;
    AccumulatedRoll = 0.0f;

    bIsSmoothRotating = false;
    TargetActorRotation = FRotator::ZeroRotator;
    SmoothRotationSpeed = 10.0f;
}

void AFPSCharacter::BeginPlay()
{
    Super::BeginPlay();

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = false;

    // hide the gravity indicator and its children on game start
    if (GravityPlane)
    {
        GravityPlane->SetVisibility(false, true);
    }
}

void AFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::Look);
        if (MouseLookAction)
        {
            EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AFPSCharacter::MouseLook);
        }

        if (GyroDataAction)
        {
            EnhancedInputComponent->BindAction(GyroDataAction, ETriggerEvent::Triggered, this, &AFPSCharacter::InputGyroData);
        }
        if (GyroGravityAction)
        {
            EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Started, this, &AFPSCharacter::GyroGravityStarted);
            EnhancedInputComponent->BindAction(GyroGravityAction, ETriggerEvent::Completed, this, &AFPSCharacter::GyroGravityCompleted);
        }
    }
}

void AFPSCharacter::InputGyroData(const FInputActionValue& Value)
{
    FVector GyroVector = Value.Get<FVector>();
    CurrentGyroRotation = FRotator(GyroVector.Y, GyroVector.Z, GyroVector.X);
}

void AFPSCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsShiftingGravity && GravityPlane)
    {
        float PitchDelta = -CurrentGyroRotation.Pitch * DeltaTime * GyroGravitySensitivity;
        float RollDelta  =  CurrentGyroRotation.Roll  * DeltaTime * GyroGravitySensitivity;

        float AbsPitch = FMath::Abs(PitchDelta);
        float AbsRoll  = FMath::Abs(RollDelta);

        float PitchMultiplier = 1.0f;
        float RollMultiplier  = 1.0f;

        if (AbsPitch > AbsRoll && AbsPitch > 0.01f)
        {
            AccumulatedPitch += (PitchDelta * PitchMultiplier);
        }
        else if (AbsRoll > AbsPitch && AbsRoll > 0.01f)
        {
            AccumulatedRoll += (RollDelta * RollMultiplier);
        }

        FQuat QPitch(FVector::RightVector, FMath::DegreesToRadians(AccumulatedPitch));
        FQuat QRoll(FVector::ForwardVector, FMath::DegreesToRadians(AccumulatedRoll));
        FQuat NewRelativeRot = InitialPlaneQuat * QPitch * QRoll;

        GravityPlane->SetRelativeRotation(NewRelativeRot);
    }
    else if (bIsSmoothRotating)
    {
        FRotator CurrentActorRot = GetActorRotation();
        FRotator NewActorRot = FMath::RInterpTo(CurrentActorRot, TargetActorRotation, DeltaTime, SmoothRotationSpeed);

        SetActorRotation(NewActorRot);
        if (Controller)
        {
            Controller->SetControlRotation(NewActorRot);
        }

        float AngleDifference = FMath::RadiansToDegrees(NewActorRot.Quaternion().AngularDistance(TargetActorRotation.Quaternion()));

        if (AngleDifference < 1.0f)
        {
            SetActorRotation(TargetActorRotation);
            if (Controller)
            {
                Controller->SetControlRotation(TargetActorRotation);
            }

            FVector SnappedUp = TargetActorRotation.Quaternion().GetAxisZ();
            GetCharacterMovement()->SetGravityDirection(-SnappedUp);
            GetCharacterMovement()->SetMovementMode(MOVE_Falling);

            bIsSmoothRotating = false;

            // gravity aligned — trigger level refresh
            SpawnRoomObjects();

            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("[Gravity aligned — level objects spawned]"));
            }
        }
    }
}

void AFPSCharacter::GyroGravityStarted()
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    bIsShiftingGravity = true;
    AccumulatedPitch = 0.0f;
    AccumulatedRoll = 0.0f;

    if (GravityPlane)
    {
        InitialPlaneQuat = GravityPlane->GetRelativeRotation().Quaternion();

        // show the gravity indicator on button press
        GravityPlane->SetVisibility(true, true);
    }

    GetCharacterMovement()->Velocity = FVector::ZeroVector;
    GetCharacterMovement()->SetMovementMode(MOVE_None);

    // snap camera pitch to zero so the player aims from a neutral horizontal baseline
    CurrentCameraPitch = 0.0f;
    if (FirstPersonCamera)
    {
        FirstPersonCamera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Yellow, TEXT("[Gravity aim mode active]"));
    }
}

void AFPSCharacter::GyroGravityCompleted()
{
    if (!bIsShiftingGravity) return;

    bIsShiftingGravity = false;

    if (GravityArrow)
    {
        FVector CurrentUp = GravityArrow->GetComponentQuat().GetAxisZ();
        FVector CurrentForward = GravityArrow->GetComponentQuat().GetAxisX();

        FVector SnappedUp = SnapVectorToClosestAxis(CurrentUp);
        FVector ProjectedForward = FVector::VectorPlaneProject(CurrentForward, SnappedUp).GetSafeNormal();
        FVector SnappedForward = SnapVectorToClosestAxis(ProjectedForward);

        FRotator FinalSnappedRot = FRotationMatrix::MakeFromXZ(SnappedForward, SnappedUp).Rotator();

        TargetActorRotation = FinalSnappedRot;
        bIsSmoothRotating = true;

        if (GravityPlane)
        {
            GravityPlane->SetRelativeRotation(InitialPlaneQuat);

            // hide the gravity indicator on button release
            GravityPlane->SetVisibility(false, true);
        }
    }
}

FVector AFPSCharacter::SnapVectorToClosestAxis(const FVector& InVector)
{
    FVector Snapped = FVector::ZeroVector;
    float MaxVal = -1.0f;

    if (FMath::Abs(InVector.X) > MaxVal) { MaxVal = FMath::Abs(InVector.X); Snapped = FVector(FMath::Sign(InVector.X), 0.f, 0.f); }
    if (FMath::Abs(InVector.Y) > MaxVal) { MaxVal = FMath::Abs(InVector.Y); Snapped = FVector(0.f, FMath::Sign(InVector.Y), 0.f); }
    if (FMath::Abs(InVector.Z) > MaxVal) { MaxVal = FMath::Abs(InVector.Z); Snapped = FVector(0.f, 0.f, FMath::Sign(InVector.Z)); }

    return Snapped;
}

void AFPSCharacter::Move(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D MovementVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddMovementInput(GetActorForwardVector(), MovementVector.Y);
        AddMovementInput(GetActorRightVector(), MovementVector.X);
    }
}

void AFPSCharacter::Look(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D Axis = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        float YawInput   = Axis.X * GamepadLookSensitivity * GetWorld()->GetDeltaSeconds();
        float PitchInput = -Axis.Y * GamepadLookSensitivity * GetWorld()->GetDeltaSeconds();

        FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(YawInput));
        AddActorLocalRotation(YawRotation);

        CurrentCameraPitch = FMath::Clamp(CurrentCameraPitch + PitchInput, -89.0f, 89.0f);
        FirstPersonCamera->SetRelativeRotation(FRotator(CurrentCameraPitch, 0.f, 0.f));
    }
}

void AFPSCharacter::MouseLook(const FInputActionValue& Value)
{
    if (bIsShiftingGravity || bIsSmoothRotating) return;

    FVector2D Axis = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        float YawInput   = Axis.X * MouseLookSensitivity;
        float PitchInput = -Axis.Y * MouseLookSensitivity;

        FQuat YawRotation(FVector::UpVector, FMath::DegreesToRadians(YawInput));
        AddActorLocalRotation(YawRotation);

        CurrentCameraPitch = FMath::Clamp(CurrentCameraPitch + PitchInput, -89.0f, 89.0f);
        FirstPersonCamera->SetRelativeRotation(FRotator(CurrentCameraPitch, 0.f, 0.f));
    }
}

bool AFPSCharacter::TryFindRandomWallSpawnPoint(FVector& OutLocation, FRotator& OutRotation)
{
    FVector StartLocation = GetActorLocation();

    // max 10 attempts — don't stall the frame
    for (int32 i = 0; i < 10; ++i)
    {
        FVector RandomDirection = FMath::VRand();
        FVector EndLocation = StartLocation + (RandomDirection * 3000.0f);

        FHitResult HitResult;
        FCollisionQueryParams QueryParams;
        QueryParams.AddIgnoredActor(this);

        if (GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
        {
            AActor* HitActor = HitResult.GetActor();
            if (HitActor)
            {
                DrawDebugLine(GetWorld(), StartLocation, HitResult.ImpactPoint, FColor::Red, false, 5.0f);

                FString HitName = HitActor->GetName();
                FString InfoMsg = FString::Printf(TEXT("[Debug] Hit actor: %s"), *HitName);

                // only surfaces tagged SpawnableWall are valid spawn points
                if (HitActor->ActorHasTag(FName("SpawnableWall")))
                {
                    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("[OK] Hit valid wall: %s"), *HitName));
                    DrawDebugBox(GetWorld(), HitResult.ImpactPoint, FVector(10.0f), FColor::Green, false, 5.0f);

                    OutLocation = HitResult.ImpactPoint;

                    // ImpactNormal as X axis — vent faces outward automatically
                    OutRotation = FRotationMatrix::MakeFromX(HitResult.ImpactNormal).Rotator();

                    return true;
                }
                else
                {
                    // if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString::Printf(TEXT("[Miss] No tag: %s"), *HitName));
                    DrawDebugBox(GetWorld(), HitResult.ImpactPoint, FVector(5.0f), FColor::Red, false, 5.0f);
                }
            }
        }
    }

    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Yellow, TEXT("All 10 raycasts missed a valid wall — skipping spawn"));
    return false;
}

void AFPSCharacter::SpawnRoomObjects()
{
    FVector SpawnLoc;
    FRotator SpawnRot;

    FActorSpawnParameters SpawnParams;
    // always spawn even if slightly overlapping
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    // 1. spawn the vent (enemy exit)
    if (VentSpawnerClass && TryFindRandomWallSpawnPoint(SpawnLoc, SpawnRot))
    {
        GetWorld()->SpawnActor<AActor>(VentSpawnerClass, SpawnLoc, SpawnRot, SpawnParams);
    }

    // 2. spawn the escape portal
    if (EscapePortalClass && TryFindRandomWallSpawnPoint(SpawnLoc, SpawnRot))
    {
        GetWorld()->SpawnActor<AActor>(EscapePortalClass, SpawnLoc, SpawnRot, SpawnParams);
    }
}
