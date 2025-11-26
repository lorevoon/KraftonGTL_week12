#include "pch.h"
#include "ParticleModuleSize.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"
#include <random>

UParticleModuleSize::UParticleModuleSize()
	: StartSizeMin(FVector(1.0f, 1.0f, 1.0f))
	, StartSizeMax(FVector(1.0f, 1.0f, 1.0f))
	, bUseSizeCurve(false)
{
	bSpawnModule = true;
	bUpdateModule = true;  // Enable update for curve mode
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
	Particle->BaseSize = Size;

	// Apply curve at time 0 if enabled
	if (bUseSizeCurve && SizeCurve.HasKeys())
	{
		FVector Scale = SizeCurve.EvaluateVector(0.0f);
		Particle->Size.X = Size.X * Scale.X;
		Particle->Size.Y = Size.Y * Scale.Y;
		Particle->Size.Z = Size.Z * Scale.Z;
	}
	else
	{
		Particle->Size = Size;
	}
}

void UParticleModuleSize::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (!bUseSizeCurve || !SizeCurve.HasKeys()) return;

	MUNDI_BEGIN_UPDATE_LOOP
	{
		float Time = Particle->RelativeTime;
		FVector Scale = SizeCurve.EvaluateVector(Time);

		// Multiply base size by curve scale
		Particle->Size.X = Particle->BaseSize.X * Scale.X;
		Particle->Size.Y = Particle->BaseSize.Y * Scale.Y;
		Particle->Size.Z = Particle->BaseSize.Z * Scale.Z;
	}
	MUNDI_END_UPDATE_LOOP;
}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("bUseSizeCurve"))
		{
			bUseSizeCurve = InOutHandle["bUseSizeCurve"].ToBool();
		}
		if (InOutHandle.hasKey("SizeCurve"))
		{
			SizeCurve.Serialize(true, InOutHandle["SizeCurve"]);
		}
	}
	else
	{
		InOutHandle["bUseSizeCurve"] = bUseSizeCurve;
		if (bUseSizeCurve && SizeCurve.HasKeys())
		{
			JSON CurveJson = JSON::Make(JSON::Class::Object);
			SizeCurve.Serialize(false, CurveJson);
			InOutHandle["SizeCurve"] = CurveJson;
		}
	}
}

void UParticleModuleSize::GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties)
{
	FCurvePropertyInfo SizeInfo;
	SizeInfo.PropertyName = "Size Over Life";
	SizeInfo.NumChannels = 3;
	SizeInfo.CurvePtr = &SizeCurve;
	SizeInfo.bUseCurvePtr = &bUseSizeCurve;
	SizeInfo.ChannelNames = {"X", "Y", "Z"};
	SizeInfo.ChannelColors = {
		IM_COL32(255, 80, 80, 255),   // X - Red
		IM_COL32(80, 255, 80, 255),   // Y - Green
		IM_COL32(80, 80, 255, 255)    // Z - Blue
	};
	OutProperties.Add(SizeInfo);
}
