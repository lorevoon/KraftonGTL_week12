#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleTypes.h"
#include "ParticleHelper.h"

UParticleModuleColor::UParticleModuleColor()
	: StartColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
	, EndColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
	, bUseCurve(false)
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
	if (bUseCurve && ColorCurve.HasKeys())
	{
		// 커브 모드: 시간 0에서의 값으로 시작
		Particle->Color = ColorCurve.EvaluateColor(0.0f);
	}
	else
	{
		// 기존 모드: StartColor로 시작
		Particle->Color = StartColor;
	}
	Particle->BaseColor = Particle->Color;
}

void UParticleModuleColor::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	MUNDI_BEGIN_UPDATE_LOOP
	{
		float Time = Particle->RelativeTime;

		if (bUseCurve && ColorCurve.HasKeys())
		{
			// 커브 모드: 커브에서 색상 평가
			Particle->Color = ColorCurve.EvaluateColor(Time);
		}
		else
		{
			// 기존 모드: StartColor에서 EndColor로 선형 보간
			Particle->Color.R = StartColor.R + (EndColor.R - StartColor.R) * Time;
			Particle->Color.G = StartColor.G + (EndColor.G - StartColor.G) * Time;
			Particle->Color.B = StartColor.B + (EndColor.B - StartColor.B) * Time;
			Particle->Color.A = StartColor.A + (EndColor.A - StartColor.A) * Time;
		}
	}
	MUNDI_END_UPDATE_LOOP;
}

void UParticleModuleColor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	// 기본 모듈 직렬화 (StartColor, EndColor 등 UPROPERTY 처리)
	Super::Serialize(bInIsLoading, InOutHandle);

	// 커브 데이터 직렬화
	if (bInIsLoading)
	{
		// 로드
		if (InOutHandle.hasKey("bUseCurve"))
		{
			bUseCurve = InOutHandle["bUseCurve"].ToBool();
		}
		if (InOutHandle.hasKey("ColorCurve"))
		{
			ColorCurve.Serialize(true, InOutHandle["ColorCurve"]);
		}
	}
	else
	{
		// 저장
		InOutHandle["bUseCurve"] = bUseCurve;
		if (bUseCurve && ColorCurve.HasKeys())
		{
			JSON CurveJson = JSON::Make(JSON::Class::Object);
			ColorCurve.Serialize(false, CurveJson);
			InOutHandle["ColorCurve"] = CurveJson;
		}
	}
}

void UParticleModuleColor::GetCurveProperties(TArray<FCurvePropertyInfo>& OutProperties)
{
	FCurvePropertyInfo ColorInfo;
	ColorInfo.PropertyName = "Color Over Life";
	ColorInfo.NumChannels = 4;
	ColorInfo.CurvePtr = &ColorCurve;
	ColorInfo.bUseCurvePtr = &bUseCurve;
	ColorInfo.ChannelNames = {"R", "G", "B", "A"};
	ColorInfo.ChannelColors = {
		IM_COL32(255, 80, 80, 255),   // Red
		IM_COL32(80, 255, 80, 255),   // Green
		IM_COL32(80, 80, 255, 255),   // Blue
		IM_COL32(200, 200, 200, 255)  // Alpha (gray)
	};
	OutProperties.Add(ColorInfo);
}
