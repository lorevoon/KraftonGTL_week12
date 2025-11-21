#include "pch.h"
#include "ParticleModuleLocation.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"

UParticleModuleLocation::UParticleModuleLocation()
	: StartLocation(FVector())
{
	bSpawnModule = true;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Location";
}

void UParticleModuleLocation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// 시작 위치 오프셋 적용
	Particle->Location.X += StartLocation.X;
	Particle->Location.Y += StartLocation.Y;
	Particle->Location.Z += StartLocation.Z;

	Particle->OldLocation = Particle->Location;
}
