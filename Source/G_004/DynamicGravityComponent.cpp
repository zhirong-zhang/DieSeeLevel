#include "DynamicGravityComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UDynamicGravityComponent::UDynamicGravityComponent()
{
	// enable Tick so we can apply gravity every frame
	PrimaryComponentTick.bCanEverTick = true;
}

void UDynamicGravityComponent::BeginPlay()
{
	Super::BeginPlay();

	// get the root component of the owning Actor (the physics mesh)
	PhysicsComp = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent());

	if (PhysicsComp && PhysicsComp->IsSimulatingPhysics())
	{
		// kill built-in gravity — this component takes over from here
		PhysicsComp->SetEnableGravity(false);
	}

	// cache player reference so we can read gravity direction each tick
	PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
}

void UDynamicGravityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (PhysicsComp && PhysicsComp->IsSimulatingPhysics() && PlayerCharacter)
	{
		FVector CurrentGravityDir = PlayerCharacter->GetCharacterMovement()->GetGravityDirection();

		// detect gravity direction change and wake the body before applying force
		// Chaos puts idle rigid bodies to sleep — sleeping bodies ignore AddForce
		if (!CurrentGravityDir.Equals(LastGravityDir, 0.01f))
		{
			PhysicsComp->WakeRigidBody();
			LastGravityDir = CurrentGravityDir;
		}

		// bAccelChange = true: pass acceleration, not force
		// engine multiplies by mass internally — all props fall at 980 cm/s² regardless of weight
		PhysicsComp->AddForce(CurrentGravityDir * 980.0f, NAME_None, true);
	}
}
