#pragma once

#include "ParticleModule.h"
#include "UParticleModuleRequired.generated.h"

// 전방 선언
class UMaterialInterface;

// 필수 모듈
// - 모든 Emitter가 반드시 가져야 하는 모듈
// - SpawnRate, EmitterDuration, EmitterLoops 등 핵심 설정
UCLASS(DisplayName="파티클 필수 모듈", Description="모든 이미터가 반드시 가져야 하는 필수 모듈입니다")
class UParticleModuleRequired : public UParticleModule
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleRequired();

    // 스폰 설정
    UPROPERTY(EditAnywhere, Category="Spawn")
    float SpawnRate;                    // 초당 스폰할 파티클 수

    // 이미터 생명주기
    UPROPERTY(EditAnywhere, Category="Duration")
    float EmitterDuration;              // 이미터 지속 시간 (초, 0 = 무한)

    UPROPERTY(EditAnywhere, Category="Duration")
    int32 EmitterLoops;                 // 반복 횟수 (0 = 무한)

    // 렌더링 설정
    UPROPERTY(EditAnywhere, Category="Rendering")
    UMaterialInterface* Material;       // 파티클 렌더링 머티리얼

    UPROPERTY(EditAnywhere, Category="Rendering")
    EParticleSortMode SortMode;         // 파티클 정렬 모드

    UPROPERTY(EditAnywhere, Category="Rendering")
    FVector EmitterOrigin;              // 이미터 원점 오프셋

    UPROPERTY(EditAnywhere, Category="Rendering")
    float EmitterRotation;              // 이미터 회전 (Degrees)

public:
    // Spawn 단계에서만 실행
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
};
