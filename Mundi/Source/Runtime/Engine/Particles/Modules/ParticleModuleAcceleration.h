#pragma once

#include "ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Curves/ParticleCurve.h"
#include "UParticleModuleAcceleration.generated.h"

// 파티클 가속도 모듈
// - Update 단계에서 매 프레임 가속도를 속도에 적용
// - 중력, 바람 등 지속적인 힘 시뮬레이션용
// - 커브 모드로 시간에 따른 가속도 변화 가능
UCLASS(DisplayName="가속도 모듈", Description="파티클에 가속도를 적용합니다")
class UParticleModuleAcceleration : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleAcceleration();

	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	virtual void GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties) override;

	// 가속도 벡터 (단위: units/sec²)
	// 예: (0, 0, -9.8) = 중력 (Z-up 좌표계에서 아래 방향)
	UPROPERTY(EditAnywhere, Category="Acceleration")
	FVector Acceleration;

	// 커브 모드 사용 여부
	UPROPERTY(EditAnywhere, Category="Acceleration")
	bool bUseAccelerationCurve;

	// 가속도 커브 (3채널: X, Y, Z) - 기본 가속도에 곱해지는 스케일 값
	FParticleCurve AccelerationCurve;
};
