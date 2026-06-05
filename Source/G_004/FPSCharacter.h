// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FPSCharacter.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UInputAction;
struct FInputActionValue;

UCLASS()
class AFPSCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AFPSCharacter();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Generation")
    TSubclassOf<AActor> VentSpawnerClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Generation")
    TSubclassOf<AActor> EscapePortalClass;

    bool TryFindRandomWallSpawnPoint(FVector& OutLocation, FRotator& OutRotation);
    void SpawnRoomObjects();

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ---------------- Components ----------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    UCameraComponent* FirstPersonCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GravityPlane;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* GravityArrow;

    // ---------------- Input Actions ----------------
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* LookAction;

    // mouse look (Mouse XY 2D-Axis) — separate from gamepad stick so sensitivity can differ
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* MouseLookAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Sensitivity")
    float GamepadLookSensitivity = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Settings")
    float MouseLookSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Settings")
    float GyroGravitySensitivity = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Gravity")
    UInputAction* GyroGravityAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input|Gravity")
    UInputAction* GyroDataAction;

    // ---------------- Core State ----------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    FRotator CurrentGyroRotation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    FQuat InitialPlaneQuat;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    bool bIsShiftingGravity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    float AccumulatedPitch;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|State")
    float AccumulatedRoll;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    bool bIsSmoothRotating;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    FRotator TargetActorRotation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravity|Transition")
    float SmoothRotationSpeed;

    // camera pitch tracked manually — fixes camera lock after a gravity flip
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    float CurrentCameraPitch;

    // ---------------- Internal Helpers ----------------
    virtual void BeginPlay() override;
    FVector SnapVectorToClosestAxis(const FVector& InVector);

    // ---------------- Input Handlers ----------------
    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);       // gamepad right stick
    void MouseLook(const FInputActionValue& Value);  // mouse
    void InputGyroData(const FInputActionValue& Value);
    void GyroGravityStarted();
    void GyroGravityCompleted();
};
