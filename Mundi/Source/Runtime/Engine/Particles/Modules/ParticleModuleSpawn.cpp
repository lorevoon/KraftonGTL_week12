#include "pch.h"
#include "ParticleModuleSpawn.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleSpawn::UParticleModuleSpawn()
	: LocationMin(FVector())
	, LocationMax(FVector())
	, DistributionType(EDistributionType::Constant)
{
	bSpawnModule = true;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Spawn";
}

void UParticleModuleSpawn::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	// MUNDI_SPAWN_INIT: 3개 파라미터 (Owner, Offset, ParticleBase)
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// 분포 타입별 위치 계산
	FVector SpawnLocation;

	switch (DistributionType)
	{
	case EDistributionType::Constant:
		SpawnLocation = LocationMin;
		break;

	case EDistributionType::Uniform:
		{
			static std::random_device rd;
			static std::mt19937 gen(rd());
			std::uniform_real_distribution<float> dist(0.0f, 1.0f);

			float RandX = dist(gen);
			float RandY = dist(gen);
			float RandZ = dist(gen);

			SpawnLocation.X = LocationMin.X + (LocationMax.X - LocationMin.X) * RandX;
			SpawnLocation.Y = LocationMin.Y + (LocationMax.Y - LocationMin.Y) * RandY;
			SpawnLocation.Z = LocationMin.Z + (LocationMax.Z - LocationMin.Z) * RandZ;
		}
		break;

	default:
		SpawnLocation = LocationMin;
		break;
	}

	// World Space면 컴포넌트 월드 위치를 더함
	if (!Owner->UseLocalSpace())
	{
		SpawnLocation = SpawnLocation + Owner->GetComponentWorldLocation();
	}

	// 파티클 위치 설정
	Particle->Location = SpawnLocation;
	Particle->OldLocation = SpawnLocation;
}
