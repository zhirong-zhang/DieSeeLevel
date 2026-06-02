// Fill out your copyright notice in the Description page of Project Settings.


#include "EggActor.h"


// Sets default values
AEggActor::AEggActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEggActor::BeginPlay()
{
	Super::BeginPlay();
	
}


void AEggActor::HatchEgg()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Start hatching"));
	
	if (bHasHatched)
	{
		return;
	}
	
	bHasHatched = true;
	
	PlayHatchAnimation();
	
	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&AEggActor::SpawnEnemy,
		1.5f,
		false);
	
}

void AEggActor::SpawnEnemy()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Main Enemy Spawned"));
	
	if (MainEnemy)
	{
		GetWorld()->SpawnActor<AActor>(
			MainEnemy,
			GetActorLocation(),
			GetActorRotation());
	}
	
	Destroy();
}

