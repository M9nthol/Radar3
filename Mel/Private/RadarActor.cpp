#include "RadarActor.h"
#include "MissleActor.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"

ARadarActor::ARadarActor()
{
    PrimaryActorTick.bCanEverTick = true;
    
    AudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("AudioComponent"));
    RootComponent = AudioComponent;
}

void ARadarActor::BeginPlay()
{
    Super::BeginPlay();
    CurrentScanAngle = 0.0f;
    TimeSinceLastScan = 0.0f;
}

void ARadarActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Update scan angle
    CurrentScanAngle += ScanSpeed * DeltaTime;
    if (CurrentScanAngle >= 360.0f)
    {
        CurrentScanAngle -= 360.0f;
    }

    // Perform scan at intervals
    TimeSinceLastScan += DeltaTime;
    if (TimeSinceLastScan >= ScanInterval)
    {
        PerformScan();
        TimeSinceLastScan = 0.0f;
    }

    // Cleanup old detections
    CleanupOldDetections();

    // Draw debug visualization of radar sweep
    FVector Start = GetActorLocation();
    FVector End = Start + FVector(FMath::Cos(FMath::DegreesToRadians(CurrentScanAngle)), 
                                FMath::Sin(FMath::DegreesToRadians(CurrentScanAngle)), 0.0f) * ScanRadius;
    DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1.0f, 0, 2.0f);

    // Draw scan sector
    float HalfSector = ScanSectorWidth * 0.5f;
    FVector SectorStart1 = Start + FVector(FMath::Cos(FMath::DegreesToRadians(CurrentScanAngle - HalfSector)), 
                                         FMath::Sin(FMath::DegreesToRadians(CurrentScanAngle - HalfSector)), 0.0f) * ScanRadius;
    FVector SectorStart2 = Start + FVector(FMath::Cos(FMath::DegreesToRadians(CurrentScanAngle + HalfSector)), 
                                         FMath::Sin(FMath::DegreesToRadians(CurrentScanAngle + HalfSector)), 0.0f) * ScanRadius;
    DrawDebugLine(GetWorld(), Start, SectorStart1, FColor::Yellow, false, -1.0f, 0, 1.0f);
    DrawDebugLine(GetWorld(), Start, SectorStart2, FColor::Yellow, false, -1.0f, 0, 1.0f);
}

void ARadarActor::PerformScan()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMissleActor::StaticClass(), FoundActors);

    for (AActor* Actor : FoundActors)
    {
        if (!Actor || !Actor->IsValidLowLevel())
            continue;

        FVector MissileLocation = Actor->GetActorLocation();
        
        // Проверяем высоту ракеты
        if (!IsMissileInHeightRange(MissileLocation))
            continue;

        // Проверяем, находится ли ракета в секторе сканирования
        if (IsMissileInScanSector(MissileLocation))
        {
            UpdateMissileData(Actor);
        }
    }

    // Сортируем ракеты по уровню угрозы
    SortMissilesByThreat();

    // Выводим информацию о наиболее опасных ракетах
    for (int32 i = 0; i < FMath::Min(DetectedMissiles.Num(), 3); i++)
    {
        const FMissileData& MissileData = DetectedMissiles[i];
        if (MissileData.Missile && MissileData.Missile->IsValidLowLevel())
        {
            FString Message = FString::Printf(TEXT("РАДАР #%d: Ракета обнаружена! Угроза: %.2f | Координаты: X=%.0f, Y=%.0f, Z=%.0f | Скорость: %.0f м/с"),
                i + 1,
                MissileData.ThreatLevel,
                MissileData.Position.X,
                MissileData.Position.Y,
                MissileData.Position.Z,
                MissileData.Velocity.Size());
            
            FColor MessageColor = (i == 0) ? FColor::Red : (i == 1) ? FColor::Orange : FColor::Yellow;
            GEngine->AddOnScreenDebugMessage(-1, 0.1f, MessageColor, Message);
        }
    }
}

