// MissileGameMode.cpp

#include "MissleGameMode.h"
#include "MissleActor.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void AMissleGameMode::BeginPlay()
{
    Super::BeginPlay();

    // Спавним несколько ракет
    for (int i = 0; i < 10; ++i)
    {
        SpawnMissle();
    }
}

void AMissleGameMode::SpawnMissle()
{
    if (!MissleClass) return;

    FVector SpawnLocation = GetRandomEdgePosition(MapHalfSize);
    FActorSpawnParameters Params;
    GetWorld()->SpawnActor<AMissleActor>(MissleClass, SpawnLocation, FRotator::ZeroRotator, Params);
}

FVector AMissleGameMode::GetRandomEdgePosition(float Distance)
{
    int32 Edge = FMath::RandRange(0, 3);
    float X = 0.f, Y = 0.f;
    switch (Edge)
    {
    case 0: X = Distance; Y = FMath::RandRange(-Distance, Distance); break;  // Right
    case 1: X = -Distance; Y = FMath::RandRange(-Distance, Distance); break; // Left
    case 2: X = FMath::RandRange(-Distance, Distance); Y = Distance; break;  // Top
    case 3: X = FMath::RandRange(-Distance, Distance); Y = -Distance; break; // Bottom
    }

    return FVector(X, Y, 100.f); // Z фиксируем
}

