#include "AAProjectileActor.h"
#include "MissleActor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"

AAAProjectileActor::AAAProjectileActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;
    TargetMissile = nullptr;
    TravelledDistance = 0.0f;
    bIsHoming = false;
    
    // Настройка коллизий
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    Mesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
}

void AAAProjectileActor::BeginPlay()
{
    Super::BeginPlay();
    StartLocation = GetActorLocation();
    TravelledDistance = 0.0f;
    bIsHoming = false;
}

void AAAProjectileActor::InitProjectile(AMissleActor* Target, float InForwardDistance, float InSpeed)
{
    TargetMissile = Target;
    ForwardDistance = InForwardDistance;
    Speed = InSpeed;
    bIsHoming = false;
}

void AAAProjectileActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    FVector CurrentLocation = GetActorLocation();
    FVector ForwardVector = GetActorForwardVector();

    if (!bIsHoming)
    {
        FVector NewLocation = CurrentLocation + ForwardVector * Speed * DeltaTime;
        SetActorLocation(NewLocation);
        TravelledDistance += (NewLocation - CurrentLocation).Size();
        if (TravelledDistance >= ForwardDistance)
        {
            bIsHoming = true;
        }
    }
    else if (TargetMissile && TargetMissile->IsValidLowLevel())
    {
        // Улучшенное наведение с предсказанием
        FVector TargetLocation = TargetMissile->GetActorLocation();
        FVector TargetVelocity = FVector::ZeroVector;
        
        // Получаем скорость цели
        UMovementComponent* MovementComp = TargetMissile->FindComponentByClass<UMovementComponent>();
        if (MovementComp)
        {
            TargetVelocity = MovementComp->Velocity;
        }
        
        // Предсказываем позицию цели
        float TimeToTarget = FVector::Dist(CurrentLocation, TargetLocation) / Speed;
        FVector PredictedTargetLocation = TargetLocation + TargetVelocity * TimeToTarget;
        
        // Направление к предсказанной позиции
        FVector ToTarget = (PredictedTargetLocation - CurrentLocation).GetSafeNormal();
        FVector NewLocation = CurrentLocation + ToTarget * Speed * DeltaTime;
        SetActorLocation(NewLocation);
        SetActorRotation(ToTarget.Rotation());
        
        // Проверяем близость к цели
        float DistanceToTarget = FVector::Dist(CurrentLocation, TargetLocation);
        if (DistanceToTarget < 200.0f) // 2 метра
        {
            // Попадание!
            GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("ПВО: Попадание!"));
            
            // Уничтожаем ракету
            if (TargetMissile && TargetMissile->IsValidLowLevel())
            {
                TargetMissile->Destroy();
            }
            
            // Уничтожаем снаряд
            Destroy();
            return;
        }
    }
    else
    {
        // Нет цели — летим прямо
        FVector NewLocation = CurrentLocation + ForwardVector * Speed * DeltaTime;
        SetActorLocation(NewLocation);
    }
    
    // Проверяем столкновения с ракетами
    CheckCollisionWithMissiles();
}

void AAAProjectileActor::CheckCollisionWithMissiles()
{
    FVector CurrentLocation = GetActorLocation();
    
    // Ищем все ракеты в радиусе
    TArray<AActor*> FoundMissiles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMissleActor::StaticClass(), FoundMissiles);
    
    for (AActor* Actor : FoundMissiles)
    {
        if (Actor && Actor->IsValidLowLevel())
        {
            float Distance = FVector::Dist(CurrentLocation, Actor->GetActorLocation());
            if (Distance < 300.0f) // 3 метра
            {
                // Попадание!
                GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, TEXT("ПВО: Попадание по близости!"));
                
                // Уничтожаем ракету
                Actor->Destroy();
                
                // Уничтожаем снаряд
                Destroy();
                return;
            }
        }
    }
} 