bool ARadarActor::IsMissileInScanSector(const FVector& MissileLocation)
{
    FVector DirectionToMissile = MissileLocation - GetActorLocation();
    DirectionToMissile.Z = 0.0f;
    float Distance = DirectionToMissile.Size();
    
    if (Distance > ScanRadius)
        return false;

    DirectionToMissile.Normalize();

    float MissileAngle = FMath::RadiansToDegrees(FMath::Atan2(DirectionToMissile.Y, DirectionToMissile.X));
    if (MissileAngle < 0.0f)
    {
        MissileAngle += 360.0f;
    }

    float AngleDifference = FMath::Abs(MissileAngle - CurrentScanAngle);
    if (AngleDifference > 180.0f)
    {
        AngleDifference = 360.0f - AngleDifference;
    }

    return AngleDifference <= ScanSectorWidth * 0.5f;
}

bool ARadarActor::IsMissileInHeightRange(const FVector& MissileLocation)
{
    return MissileLocation.Z >= MinDetectionHeight && MissileLocation.Z <= MaxDetectionHeight;
}

void ARadarActor::UpdateMissileData(AActor* Missile)
{
    if (!Missile || !Missile->IsValidLowLevel())
        return;

    FVector CurrentPosition = Missile->GetActorLocation();
    FVector CurrentVelocity = FVector::ZeroVector;

    UMovementComponent* MovementComp = Missile->FindComponentByClass<UMovementComponent>();
    if (MovementComp)
    {
        CurrentVelocity = MovementComp->Velocity;
    }

    int32 ExistingIndex = -1;
    for (int32 i = 0; i < DetectedMissiles.Num(); i++)
    {
        if (DetectedMissiles[i].Missile == Missile)
        {
            ExistingIndex = i;
            break;
        }
    }

    if (ExistingIndex >= 0)
    {
        FMissileData& MissileData = DetectedMissiles[ExistingIndex];
        MissileData.Position = CurrentPosition;
        MissileData.Velocity = CurrentVelocity;
        MissileData.Distance = (CurrentPosition - GetActorLocation()).Size();
        MissileData.LastDetectionTime = GetWorld()->GetTimeSeconds();
        if (MissileData.bReportedTrajectory) {
            // После 4 сообщений больше ничего не выводим
            return;
        }
        if (MissileData.DetectionCount < 4) {
            MissileData.DetectionCount++;
        }
        PredictMissileTrajectory(MissileData);
        MissileData.ThreatLevel = CalculateThreatLevel(MissileData);

        int32 RocketNumber = ExistingIndex + 1;
        if (MissileData.DetectionCount == 1) {
            FString Message = FString::Printf(TEXT("Ракета #%d обнаружена! Координаты: X=%.0f, Y=%.0f, Z=%.0f"),
                RocketNumber, CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message);
            PlayPingSound();
        } else if (MissileData.DetectionCount == 2) {
            FString Message = FString::Printf(TEXT("Ракета #%d обнаружена второй раз! Координаты: X=%.0f, Y=%.0f, Z=%.0f"),
                RocketNumber, CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message);
            PlayPingSound();
        } else if (MissileData.DetectionCount == 3) {
            FString Message = FString::Printf(TEXT("Ракета #%d обнаружена третий раз! Координаты: X=%.0f, Y=%.0f, Z=%.0f"),
                RocketNumber, CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);
            GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message);
            PlayPingSound();
        } else if (MissileData.DetectionCount == 4) {
            float TimeToGround = CalculateTimeToImpact(MissileData);
            FVector ImpactPoint = CurrentPosition + CurrentVelocity * TimeToGround;
            FString TrajectoryMessage = FString::Printf(TEXT("Траектория ракеты #%d:\nСкорость: X=%.2f, Y=%.2f, Z=%.2f\nВремя до падения: %.2f сек\nТочка падения: X=%.0f, Y=%.0f, Z=%.0f"),
                RocketNumber, CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z, TimeToGround, ImpactPoint.X, ImpactPoint.Y, ImpactPoint.Z);
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TrajectoryMessage);
            PlayPingSound();
            MissileData.bReportedTrajectory = true; // После этого больше не выводим сообщений
        }
    }
    else
    {
        FMissileData NewMissileData;
        NewMissileData.Missile = Missile;
        NewMissileData.Position = CurrentPosition;
        NewMissileData.Velocity = CurrentVelocity;
        NewMissileData.Distance = (CurrentPosition - GetActorLocation()).Size();
        NewMissileData.LastDetectionTime = GetWorld()->GetTimeSeconds();
        NewMissileData.DetectionCount = 1;
        NewMissileData.bReportedTrajectory = false;
        PredictMissileTrajectory(NewMissileData);
        NewMissileData.ThreatLevel = CalculateThreatLevel(NewMissileData);
        DetectedMissiles.Add(NewMissileData);
        int32 RocketNumber = DetectedMissiles.Num();
        FString Message = FString::Printf(TEXT("Ракета #%d обнаружена! Координаты: X=%.0f, Y=%.0f, Z=%.0f"),
            RocketNumber, CurrentPosition.X, CurrentPosition.Y, CurrentPosition.Z);
        GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Yellow, Message);
        PlayPingSound();
    }
}

