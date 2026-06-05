// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "G_004Character.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
class UArrowComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(abstract)
class AG_004Character : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

protected:

    // ---------------- Base Input ----------------
    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* JumpAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MoveAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* LookAction;

    UPROPERTY(EditAnywhere, Category="Input")
    UInputAction* MouseLookAction;

    // ---------------- Gyro and Gravity Input ----------------
    UPROPERTY(EditAnywhere, Category="Input|Gyro")
    UInputAction* GyroGravityAction;

    UPROPERTY(EditAnywhere, Category="Input|Gyro")
    UInputAction* GyroDataAction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gyro")
    FRotator CurrentGyroRotation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Gyro")
    FRotator StartGyroRotation;

    // ---------------- Gravity Arrow ----------------
    /** arrow showing the current/predicted gravity direction */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
    UArrowComponent* GravityArrow;

    /** true while the player is holding the gravity aim button */
    bool bIsAimingGravity;

public:
    AG_004Character();

    // enable Tick to update arrow direction each frame
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);

    void InputGyroData(const FInputActionValue& Value);
    void GyroGravityStarted();
    void GyroGravityCompleted();

public:

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};
