#pragma once
#include "ParticleTypes.h"
#include "ParticleModule.h"
#include "UParticleModuleSpawn.generated.h"

// 파티클 생성 모듈
// - Spawn 단계에서 파티클 초기 위치 설정
// - 분포 타입에 따라 Constant/Uniform 분포 지원
UCLASS(DisplayName="생성 모듈", Description="파티클 생성 위치를 제어합니다")
class UParticleModuleSpawn : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleSpawn();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;

	// 위치 분포 최소값
	UPROPERTY(EditAnywhere, Category="Location")
	FVector LocationMin;

	// 위치 분포 최대값
	UPROPERTY(EditAnywhere, Category="Location")
	FVector LocationMax;

	// 분포 타입 (Constant, Uniform 등)
	UPROPERTY(EditAnywhere, Category="Location")
	EDistributionType DistributionType;
};
