#pragma once

#include "ParticleModule.h"
#include "UParticleModuleLocation.generated.h"

// 파티클 위치 모듈
// - Spawn 단계에서 파티클 초기 위치 오프셋 설정
// - 이미터 원점 기준 상대 위치
UCLASS(DisplayName="위치 모듈", Description="파티클 초기 위치를 설정합니다")
class UParticleModuleLocation : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleLocation();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	// 시작 위치 오프셋
	UPROPERTY(EditAnywhere, Category="Location")
	FVector StartLocation;
};
