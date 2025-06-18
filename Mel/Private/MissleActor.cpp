#include "MissleActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "DrawDebugHelpers.h"

AMissleActor::AMissleActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    // Создаём компонент для звука взлёта
    LaunchSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("LaunchSound"));
    LaunchSoundComponent->SetupAttachment(RootComponent);
    LaunchSoundComponent->bAutoActivate = false;
    LaunchSoundComponent->AttenuationSettings = nullptr;
    LaunchSoundComponent->VolumeMultiplier = 2.0f;

    MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
    MovementComponent->SetUpdatedComponent(RootComponent);
    MovementComponent->InitialSpeed = Speed;
    MovementComponent->MaxSpeed = Speed;
    MovementComponent->bRotationFollowsVelocity = true;
    MovementComponent->bShouldBounce = false;
    MovementComponent->ProjectileGravityScale = 0.0f;

    // Устанавливаем начальную ориентацию меша (ракета направлена строго вверх)
    Mesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
}

void AMissleActor::BeginPlay()
{
    Super::BeginPlay();
    Phase = EMisslePhase::Ascending;
    CurrentTransitionTime = 0.0f;
    // Устанавливаем начальное направление строго вверх
    InitialDirection = FVector(0.0f, 0.0f, 1.0f);
    TargetPoint = FVector(0.0f, 0.0f, 0.0f);
    CurrentVelocity = InitialDirection * Speed;
    
    if (MovementComponent)
    {
        MovementComponent->Velocity = CurrentVelocity;
    }

    // Воспроизводим звук взлёта
    if (LaunchSound && LaunchSoundComponent)
    {
        LaunchSoundComponent->SetSound(LaunchSound);
        LaunchSoundComponent->Play();
        
        LaunchSoundComponent->SetFloatParameter(FName("Distance"), 10000.0f);
        LaunchSoundComponent->SetFloatParameter(FName("Volume"), 2.0f);
    }
}

void AMissleActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    switch (Phase)
    {
        case EMisslePhase::Ascending:
            CurrentVelocity = FVector(0.0f, 0.0f, Speed);
            if (GetActorLocation().Z >= TargetHeight)
            {
                Phase = EMisslePhase::Transition;
                CurrentTransitionTime = 0.0f;
                // Calculate direction to target when starting transition
                FVector ToTarget = (TargetPoint - GetActorLocation()).GetSafeNormal();
                TargetDirection = ToTarget;
            }
            break;

        case EMisslePhase::Transition:
            CurrentTransitionTime += DeltaTime;
            if (CurrentTransitionTime >= TransitionTime)
            {
                Phase = EMisslePhase::Horizontal;
                // Сохраняем точки начала и конца горизонтального полета
                HorizontalStartPoint = GetActorLocation();
                // Вычисляем точку окончания горизонтального полета
                FVector DirectionToTarget = (TargetPoint - HorizontalStartPoint).GetSafeNormal();
                HorizontalEndPoint = HorizontalStartPoint + DirectionToTarget * HorizontalDistance;
                HorizontalEndPoint.Z = HorizontalHeight; // Устанавливаем высоту горизонтального полета
            }
            else
            {
                float Alpha = FMath::SmoothStep(0.0f, 1.0f, CurrentTransitionTime / TransitionTime);
                // Smoothly interpolate from vertical to horizontal direction
                FVector CurrentDirection = FMath::Lerp(FVector(0.0f, 0.0f, 1.0f), TargetDirection, Alpha);
                CurrentVelocity = CurrentDirection * Speed;
            }
            break;

        case EMisslePhase::Horizontal:
            // Летим горизонтально к точке HorizontalEndPoint
            FVector ToHorizontalEnd = (HorizontalEndPoint - GetActorLocation()).GetSafeNormal();
            CurrentVelocity = ToHorizontalEnd * Speed;
            
            // Проверяем, достигли ли мы точки окончания горизонтального полета
            if (FVector::Dist(GetActorLocation(), HorizontalEndPoint) < 100.0f)
            {
                Phase = EMisslePhase::Descent;
            }
            break;

        case EMisslePhase::Descent:
            // Calculate direction to target
            FVector ToTarget = (TargetPoint - GetActorLocation()).GetSafeNormal();
            CurrentVelocity = ToTarget * Speed;
            break;
    }

    // Обновляем скорость в компоненте движения
    if (MovementComponent)
    {
        MovementComponent->Velocity = CurrentVelocity;
    }

    // Обновляем позицию
    FVector NewLocation = GetActorLocation() + CurrentVelocity * DeltaTime;
    SetActorLocation(NewLocation);

    // Обновляем вращение
    UpdateRotation(DeltaTime);

    // Проверяем столкновение
    if (CheckTargetCollision())
    {
        Explode();
    }
}

void AMissleActor::UpdateRotation(float DeltaTime)
{
    FRotator TargetRotation;
    
    if (Phase == EMisslePhase::Ascending)
    {
        // При взлете ракета направлена основанием вниз
        TargetRotation = FRotator(-90.0f, 0.0f, 0.0f);
    }
    else
    {
        // При полете к цели ракета должна быть направлена носом вперед
        // Для этого добавляем 180 градусов к повороту, чтобы развернуть ракету
        TargetRotation = CurrentVelocity.Rotation() + FRotator(180.0f, 0.0f, 0.0f);
    }
    
    FRotator CurrentRotation = GetActorRotation();
    FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, RotationSpeed);
    SetActorRotation(NewRotation);
}

bool AMissleActor::CheckTargetCollision()
{
    if (Phase != EMisslePhase::Descent)
        return false;

    FHitResult HitResult;
    FVector Start = GetActorLocation();
    FVector End = Start + GetActorForwardVector() * 100.0f;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        return true;
    }

    return false;
}

void AMissleActor::Explode()
{
    if (ExplosionEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());
    }

    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, GetActorLocation());
    }

    // Наносим урон в радиусе взрыва
    TArray<FHitResult> HitResults;
    FCollisionShape Sphere = FCollisionShape::MakeSphere(ExplosionRadius);
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (GetWorld()->SweepMultiByChannel(HitResults, GetActorLocation(), GetActorLocation(), 
        FQuat::Identity, ECC_Visibility, Sphere, QueryParams))
    {
        for (const FHitResult& Hit : HitResults)
        {
            if (Hit.GetActor() && Hit.GetActor() != this)
            {
                // Здесь можно добавить логику нанесения урона
                // Например: Hit.GetActor()->TakeDamage(DamageAmount, FDamageEvent(), nullptr, this);
            }
        }
    }

    Destroy();
}

