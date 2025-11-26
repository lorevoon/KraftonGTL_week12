#pragma once

#include "ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Curves/ParticleCurve.h"
#include "UParticleModuleRotation.generated.h"

// 파티클 회전 모듈
// - Spawn 단계에서 초기 회전 각도 및 회전 속도 설정
// - Update 단계에서 시간에 따른 회전 처리 (회전 속도 또는 커브)
UCLASS(DisplayName="회전 모듈", Description="파티클 회전을 제어합니다")
class UParticleModuleRotation : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleRotation();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	virtual void GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties) override;

	// 시작 회전 최소값 (Degrees)
	UPROPERTY(EditAnywhere, Category="Rotation")
	float StartRotationMin;

	// 시작 회전 최대값 (Degrees)
	UPROPERTY(EditAnywhere, Category="Rotation")
	float StartRotationMax;

	// 회전 속도 최소값 (Degrees per second)
	UPROPERTY(EditAnywhere, Category="Rotation")
	float RotationRateMin;

	// 회전 속도 최대값 (Degrees per second)
	UPROPERTY(EditAnywhere, Category="Rotation")
	float RotationRateMax;

	// 커브 모드 사용 여부
	UPROPERTY(EditAnywhere, Category="Rotation")
	bool bUseRotationCurve;

	// 회전 커브 (1채널) - 시간에 따른 회전 스케일
	FParticleCurve RotationCurve;
};
