#include "pch.h"
#include "ParticleModuleAcceleration.h"
#include "ParticleEmitterInstance.h"
#include "ParticleTypes.h"

UParticleModuleAcceleration::UParticleModuleAcceleration()
	: Acceleration(FVector(0.0f, 0.0f, -9.8f))  // 기본값: 중력
{
	bSpawnModule = false;
	bUpdateModule = true;   // Update 단계에서 실행
	bFinalUpdateModule = false;
	ModuleName = "Acceleration";
}

void UParticleModuleAcceleration::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (!Owner)
	{
		return;
	}

	// 모든 활성 파티클에 가속도 적용
	for (int32 i = 0; i < Owner->ActiveParticles; ++i)
	{
		FBaseParticle* Particle = Owner->GetParticle(i);
		if (!Particle)
		{
			continue;
		}

		// v = v0 + a * dt
		Particle->Velocity = Particle->Velocity + Acceleration * DeltaTime;
		Particle->BaseVelocity = Particle->Velocity;
	}
}
