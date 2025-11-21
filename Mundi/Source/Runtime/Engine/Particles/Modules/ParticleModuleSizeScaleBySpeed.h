#pragma once
#include "ParticleModule.h"
#include "UParticleModuleSizeScaleBySpeed.generated.h"

/**
 * 속도에 따른 크기 조절 모듈
 * 파티클의 속도가 빠를수록 크기가 커짐
 * ⭐ UE5 패턴: Update 단계에서만 실행
 */
UCLASS(DisplayName="Size Scale by Speed", Description="속도 기반 크기 조절")
class UParticleModuleSizeScaleBySpeed : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleSizeScaleBySpeed();

	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

	/** 속도 스케일 계수 (속도에 곱해짐) */
	UPROPERTY(EditAnywhere, Category="Size")
	FVector SpeedScale;

	/** 최대 크기 제한 */
	UPROPERTY(EditAnywhere, Category="Size")
	FVector MaxScale;
};
