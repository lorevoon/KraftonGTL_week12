#pragma once

#include "ParticleModule.h"
#include "Source/Runtime/Engine/Particles/Curves/ParticleCurve.h"
#include "UParticleModuleColor.generated.h"

// 파티클 색상 모듈
// - Spawn 단계에서 초기 색상 설정
// - Update 단계에서 시간에 따라 색상 변화 (StartColor -> EndColor 또는 Curve)
UCLASS(DisplayName="색상 모듈", Description="파티클 색상을 제어합니다")
class UParticleModuleColor : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleColor();

	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;
	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
	virtual void GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties) override;

	// 시작 색상 (bUseCurve가 false일 때 사용)
	UPROPERTY(EditAnywhere, Category="Color")
	FLinearColor StartColor;

	// 종료 색상 (bUseCurve가 false일 때 사용)
	UPROPERTY(EditAnywhere, Category="Color")
	FLinearColor EndColor;

	// 커브 모드 사용 여부
	UPROPERTY(EditAnywhere, Category="Color")
	bool bUseCurve;

	// 색상 커브 (4채널: R, G, B, A)
	// bUseCurve가 true일 때 사용
	FParticleCurve ColorCurve;
};
