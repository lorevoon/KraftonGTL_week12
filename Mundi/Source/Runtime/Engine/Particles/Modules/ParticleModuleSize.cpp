#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleSize::UParticleModuleSize()
	: StartSizeMin(FVector(1.0f, 1.0f, 1.0f))
	, StartSizeMax(FVector(1.0f, 1.0f, 1.0f))
{
	bSpawnModule = true;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Size";
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// Min/Max 사이 랜덤 크기 계산
	FVector Size;

	if (StartSizeMax.X > StartSizeMin.X ||
		StartSizeMax.Y > StartSizeMin.Y ||
		StartSizeMax.Z > StartSizeMin.Z)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);

		float RandX = dist(gen);
		float RandY = dist(gen);
		float RandZ = dist(gen);

		Size.X = StartSizeMin.X + (StartSizeMax.X - StartSizeMin.X) * RandX;
		Size.Y = StartSizeMin.Y + (StartSizeMax.Y - StartSizeMin.Y) * RandY;
		Size.Z = StartSizeMin.Z + (StartSizeMax.Z - StartSizeMin.Z) * RandZ;
	}
	else
	{
		Size = StartSizeMin;
	}

	// 크기 설정
	Particle->Size = Size;
	Particle->BaseSize = Size;
}

void UParticleModuleSize::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	// 현재는 크기 변화 없음 (필요 시 추후 구현)
	// 예: 시간에 따라 크기 감소/증가
}
