#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AAProjectileActor.generated.h"

class AMissleActor;

UCLASS()
class MEL_API AAAProjectileActor : public AActor
{
    GENERATED_BODY()

public:
    AAAProjectileActor();
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // Инициализация снаряда
    void InitProjectile(AMissleActor* Target, float ForwardDistance, float Speed);

protected:
    UPROPERTY(EditAnywhere, Category = "Projectile")
    float Speed = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float ForwardDistance = 2000.0f; // 20 метров

    UPROPERTY(EditAnywhere, Category = "Projectile")
    float HomingAcceleration = 8000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

private:
    AMissleActor* TargetMissile;
    FVector StartLocation;
    float TravelledDistance;
    bool bIsHoming;
    
    void CheckCollisionWithMissiles();
}; 