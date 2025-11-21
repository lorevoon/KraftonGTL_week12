#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"

UParticleModuleColor::UParticleModuleColor()
	: StartColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
	, EndColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
{
	bSpawnModule = true;
	bUpdateModule = true;
	bFinalUpdateModule = false;
	ModuleName = "Color";
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	// 초기 색상 설정
	Particle->Color = StartColor;
	Particle->BaseColor = StartColor;
}

void UParticleModuleColor::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	MUNDI_BEGIN_UPDATE_LOOP
	{
		// RelativeTime을 기반으로 StartColor에서 EndColor로 보간
		float Alpha = Particle->RelativeTime;

		Particle->Color.R = StartColor.R + (EndColor.R - StartColor.R) * Alpha;
		Particle->Color.G = StartColor.G + (EndColor.G - StartColor.G) * Alpha;
		Particle->Color.B = StartColor.B + (EndColor.B - StartColor.B) * Alpha;
		Particle->Color.A = StartColor.A + (EndColor.A - StartColor.A) * Alpha;
	}
	MUNDI_END_UPDATE_LOOP;
}
