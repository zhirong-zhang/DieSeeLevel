#include "VentSpawner.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ArrowComponent.h" 
#include "Components/SceneComponent.h" 
#include "Kismet/KismetMathLibrary.h"
#include "BossAI.h" 

AVentSpawner::AVentSpawner()
{
    PrimaryActorTick.bCanEverTick = false;

    // 1. 创建空的场景组件，并将其设为真正的根组件
    DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
    RootComponent = DefaultRoot;

    // 2. 创建模型组件，挂载到根组件下
    VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
    VentMesh->SetupAttachment(RootComponent);

    // 3. 创建箭头组件指示生成方向 (X轴正方向)，挂载到根组件下
    SpawnDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirectionArrow"));
    SpawnDirectionArrow->SetupAttachment(RootComponent);
    SpawnDirectionArrow->ArrowColor = FColor::Red;
    SpawnDirectionArrow->ArrowSize = 2.0f;

    // 4. 创建生成范围框，挂载到根组件下
    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnArea->SetupAttachment(RootComponent);
    SpawnArea->SetBoxExtent(FVector(100.f, 100.f, 50.f));
    
    // 【关键】将生成框沿 X 轴（即红色箭头指向的正前方）推出去，防止怪物生成在墙体内
    SpawnArea->SetRelativeLocation(FVector(100.f, 0.f, 0.f)); 
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AVentSpawner::BeginPlay()
{
    Super::BeginPlay();
    
    // (测试用) 游戏开始时延迟 2 秒自动生成一个 Boss，测试完后可以删掉这行
    /* FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AVentSpawner::SpawnBoss, 2.0f, false);
    */
}

void AVentSpawner::SpawnBoss()
{
    // 安全检查：策划到底在蓝图里配置 Boss 类了没有？
    if (!BossClassToSpawn)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("【警告】VentSpawner 忘记配置 BossClassToSpawn 了！"));
        return;
    }

    // 在 Box 组件的 3D 范围内，随机取一个点作为生成坐标
    FVector RandomSpawnLocation = UKismetMathLibrary::RandomPointInBoundingBox(
        SpawnArea->GetComponentLocation(), 
        SpawnArea->GetScaledBoxExtent()
    );

    // 保持怪物出生时的旋转角度和 Actor（以及箭头）一致
    FRotator SpawnRotation = GetActorRotation();

    // 配置生成参数：如果位置有点重叠，让引擎尽量挪一挪，但必须生成出来
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 正式生成！
    ABossAI* SpawnedBoss = GetWorld()->SpawnActor<ABossAI>(
        BossClassToSpawn, 
        RandomSpawnLocation, 
        SpawnRotation, 
        SpawnParams
    );

    if (SpawnedBoss)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("【生成器】已成功喷出一个 Boss！"));
    }
}