void ARadarActor::PredictMissileTrajectory(FMissileData& MissileData)
{
    if (MissileData.Velocity.SizeSquared() < 1.0f)
        return;

    // Простое предсказание: ракета продолжит движение с текущей скоростью
    MissileData.PredictedPosition = MissileData.Position + MissileData.Velocity * PredictionTime;
    
    // Если ракета движется вниз, предсказываем точку падения
    if (MissileData.Velocity.Z < -100.0f) // Значительная скорость вниз
    {
        float TimeToGround = -MissileData.Position.Z / MissileData.Velocity.Z;
        if (TimeToGround > 0.0f && TimeToGround < PredictionTime)
        {
            MissileData.PredictedPosition = MissileData.Position + MissileData.Velocity * TimeToGround;
            MissileData.PredictedPosition.Z = 0.0f; // Устанавливаем на уровень земли
        }
    }
}

float ARadarActor::CalculateTimeToImpact(const FMissileData& MissileData)
{
    FVector Position = MissileData.Position;
    FVector Velocity = MissileData.Velocity;
    
    // Если ракета уже на земле или ниже
    if (Position.Z <= 0.0f)
        return 0.0f;
    
    // Если ракета движется вверх или горизонтально, нужно дождаться фазы снижения
    if (Velocity.Z >= 0.0f)
    {
        // Ракета еще не начала снижаться
        // Оцениваем время до начала снижения на основе текущей высоты и скорости
        float TimeToPeak = Velocity.Z / 1500.0f; // Примерное время до пика (можно уточнить)
        float TimeToDescent = TimeToPeak + 2.0f; // Дополнительное время на горизонтальный полет
        
        // Если ракета на большой высоте, добавляем время горизонтального полета
        if (Position.Z > 15000.0f)
        {
            TimeToDescent += 5.0f; // Дополнительное время для горизонтального полета
        }
        
        return TimeToDescent;
    }
    else
    {
        // Ракета уже снижается - используем более точный расчет
        float Gravity = 1500.0f; // Ускорение свободного падения (можно уточнить)
        float DragCoefficient = 0.1f; // Коэффициент сопротивления воздуха
        
        // Учитываем сопротивление воздуха
        float EffectiveGravity = Gravity * (1.0f + DragCoefficient * Velocity.Size() / 1000.0f);
        
        // Используем квадратичную формулу для расчета времени с учетом ускорения
        // h = h0 + v0*t + 0.5*a*t^2, где a = -EffectiveGravity
        // Решаем: 0 = h0 + v0*t - 0.5*EffectiveGravity*t^2
        
        float a = -0.5f * EffectiveGravity;
        float b = Velocity.Z;
        float c = Position.Z;
        
        // Дискриминант
        float Discriminant = b * b - 4.0f * a * c;
        
        if (Discriminant >= 0.0f)
        {
            float t1 = (-b + FMath::Sqrt(Discriminant)) / (2.0f * a);
            float t2 = (-b - FMath::Sqrt(Discriminant)) / (2.0f * a);
            
            // Выбираем положительное время
            float TimeToGround = (t1 > 0.0f) ? t1 : t2;
            
            if (TimeToGround > 0.0f)
            {
                return TimeToGround;
            }
        }
        
        // Если квадратичная формула не работает, используем линейную аппроксимацию
        return -Position.Z / Velocity.Z;
    }
}

