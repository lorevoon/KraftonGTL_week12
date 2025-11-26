#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleRotation::UParticleModuleRotation()
	: StartRotationMin(0.0f)
	, StartRotationMax(0.0f)
	, RotationRateMin(0.0f)
	, RotationRateMax(0.0f)
	, bUseRotationCurve(false)
{
	bSpawnModule = true;
	bUpdateModule = true;
	bFinalUpdateModule = false;
	ModuleName = "Rotation";
}

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);

	// 시작 회전 각도 설정 (Degrees -> Radians)
	float StartRotation;
	if (StartRotationMax > StartRotationMin)
	{
		float Rand = dist(gen);
		StartRotation = StartRotationMin + (StartRotationMax - StartRotationMin) * Rand;
	}
	else
	{
		StartRotation = StartRotationMin;
	}
	Particle->Rotation = StartRotation * (3.14159265358979f / 180.0f);

	// 회전 속도 설정 (Degrees per second -> Radians per second)
	float RotationRate;
	if (RotationRateMax > RotationRateMin)
	{
		float Rand = dist(gen);
		RotationRate = RotationRateMin + (RotationRateMax - RotationRateMin) * Rand;
	}
	else
	{
		RotationRate = RotationRateMin;
	}
	Particle->BaseRotationRate = RotationRate * (3.14159265358979f / 180.0f);
}

void UParticleModuleRotation::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	MUNDI_BEGIN_UPDATE_LOOP
	{
		if (bUseRotationCurve && RotationCurve.HasKeys())
		{
			// 커브 모드: 시간에 따른 회전 스케일
			float Time = Particle->RelativeTime;
			float Scale = RotationCurve.EvaluateChannel(0, Time);
			Particle->Rotation += Particle->BaseRotationRate * Scale * DeltaTime;
		}
		else
		{
			// 일반 모드: 회전 속도로 회전
			Particle->Rotation += Particle->BaseRotationRate * DeltaTime;
		}
	}
	MUNDI_END_UPDATE_LOOP;
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("bUseRotationCurve"))
		{
			bUseRotationCurve = InOutHandle["bUseRotationCurve"].ToBool();
		}
		if (InOutHandle.hasKey("RotationCurve"))
		{
			RotationCurve.Serialize(true, InOutHandle["RotationCurve"]);
		}
	}
	else
	{
		InOutHandle["bUseRotationCurve"] = bUseRotationCurve;
		if (bUseRotationCurve && RotationCurve.HasKeys())
		{
			JSON CurveJson = JSON::Make(JSON::Class::Object);
			RotationCurve.Serialize(false, CurveJson);
			InOutHandle["RotationCurve"] = CurveJson;
		}
	}
}

void UParticleModuleRotation::GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties)
{
	FCurvePropertyInfo RotationInfo;
	RotationInfo.PropertyName = "Rotation Rate Over Life";
	RotationInfo.NumChannels = 1;
	RotationInfo.CurvePtr = &RotationCurve;
	RotationInfo.bUseCurvePtr = &bUseRotationCurve;
	RotationInfo.ChannelNames = {"Rate"};
	RotationInfo.ChannelColors = {
		IM_COL32(255, 200, 80, 255)   // Orange-ish
	};
	OutProperties.Add(RotationInfo);
}
