#include "pch.h"
#include "ParticleModuleBeamNoise.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include "ParticleLODLevel.h"
#include "TypeData/ParticleModuleTypeDataBeam.h"

UParticleModuleBeamNoise::UParticleModuleBeamNoise()
    : NoiseRange(FVector(0.2f, 0.2f, 0.0f))  // 20cm
    , NoiseLockTime(0.1f)
    , bSmooth(true)
    , bOscillate(false)
{
    bSpawnModule = true;
    bUpdateModule = true;
    bFinalUpdateModule = false;
    ModuleName = "Beam Noise";
}

void UParticleModuleBeamNoise::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
    if (!HasNoise() || !Owner)
        return;

    // 빔 타입 모듈 체크
    if (!Owner->CurrentLODLevel || !Owner->CurrentLODLevel->TypeDataModule)
        return;

    UParticleModuleTypeDataBeam* BeamTypeData = Cast<UParticleModuleTypeDataBeam>(Owner->CurrentLODLevel->TypeDataModule);
    if (!BeamTypeData)
        return;

    int32 Segments = BeamTypeData->Segments;

    // 캐시 가져오거나 생성
    if (!NoiseCaches.Contains(Owner))
    {
        NoiseCaches.Add(Owner, FBeamNoiseCache());
    }
    FBeamNoiseCache& Cache = *NoiseCaches.Find(Owner);

    // 새 파티클 인덱스 (ActiveParticles가 증가하기 전이므로 현재 값이 새 인덱스)
    int32 ParticleIndex = Owner->ActiveParticles;

    // 배열 확장
    if (ParticleIndex >= Cache.NoisePoints.Num())
    {
        int32 NewSize = ParticleIndex + 1;
        Cache.NoisePoints.SetNum(NewSize);
        Cache.NextNoisePoints.SetNum(NewSize);
        Cache.NoiseTimers.SetNum(NewSize);
    }

    // 노이즈 초기화
    InitializeParticleNoise(Cache, ParticleIndex, Segments);
}

void UParticleModuleBeamNoise::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
    if (!HasNoise() || !Owner || Owner->ActiveParticles == 0)
        return;

    // 빔 타입 모듈 체크
    if (!Owner->CurrentLODLevel || !Owner->CurrentLODLevel->TypeDataModule)
        return;

    UParticleModuleTypeDataBeam* BeamTypeData = Cast<UParticleModuleTypeDataBeam>(Owner->CurrentLODLevel->TypeDataModule);
    if (!BeamTypeData)
        return;

    int32 Segments = BeamTypeData->Segments;

    FBeamNoiseCache* CachePtr = NoiseCaches.Find(Owner);
    if (!CachePtr)
        return;

    FBeamNoiseCache& Cache = *CachePtr;

    // 파티클 수 변경 처리
    int32 ParticleCount = Owner->ActiveParticles;
    if (ParticleCount > Cache.NoisePoints.Num())
    {
        int32 OldSize = Cache.NoisePoints.Num();
        Cache.NoisePoints.SetNum(ParticleCount);
        Cache.NextNoisePoints.SetNum(ParticleCount);
        Cache.NoiseTimers.SetNum(ParticleCount);

        for (int32 i = OldSize; i < ParticleCount; ++i)
        {
            InitializeParticleNoise(Cache, i, Segments);
        }
    }

    // 노이즈 타이머 업데이트
    for (int32 p = 0; p < ParticleCount; ++p)
    {
        if (p >= Cache.NoiseTimers.Num())
            continue;

        Cache.NoiseTimers[p] += DeltaTime;

        // NoiseLockTime 초과 시 노이즈 갱신
        if (NoiseLockTime > 0.0001f && Cache.NoiseTimers[p] >= NoiseLockTime)
        {
            Cache.NoiseTimers[p] = 0.0f;

            if (bSmooth)
            {
                // 현재 → 다음으로 복사 후 새 다음 생성
                Cache.NoisePoints[p] = Cache.NextNoisePoints[p];
                GenerateNoisePoints(Cache.NextNoisePoints[p], Segments);
            }
            else
            {
                // 직접 갱신
                GenerateNoisePoints(Cache.NoisePoints[p], Segments);
            }
        }
    }
}

FBeamNoiseCache* UParticleModuleBeamNoise::GetNoiseCache(FParticleEmitterInstance* Owner)
{
    return NoiseCaches.Find(Owner);
}

void UParticleModuleBeamNoise::GenerateNoisePoints(TArray<FVector>& OutPoints, int32 SegmentCount)
{
    // 내부 포인트만 (시작/끝 제외)
    int32 NoisePointCount = FMath::Max(0, SegmentCount - 1);
    OutPoints.SetNum(NoisePointCount);

    int32 Extreme = -1;
    for (int32 i = 0; i < NoisePointCount; ++i)
    {
        if (bOscillate)
        {
            Extreme = -Extreme;
            OutPoints[i] = NoiseRange * (float)Extreme;
        }
        else
        {
            auto RandF = []() { return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f; };
            OutPoints[i] = FVector(
                RandF() * NoiseRange.X,
                RandF() * NoiseRange.Y,
                RandF() * NoiseRange.Z
            );
        }
    }
}

void UParticleModuleBeamNoise::InitializeParticleNoise(FBeamNoiseCache& Cache, int32 ParticleIndex, int32 SegmentCount)
{
    GenerateNoisePoints(Cache.NoisePoints[ParticleIndex], SegmentCount);
    GenerateNoisePoints(Cache.NextNoisePoints[ParticleIndex], SegmentCount);
    Cache.NoiseTimers[ParticleIndex] = 0.0f;
}
