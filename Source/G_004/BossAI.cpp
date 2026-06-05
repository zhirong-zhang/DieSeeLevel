// Fill out your copyright notice in the Description page of Project Settings.

#include "BossAI.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

ABossAI::ABossAI()
{
    PrimaryActorTick.bCanEverTick = true;

    // disable Controller rotation coupling — we drive orientation manually with quaternions
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // disable CMC auto-rotation — it misbehaves in multi-gravity environments
    GetCharacterMovement()->bOrientRotationToMovement = false;

    // auto-bind AI Controller whether placed in level or dynamically spawned
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // after a gravity switch the Boss may be mid-air — AirControl=1.0 keeps it responsive
    GetCharacterMovement()->AirControl = 1.0f;
}

void ABossAI::BeginPlay()
{
    Super::BeginPlay();

    // auto-grab the player as target (index 0 = local player)
    TargetPlayer = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
}

void ABossAI::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetController() == nullptr)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(1, 0.1f, FColor::Red, TEXT("[Warning] Boss has no Controller!"));
    }

    if (!TargetPlayer) return;

    // read the player's CMC for the actual current gravity
    UCharacterMovementComponent* PlayerCMC = TargetPlayer->FindComponentByClass<UCharacterMovementComponent>();
    if (!PlayerCMC) return;

    // read from the player's CMC — it snaps to the final gravity on alignment,
    // avoiding the wobble you get from reading UpVector during a rotation transition
    FVector TargetGravity = PlayerCMC->GetGravityDirection();
    FVector TargetUpVector = -TargetGravity; // gravity's opposite = true up

    FVector BossCurrentGravity = GetCharacterMovement()->GetGravityDirection();

    // gravity mismatch means the player just switched — sync immediately
    if (!BossCurrentGravity.Equals(TargetGravity, 0.01f))
    {
        GetCharacterMovement()->SetGravityDirection(TargetGravity);

        // anti-explosion ①: break off old floor surface snapping
        GetCharacterMovement()->SetMovementMode(MOVE_Falling);

        // anti-explosion ②: nudge capsule upward so it doesn't clip into the new floor
        AddActorWorldOffset(GetActorUpVector() * 60.0f, false);
    }

    // check how far the Boss's up vector is from the target — still flipping?
    float AngleDiff = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(GetActorUpVector(), TargetUpVector)));
    bool bIsAligningGravity = (AngleDiff > 2.0f);

    // build target orientation: Z = new up, X = toward player
    FVector ToPlayer = TargetPlayer->GetActorLocation() - GetActorLocation();
    FVector MoveDirection = FVector::VectorPlaneProject(ToPlayer, TargetUpVector).GetSafeNormal();

    if (!MoveDirection.IsNearlyZero())
    {
        FQuat TargetQuat = FRotationMatrix::MakeFromZX(TargetUpVector, MoveDirection).ToQuat();

        FQuat NewQuat = FMath::QInterpTo(GetActorQuat(), TargetQuat, DeltaTime, RotationSpeed);
        SetActorRotation(NewQuat);
    }

    // don't move until orientation is settled — prevents chasing sideways mid-flip
    if (!bIsAligningGravity)
    {
        float DistanceToPlayer = ToPlayer.Length();
        if (DistanceToPlayer > StopDistance)
        {
            AddMovementInput(MoveDirection, 1.0f);
        }
    }
}
