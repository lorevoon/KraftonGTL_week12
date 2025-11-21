#pragma once

#include "ParticleModule.h"
#include "UParticleModuleLifetime.generated.h"

// 파티클 수명 모듈
// - Spawn 단계에서 파티클 수명 설정
// - Min/Max 범위 내에서 랜덤 수명 결정
UCLASS(DisplayName="수명 모듈", Description="파티클 수명을 제어합니다")
class UParticleModuleLifetime : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleLifetime();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	// 최소 수명 (초 단위)
	UPROPERTY(EditAnywhere, Category="Lifetime")
	float LifetimeMin;

	// 최대 수명 (초 단위)
	UPROPERTY(EditAnywhere, Category="Lifetime")
	float LifetimeMax;
};
