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

    // 1. empty scene component as the true root
    DefaultRoot = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRoot"));
    RootComponent = DefaultRoot;

    // 2. vent mesh attached to root
    VentMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VentMesh"));
    VentMesh->SetupAttachment(RootComponent);

    // 3. spawn direction arrow (points along X+)
    SpawnDirectionArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("SpawnDirectionArrow"));
    SpawnDirectionArrow->SetupAttachment(RootComponent);
    SpawnDirectionArrow->ArrowColor = FColor::Red;
    SpawnDirectionArrow->ArrowSize = 2.0f;

    // 4. spawn volume box
    SpawnArea = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnArea"));
    SpawnArea->SetupAttachment(RootComponent);
    SpawnArea->SetBoxExtent(FVector(100.f, 100.f, 50.f));

    // offset along X (the vent's forward direction) so the Boss spawns in open air, not inside the wall
    SpawnArea->SetRelativeLocation(FVector(100.f, 0.f, 0.f));
    SpawnArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AVentSpawner::BeginPlay()
{
    Super::BeginPlay();

    // (test only) auto-spawn a Boss 2 seconds after game start — remove when done
    /* FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, this, &AVentSpawner::SpawnBoss, 2.0f, false);
    */
}

void AVentSpawner::SpawnBoss()
{
    // safety check: did the designer actually set BossClassToSpawn in Blueprint?
    if (!BossClassToSpawn)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, TEXT("[Warning] VentSpawner: BossClassToSpawn is not set!"));
        return;
    }

    // pick a random point inside the 3D spawn volume
    FVector RandomSpawnLocation = UKismetMathLibrary::RandomPointInBoundingBox(
        SpawnArea->GetComponentLocation(),
        SpawnArea->GetScaledBoxExtent()
    );

    // match Boss spawn rotation to the Actor (and arrow) rotation
    FRotator SpawnRotation = GetActorRotation();

    // adjust position if slightly overlapping, but always spawn
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ABossAI* SpawnedBoss = GetWorld()->SpawnActor<ABossAI>(
        BossClassToSpawn,
        RandomSpawnLocation,
        SpawnRotation,
        SpawnParams
    );

    if (SpawnedBoss)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("[VentSpawner] Boss ejected successfully!"));
    }
}
