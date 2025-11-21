#pragma once

#include "ParticleModule.h"
#include "UParticleModuleVelocity.generated.h"

// 파티클 속도 모듈
// - Spawn 단계에서 파티클 초기 속도 설정
// - Min/Max 범위 내에서 랜덤 속도 벡터 생성
UCLASS(DisplayName="속도 모듈", Description="파티클 초기 속도를 설정합니다")
class UParticleModuleVelocity : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleVelocity();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	// 초기 속도 최소값
	UPROPERTY(EditAnywhere, Category="Velocity")
	FVector StartVelocityMin;

	// 초기 속도 최대값
	UPROPERTY(EditAnywhere, Category="Velocity")
	FVector StartVelocityMax;
};