float ARadarActor::CalculateThreatLevel(const FMissileData& MissileData)
{
    float ThreatLevel = 0.0f;
    
    // Фактор расстояния (ближе = опаснее)
    float DistanceFactor = FMath::Clamp(1.0f - (MissileData.Distance / ScanRadius), 0.0f, 1.0f);
    
    // Фактор скорости (быстрее = опаснее)
    float SpeedFactor = FMath::Clamp(MissileData.Velocity.Size() / 2000.0f, 0.0f, 1.0f);
    
    // Фактор высоты (ниже = опаснее, так как ближе к цели)
    float HeightFactor = FMath::Clamp(1.0f - (MissileData.Position.Z / MaxDetectionHeight), 0.0f, 1.0f);
    
    // Фактор направления (движение к радару = опаснее)
    FVector DirectionToRadar = (GetActorLocation() - MissileData.Position).GetSafeNormal();
    float DirectionFactor = FVector::DotProduct(MissileData.Velocity.GetSafeNormal(), DirectionToRadar);
    DirectionFactor = FMath::Clamp(DirectionFactor, 0.0f, 1.0f);
    
    // Комбинируем факторы
    ThreatLevel = DistanceFactor * ThreatDistanceWeight +
                  SpeedFactor * ThreatSpeedWeight +
                  HeightFactor * ThreatHeightWeight +
                  DirectionFactor * 0.2f; // Дополнительный вес для направления
    
    return FMath::Clamp(ThreatLevel, 0.0f, 1.0f);
}

void ARadarActor::CleanupOldDetections()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    for (int32 i = DetectedMissiles.Num() - 1; i >= 0; i--)
    {
        if (CurrentTime - DetectedMissiles[i].LastDetectionTime > 5.0f) // Удаляем через 5 секунд без обнаружения
        {
            DetectedMissiles.RemoveAt(i);
        }
    }
}

void ARadarActor::SortMissilesByThreat()
{
    DetectedMissiles.Sort([](const FMissileData& A, const FMissileData& B) {
        return A.ThreatLevel > B.ThreatLevel; // Сортировка по убыванию угрозы
    });
}

void ARadarActor::CalculateImpactPoint(const FMissileData& MissileData)
{
    if (!MissileData.Missile || !MissileData.Missile->IsValidLowLevel())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("РАДАР: Ошибка - ракета недействительна"));
        return;
    }

    FVector CurrentPos = MissileData.Position;
    FVector Velocity = MissileData.Velocity;
    
    // Проверяем, что ракета движется
    if (Velocity.SizeSquared() < 1.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("РАДАР: Ошибка - ракета не движется"));
        return;
    }

    // Вычисляем время до падения на основе вертикальной скорости
    float TimeToGround = 0.0f;
    if (FMath::Abs(Velocity.Z) > 0.1f)
    {
        TimeToGround = -CurrentPos.Z / Velocity.Z;
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("РАДАР: Ошибка - недостаточная вертикальная скорость"));
        return;
    }

    if (TimeToGround <= 0.0f)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("РАДАР: Ошибка - ракета уже упала"));
        return;
    }

    // Вычисляем точку падения
    FVector ImpactPoint = CurrentPos + Velocity * TimeToGround;

    // Выводим информацию о траектории
    FString TrajectoryMessage = FString::Printf(TEXT("РАДАР: Анализ траектории ракеты:\n"
        "Текущая скорость: X=%.2f, Y=%.2f, Z=%.2f\n"
        "Время до падения: %.2f секунд\n"
        "Точка падения: X=%.2f, Y=%.2f, Z=%.2f\n"
        "Уровень угрозы: %.2f"),
        Velocity.X, Velocity.Y, Velocity.Z,
        TimeToGround,
        ImpactPoint.X, ImpactPoint.Y, ImpactPoint.Z,
        MissileData.ThreatLevel);
    
    GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TrajectoryMessage);
}

void ARadarActor::PlayPingSound()
{
    if (PingSound && AudioComponent)
    {
        AudioComponent->SetSound(PingSound);
        AudioComponent->Play();
    }
}

TArray<AMissleActor*> ARadarActor::GetMissilesDetectedThreeTimes() const
{
    TArray<AMissleActor*> Result;
    for (const FMissileData& MissileData : DetectedMissiles)
    {
        if (MissileData.DetectionCount >= 3 && MissileData.Missile && MissileData.Missile->IsValidLowLevel())
        {
            AMissleActor* Missile = Cast<AMissleActor>(MissileData.Missile);
            if (Missile)
            {
                Result.Add(Missile);
            }
        }
    }
    return Result;
} 