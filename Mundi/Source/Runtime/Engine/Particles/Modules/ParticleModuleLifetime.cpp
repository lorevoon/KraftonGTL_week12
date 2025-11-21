#include "pch.h"
#include "ParticleModuleLifetime.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleLifetime::UParticleModuleLifetime()
	: LifetimeMin(1.0f)
	, LifetimeMax(1.0f)
{
	bSpawnModule = true;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Lifetime";
}

void UParticleModuleLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// Min/Max 사이 랜덤 수명 계산
	float Lifetime = LifetimeMin;

	if (LifetimeMax > LifetimeMin)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		float RandValue = dist(gen);
		Lifetime = LifetimeMin + (LifetimeMax - LifetimeMin) * RandValue;
	}

	// OneOverMaxLifetime 최적화 적용
	if (Lifetime > 0.0f)
	{
		Particle->OneOverMaxLifetime = 1.0f / Lifetime;
	}
	else
	{
		Particle->OneOverMaxLifetime = 0.0f;
	}

	// RelativeTime 초기화
	Particle->RelativeTime = 0.0f;
}
