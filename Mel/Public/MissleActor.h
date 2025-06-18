#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "MissleActor.generated.h"

UENUM(BlueprintType)
enum class EMisslePhase : uint8
{
    Ascending,    // Вертикальный подъем
    Transition,   // Плавный переход к цели
    Horizontal,   // Горизонтальный полет на заданной высоте
    Descent       // Плавное снижение к цели
};

UCLASS()
class MEL_API AMissleActor : public AActor
{
    GENERATED_BODY()

public:
    AMissleActor();

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UAudioComponent* LaunchSoundComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UProjectileMovementComponent* MovementComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    UParticleSystem* ExplosionEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    USoundBase* ExplosionSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
    USoundBase* LaunchSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float TargetHeight = 20000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float HorizontalHeight = 17000.f; // Высота горизонтального полета (TargetHeight - 3000)

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float HorizontalDistance = 5000.f; // Расстояние горизонтального полета

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float Speed = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float RotationSpeed = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float TransitionTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float Gravity = 1500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float ExplosionRadius = 3000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flight")
    float MinTargetDistance = 5000.f;

private:
    void UpdateRotation(float DeltaTime);
    bool CheckTargetCollision();
    void Explode();

    EMisslePhase Phase;
    FVector InitialDirection;
    FVector TargetDirection;
    FVector TargetPoint;
    FVector CurrentVelocity;
    float CurrentTransitionTime;
    FVector HorizontalStartPoint; // Точка начала горизонтального полета
    FVector HorizontalEndPoint;   // Точка окончания горизонтального полета
};
