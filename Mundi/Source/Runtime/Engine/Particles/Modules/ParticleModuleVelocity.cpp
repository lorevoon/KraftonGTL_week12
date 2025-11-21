#include "pch.h"
#include "ParticleModuleVelocity.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleVelocity::UParticleModuleVelocity()
	: StartVelocityMin(FVector())
	, StartVelocityMax(FVector())
{
	bSpawnModule = true;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Velocity";
}

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// Min/Max 사이 랜덤 속도 계산
	FVector Velocity;

	if (StartVelocityMax.X > StartVelocityMin.X ||
		StartVelocityMax.Y > StartVelocityMin.Y ||
		StartVelocityMax.Z > StartVelocityMin.Z)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		float RandX = dist(gen);
		float RandY = dist(gen);
		float RandZ = dist(gen);

		Velocity.X = StartVelocityMin.X + (StartVelocityMax.X - StartVelocityMin.X) * RandX;
		Velocity.Y = StartVelocityMin.Y + (StartVelocityMax.Y - StartVelocityMin.Y) * RandY;
		Velocity.Z = StartVelocityMin.Z + (StartVelocityMax.Z - StartVelocityMin.Z) * RandZ;
	}
	else
	{
		Velocity = StartVelocityMin;
	}

	// 속도 설정
	Particle->Velocity = Velocity;
	Particle->BaseVelocity = Velocity;
}
