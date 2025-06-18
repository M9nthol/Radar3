#include "MissleSpawner.h"
#include "MissleActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AMissleSpawner::AMissleSpawner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AMissleSpawner::BeginPlay()
{
    Super::BeginPlay();

    for (int i = 0; i < MissleCount; ++i)
    {
        SpawnMissle();
    }
}

void AMissleSpawner::SpawnMissle()
{
    if (!MissleClass) return;

    FVector SpawnLocation = GetRandomEdgePosition(MapHalfSize);
    GetWorld()->SpawnActor<AMissleActor>(MissleClass, SpawnLocation, FRotator::ZeroRotator);
}

FVector AMissleSpawner::GetRandomEdgePosition(float Distance)
{
    int32 Edge = FMath::RandRange(0, 3);
    float X = 0.f, Y = 0.f;
    switch (Edge)
    {
    case 0: X = Distance; Y = FMath::RandRange(-Distance, Distance); break;
    case 1: X = -Distance; Y = FMath::RandRange(-Distance, Distance); break;
    case 2: Y = Distance; X = FMath::RandRange(-Distance, Distance); break;
    case 3: Y = -Distance; X = FMath::RandRange(-Distance, Distance); break;
    }

    return FVector(X, Y, 100.f);
}

