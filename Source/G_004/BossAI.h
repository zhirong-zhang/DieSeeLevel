// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BossAI.generated.h"

UCLASS()
class G_004_API ABossAI : public ACharacter
{
	GENERATED_BODY()

public:
	ABossAI();
	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

public:
	// --- AI properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float StopDistance = 150.0f;  // how close to the player before stopping

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float RotationSpeed = 8.0f;   // smoothing speed for body reorientation

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float FloorTraceDistance = 500.0f; // floor detection ray length

private:
	// cached reference to the target player
	AActor* TargetPlayer;
};
