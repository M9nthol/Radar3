#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MissleSpawner.generated.h"

UCLASS()
class MEL_API AMissleSpawner : public AActor
{
    GENERATED_BODY()

public:
    AMissleSpawner();

protected:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    void SpawnMissle();

    FVector GetRandomEdgePosition(float Distance);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
    TSubclassOf<class AMissleActor> MissleClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Missile")
    int32 MissleCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    float MapHalfSize = 30000.f;
};
