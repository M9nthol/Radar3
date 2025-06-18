#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AAActor.generated.h"

class ARadarActor;
class AMissleActor;
class AAAProjectileActor;

UCLASS()
class MEL_API AAAActor : public AActor
{
    GENERATED_BODY()

public:
    AAAActor();
    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // Установить ссылку на радар
    void SetRadar(ARadarActor* Radar);

protected:
    UPROPERTY(EditAnywhere, Category = "AA Settings")
    float FireInterval = 2.0f;

    UPROPERTY(EditAnywhere, Category = "AA Settings")
    float ProjectileSpeed = 3000.0f;

    UPROPERTY(EditAnywhere, Category = "AA Settings")
    TSubclassOf<AAAProjectileActor> ProjectileClass;

    UPROPERTY(EditAnywhere, Category = "AA Settings")
    float InitialForwardDistance = 2000.0f; // 20 метров (в Unreal 1 ед. = 1 см)

    UPROPERTY(EditAnywhere, Category = "AA Settings")
    float DetectionRadius = 20000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

private:
    ARadarActor* RadarRef;
    float TimeSinceLastFire;

    void TryFireAtMissile();
    AMissleActor* FindTargetMissile();
}; 