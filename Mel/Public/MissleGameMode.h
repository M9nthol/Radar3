// MissileGameMode.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MissleGameMode.generated.h"

UCLASS(Blueprintable)
class MEL_API AMissleGameMode : public AGameModeBase
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;

public:
    void SpawnMissle();
    FVector GetRandomEdgePosition(float DistanceFromCenter);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Missle")
    TSubclassOf<class AMissleActor> MissleClass;

    float MapHalfSize = 5000.f; // Половина карты
};
