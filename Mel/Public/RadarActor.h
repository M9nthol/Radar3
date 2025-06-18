#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "GameFramework/MovementComponent.h"
#include "RadarActor.generated.h"

class AMissleActor;

// Структура для хранения информации о ракете
USTRUCT(BlueprintType)
struct FMissileData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    AActor* Missile;

    UPROPERTY(BlueprintReadWrite)
    FVector Position;

    UPROPERTY(BlueprintReadWrite)
    FVector Velocity;

    UPROPERTY(BlueprintReadWrite)
    FVector PredictedPosition;

    UPROPERTY(BlueprintReadWrite)
    float Distance;

    UPROPERTY(BlueprintReadWrite)
    float ThreatLevel;

    UPROPERTY(BlueprintReadWrite)
    float LastDetectionTime;

    UPROPERTY(BlueprintReadWrite)
    int32 DetectionCount;

    UPROPERTY(BlueprintReadWrite)
    bool bReportedTrajectory;

    FMissileData()
    {
        Missile = nullptr;
        Position = FVector::ZeroVector;
        Velocity = FVector::ZeroVector;
        PredictedPosition = FVector::ZeroVector;
        Distance = 0.0f;
        ThreatLevel = 0.0f;
        LastDetectionTime = 0.0f;
        DetectionCount = 0;
        bReportedTrajectory = false;
    }
};

UCLASS()
class MEL_API ARadarActor : public AActor
{
    GENERATED_BODY()

public:
    ARadarActor();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

    // Публичный аксессор для ПВО
    const TArray<FMissileData>& GetDetectedMissiles() const { return DetectedMissiles; }

    // Получить ракеты, обнаруженные 3 раза
    TArray<AMissleActor*> GetMissilesDetectedThreeTimes() const;

protected:
    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ScanRadius = 25000.0f;

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ScanSpeed = 300.0f; // Увеличено до 360 градусов/сек для быстрого сканирования

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ScanInterval = 0.05f; // Уменьшено до 20мс для более частого сканирования

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ScanSectorWidth = 20.0f; // Увеличено до 15 градусов для лучшего покрытия

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float MinDetectionHeight = 1000.0f; // Минимальная высота для обнаружения

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float MaxDetectionHeight = 25000.0f; // Максимальная высота для обнаружения

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float PredictionTime = 2.0f; // Время для предсказания траектории

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ThreatDistanceWeight = 0.4f; // Вес расстояния в расчете угрозы

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ThreatSpeedWeight = 0.3f; // Вес скорости в расчете угрозы

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    float ThreatHeightWeight = 0.3f; // Вес высоты в расчете угрозы

    UPROPERTY(EditAnywhere, Category = "Radar Settings")
    USoundBase* PingSound;

    UPROPERTY()
    UAudioComponent* AudioComponent;

private:
    float CurrentScanAngle;
    float TimeSinceLastScan;
    TArray<FMissileData> DetectedMissiles;
    TMap<AActor*, float> MissileLastDetectionTimes;

    void PerformScan();
    void CalculateImpactPoint(const FMissileData& MissileData);
    bool IsMissileInScanSector(const FVector& MissileLocation);
    void PlayPingSound();
    void UpdateMissileData(AActor* Missile);
    void PredictMissileTrajectory(FMissileData& MissileData);
    float CalculateThreatLevel(const FMissileData& MissileData);
    void CleanupOldDetections();
    void SortMissilesByThreat();
    bool IsMissileInHeightRange(const FVector& MissileLocation);
    float CalculateTimeToImpact(const FMissileData& MissileData);
}; 