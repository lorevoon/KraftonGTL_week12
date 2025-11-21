#include "pch.h"
#include "ParticleModuleSizeScaleBySpeed.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"

UParticleModuleSizeScaleBySpeed::UParticleModuleSizeScaleBySpeed()
	: SpeedScale(0.01f, 0.01f, 0.0f)
	, MaxScale(5.0f, 5.0f, 5.0f)
{
	// ⭐ UE5 패턴: Update 단계에서만 실행
	bSpawnModule = false;
	bUpdateModule = true;
	bFinalUpdateModule = false;
	ModuleName = "SizeScaleBySpeed";
}

void UParticleModuleSizeScaleBySpeed::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (!Owner || !bEnabled)
		return;

	// ⭐ UE5 패턴: 헬퍼 매크로 사용
	MUNDI_BEGIN_UPDATE_LOOP
	{
		// 속도 크기 계산
		float Speed = Particle->Velocity.Size();

		// 크기 스케일 적용 (1.0에서 시작하여 속도에 비례)
		FVector Scale;
		Scale.X = 1.0f + (SpeedScale.X * Speed);
		Scale.Y = 1.0f + (SpeedScale.Y * Speed);
		Scale.Z = 1.0f + (SpeedScale.Z * Speed);

		// 최소값 1.0 보장
		Scale.X = FMath::Max(Scale.X, 1.0f);
		Scale.Y = FMath::Max(Scale.Y, 1.0f);
		Scale.Z = FMath::Max(Scale.Z, 1.0f);

		// 최대값 제한
		Scale.X = FMath::Min(Scale.X, MaxScale.X);
		Scale.Y = FMath::Min(Scale.Y, MaxScale.Y);
		Scale.Z = FMath::Min(Scale.Z, MaxScale.Z);

		// ⭐ UE5 패턴: BaseSize 기반 계산 (누적 오차 방지)
		Particle->Size.X = Particle->BaseSize.X * Scale.X;
		Particle->Size.Y = Particle->BaseSize.Y * Scale.Y;
		Particle->Size.Z = Particle->BaseSize.Z * Scale.Z;
	}
	MUNDI_END_UPDATE_LOOP;
}
