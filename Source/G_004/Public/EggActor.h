// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EggActor.generated.h"

UCLASS()
class G_004_API AEggActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEggActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<APawn> MainEnemy;
	
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UAnimationAsset* HatchAnimation;
	
	
	UFUNCTION()
	void SpawnEnemy();
	
	
	bool bHasHatched = false;
	
	FTimerHandle TimerHandle;
	
		
		
	

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;
	
	UFUNCTION(BlueprintCallable)
	void HatchEgg();
	
	UFUNCTION(BlueprintImplementableEvent)
	void PlayHatchAnimation();
	

};
