#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VentSpawner.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UArrowComponent;
class USceneComponent;
class ABossAI;

UCLASS()
class G_004_API AVentSpawner : public AActor
{
	GENERATED_BODY()

public:
	AVentSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// 1. empty scene component as the true root — decouples mesh orientation from Actor rotation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultRoot;

	// 2. vent mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* VentMesh;

	// 3. arrow indicating the spawn direction (X axis forward)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* SpawnDirectionArrow;

	// 4. box defining the random spawn volume for the Boss
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SpawnArea;

	// Boss class to spawn (set in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<ABossAI> BossClassToSpawn;

	// spawns the Boss inside the SpawnArea box
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnBoss();
};
