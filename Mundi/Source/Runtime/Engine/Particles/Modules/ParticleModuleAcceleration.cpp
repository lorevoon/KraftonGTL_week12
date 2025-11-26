#include "pch.h"
#include "ParticleModuleAcceleration.h"
#include "ParticleEmitterInstance.h"
#include "ParticleTypes.h"

UParticleModuleAcceleration::UParticleModuleAcceleration()
	: Acceleration(FVector(0.0f, 0.0f, -9.8f))  // 기본값: 중력
	, bUseAccelerationCurve(false)
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

		FVector CurrentAccel = Acceleration;

		// Apply curve if enabled
		if (bUseAccelerationCurve && AccelerationCurve.HasKeys())
		{
			FVector Scale = AccelerationCurve.EvaluateVector(Particle->RelativeTime);
			CurrentAccel.X *= Scale.X;
			CurrentAccel.Y *= Scale.Y;
			CurrentAccel.Z *= Scale.Z;
		}

		// v = v0 + a * dt
		Particle->Velocity = Particle->Velocity + CurrentAccel * DeltaTime;
		Particle->BaseVelocity = Particle->Velocity;
	}
}

void UParticleModuleAcceleration::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		if (InOutHandle.hasKey("bUseAccelerationCurve"))
		{
			bUseAccelerationCurve = InOutHandle["bUseAccelerationCurve"].ToBool();
		}
		if (InOutHandle.hasKey("AccelerationCurve"))
		{
			AccelerationCurve.Serialize(true, InOutHandle["AccelerationCurve"]);
		}
	}
	else
	{
		InOutHandle["bUseAccelerationCurve"] = bUseAccelerationCurve;
		if (bUseAccelerationCurve && AccelerationCurve.HasKeys())
		{
			JSON CurveJson = JSON::Make(JSON::Class::Object);
			AccelerationCurve.Serialize(false, CurveJson);
			InOutHandle["AccelerationCurve"] = CurveJson;
		}
	}
}

void UParticleModuleAcceleration::GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties)
{
	FCurvePropertyInfo AccelInfo;
	AccelInfo.PropertyName = "Acceleration Over Life";
	AccelInfo.NumChannels = 3;
	AccelInfo.CurvePtr = &AccelerationCurve;
	AccelInfo.bUseCurvePtr = &bUseAccelerationCurve;
	AccelInfo.ChannelNames = {"X", "Y", "Z"};
	AccelInfo.ChannelColors = {
		IM_COL32(255, 80, 80, 255),   // X - Red
		IM_COL32(80, 255, 80, 255),   // Y - Green
		IM_COL32(80, 80, 255, 255)    // Z - Blue
	};
	OutProperties.Add(AccelInfo);
}
