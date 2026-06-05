#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DynamicGravityComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class G_004_API UDynamicGravityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDynamicGravityComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	// cached physics mesh of the owning Actor
	class UPrimitiveComponent* PhysicsComp;

	// cached player reference for reading gravity direction
	class ACharacter* PlayerCharacter;

	FVector LastGravityDir = FVector::ZeroVector;
};
