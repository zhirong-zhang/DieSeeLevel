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
	// 1. 空场景组件作为真正的根节点，解绑模型方向限制
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* DefaultRoot;

	// 2. 通风口模型
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* VentMesh;

	// 3. 生成方向指示箭头
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UArrowComponent* SpawnDirectionArrow;

	// 4. 怪物生成的随机范围框
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* SpawnArea;

	// 要生成的 Boss 类（需在蓝图中配置）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawning")
	TSubclassOf<ABossAI> BossClassToSpawn;

	// 生成 Boss 的函数
	UFUNCTION(BlueprintCallable, Category = "Spawning")
	void SpawnBoss();
};