#pragma once

#include "Object.h"
#include "ParticleLODLevel.h"
#include "UParticleEmitter.generated.h"

// 논리적 이미터 (에셋)
// - 여러 LOD 레벨을 포함
// - 런타임에 FParticleEmitterInstance로 인스턴스화됨
UCLASS(DisplayName="파티클 이미터", Description="LOD 레벨을 포함하는 이미터 에셋입니다")
class UParticleEmitter : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleEmitter();

    // LOD 레벨 배열
    // - LODLevels[0]: 최고 품질 (가까운 거리)
    // - LODLevels[N]: 최저 품질 (먼 거리)
    UPROPERTY(EditAnywhere, Category="LOD")
    TArray<UParticleLODLevel*> LODLevels;

    // 이미터 이름 (디버깅/에디터 용)
    UPROPERTY(EditAnywhere, Category="Emitter")
    FString EmitterName;

    // 최대 파티클 수
    UPROPERTY(EditAnywhere, Category="Emitter")
    int32 MaxParticleCount;

public:
    // LOD 레벨 접근
    UParticleLODLevel* GetLODLevel(int32 LODIndex) const;

    // LOD 레벨 추가
    void AddLODLevel(UParticleLODLevel* LODLevel);

    // 기본 LOD 레벨 (가장 가까운 거리용)
    UParticleLODLevel* GetDefaultLODLevel() const
    {
        return LODLevels.Num() > 0 ? LODLevels[0] : nullptr;
    }

    // Serialization
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
