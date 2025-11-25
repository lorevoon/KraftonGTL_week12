#pragma once

#include "ParticleModule.h"
#include "UParticleModuleBeamNoise.generated.h"

// 이미터 인스턴스별 노이즈 캐시
struct FBeamNoiseCache
{
    TArray<TArray<FVector>> NoisePoints;      // [파티클][세그먼트]
    TArray<TArray<FVector>> NextNoisePoints;  // [파티클][세그먼트] (bSmooth용)
    TArray<float> NoiseTimers;                // [파티클]
    int32 CachedParticleCount = 0;

    void Reset()
    {
        NoisePoints.Empty();
        NextNoisePoints.Empty();
        NoiseTimers.Empty();
        CachedParticleCount = 0;
    }
};

// 빔 노이즈 모듈
// - 빔 파티클에 노이즈/진동 효과 적용
// - NoiseLockTime 간격으로 노이즈 갱신
UCLASS(DisplayName="빔 노이즈", Description="빔 파티클에 노이즈 효과를 추가합니다")
class UParticleModuleBeamNoise : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleBeamNoise();

    // 노이즈 범위 (X=Right, Y=Up 방향, 미터 단위)
    UPROPERTY(EditAnywhere, Category="Noise")
    FVector NoiseRange;

    // 노이즈 갱신 주기 (초). 0이면 매 프레임
    UPROPERTY(EditAnywhere, Category="Noise")
    float NoiseLockTime;

    // 부드러운 노이즈 보간
    UPROPERTY(EditAnywhere, Category="Noise")
    bool bSmooth;

    // 지그재그 진동 모드
    UPROPERTY(EditAnywhere, Category="Noise")
    bool bOscillate;

    // 모듈 인터페이스
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

    // 렌더링용 노이즈 캐시 접근
    FBeamNoiseCache* GetNoiseCache(FParticleEmitterInstance* Owner);

    // 노이즈 유효 여부
    bool HasNoise() const { return NoiseRange.SizeSquared() > 0.0001f; }

private:
    // 이미터 인스턴스별 노이즈 캐시 (모듈은 에셋 공유, 런타임 데이터는 인스턴스별 분리)
    TMap<FParticleEmitterInstance*, FBeamNoiseCache> NoiseCaches;

    // 노이즈 포인트 생성
    void GenerateNoisePoints(TArray<FVector>& OutPoints, int32 SegmentCount);

    // 파티클 노이즈 초기화
    void InitializeParticleNoise(FBeamNoiseCache& Cache, int32 ParticleIndex, int32 SegmentCount);
};
