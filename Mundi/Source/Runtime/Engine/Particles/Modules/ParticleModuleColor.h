#pragma once

#include "ParticleModule.h"
#include "UParticleModuleColor.generated.h"

// 파티클 색상 모듈
// - Spawn 단계에서 초기 색상 설정
// - Update 단계에서 시간에 따라 색상 변화 (StartColor -> EndColor)
UCLASS(DisplayName="색상 모듈", Description="파티클 색상을 제어합니다")
class UParticleModuleColor : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleColor();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

	// 시작 색상
	UPROPERTY(EditAnywhere, Category="Color")
	FLinearColor StartColor;

	// 종료 색상
	UPROPERTY(EditAnywhere, Category="Color")
	FLinearColor EndColor;
};
