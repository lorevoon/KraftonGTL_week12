#pragma once

#include "ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Curves/ParticleCurve.h"
#include "UParticleModuleSize.generated.h"

// 파티클 크기 모듈
// - Spawn 단계에서 초기 크기 설정
// - Min/Max 범위 내에서 랜덤 크기 생성
// - Update 단계에서 시간에 따른 크기 변화 (커브 모드)
UCLASS(DisplayName="크기 모듈", Description="파티클 크기를 제어합니다")
class UParticleModuleSize : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleSize();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	virtual void GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties) override;

	// 시작 크기 최소값
	UPROPERTY(EditAnywhere, Category="Size")
	FVector StartSizeMin;

	// 시작 크기 최대값
	UPROPERTY(EditAnywhere, Category="Size")
	FVector StartSizeMax;

	// 커브 모드 사용 여부
	UPROPERTY(EditAnywhere, Category="Size")
	bool bUseSizeCurve;

	// 크기 커브 (3채널: X, Y, Z) - BaseSize에 곱해지는 스케일 값
	FParticleCurve SizeCurve;
};
