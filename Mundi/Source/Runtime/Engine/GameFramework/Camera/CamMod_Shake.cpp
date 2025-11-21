#include "pch.h"
#include "CamMod_Shake.h"
#include "SceneView.h"
#include <cmath>
#include <algorithm>

IMPLEMENT_CLASS(UCamMod_Shake)

const uint8 UCamMod_Shake::Perm[256] = {
	151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142, // 표준 Perlin perm
	8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,
	117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,
	71,134,139,48,27,166,77,146,158,231,83,111,229,122, 60,211,133,230,220,105,
	92,41,55,46,245,40,244,102,143,54, 65,25,63,161, 1,216,80,73,209,76,132,187,
	208,89,18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,
	52,217,226,250,124,123, 5,202,38,147,118,126,255,82,85,212,207,206,59,227,
	47,16,58,17,182,189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,
	153,101,155,167, 43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185,
	112,104,218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,
	145,235,249,14,239,107, 49,192,214, 31,181,199,106,157,184, 84,204,176,115,
	121,50,45,127, 4,150,254,138,236,205,93,222,114, 67,29,24,72,243,141,128,195
};

UCamMod_Shake::UCamMod_Shake()
{
	Priority = 0;
	bEnabled = true;
	Weight = 1.0f;
	Duration = -1.f; // 무한 기본
}

void UCamMod_Shake::Initialize(float InDuration, float InAmpLoc, float InAmpRotDeg, float InFrequency,
	EShakeNoise InMode, float InMixRatio)
{
	Duration = InDuration;
	AmplitudeLocation = InAmpLoc;
	AmplitudeRotationDegrees = InAmpRotDeg;
	Frequency = InFrequency;
	NoiseMode = InMode;
	MixRatio = std::clamp(InMixRatio, 0.0f, 1.0f);

	Time = 0.f;
	EnvelopeAlpha = 0.f;
}

inline float UCamMod_Shake::Sine2Pi(float X) { return std::sin(X * 6.28318530718f); }

float UCamMod_Shake::Fade(float T) { return T * T * T * (T * (T * 6 - 15) + 10); } // 6t^5 - 15t^4 + 10t^3

float UCamMod_Shake::Grad(int Hash, float X)
{
	// 1D Perlin: hash & 1 로 ±X
	return ((Hash & 1) ? -X : X);
}

float UCamMod_Shake::Perlin1D(float X)
{
	int Xi = (int)std::floor(X) & 255;
	float xf = X - std::floor(X);

	float u = Fade(xf);

	int a = Perm[Xi];
	int b = Perm[(Xi + 1) & 255];

	float g0 = Grad(a, xf);
	float g1 = Grad(b, xf - 1.0f);

	// 선형보간
	float val = g0 + u * (g1 - g0); // 범위 대략 [-1,1]
	return std::clamp(val, -1.0f, 1.0f);
}

float UCamMod_Shake::SampleChannel(float T, float Freq, float Phase, EShakeNoise Mode) const
{
	const float xSine = Sine2Pi(T * Freq + Phase);
	const float xPerlin = Perlin1D(T * Freq + Phase);

	switch (Mode)
	{
	case EShakeNoise::Sine:   return xSine;
	case EShakeNoise::Perlin: return xPerlin;
	case EShakeNoise::Mixed:
	default:
	{
		const float w = std::clamp(MixRatio, 0.0f, 1.0f); // 0=Sine, 1=Perlin
		return (1.0f - w) * xSine + w * xPerlin;
	}
	}
}

float UCamMod_Shake::EvalEnvelope01(float T) const
{
	if (Duration <= 0.f) // 무한(루프) → 인 램프만
	{
		if (BlendIn <= 0.f) return 1.f;
		float aIn = FMath::Clamp(T / BlendIn, 0.f, 1.f);
		return aIn * aIn * (3.f - 2.f * aIn); // 스무스스텝
	}

	float x = FMath::Clamp(T / Duration, 0.f, 1.f);
	float aIn = (BlendIn > 0.f) ? FMath::Clamp(x / BlendIn, 0.f, 1.f) : 1.f;
	float aOut = (BlendOut > 0.f) ? FMath::Clamp((1.f - x) / BlendOut, 0.f, 1.f) : 1.f;
	float a = FMath::Min(aIn, aOut);
	return a * a * (3.f - 2.f * a); // 스무스스텝
}

void UCamMod_Shake::ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo)
{
	if (!bEnabled || Weight <= 0.f) return;

	// TODO: 나중에 Slomo/HitStop 연동 시 PCM의 TimeDilation 곱하기
	const float Dt = DeltaTime;
	Time += Dt;

	EnvelopeAlpha = EvalEnvelope01(Time) * Weight;
	if (EnvelopeAlpha <= 1e-4f) return;

	const float Freq = std::max(0.01f, Frequency);

	// 채널별 샘플
	const float sx = SampleChannel(Time, Freq * 1.00f, PhaseX, NoiseMode);
	const float sy = SampleChannel(Time, Freq * 1.18f, PhaseY, NoiseMode);
	const float sz = SampleChannel(Time, Freq * 1.37f, PhaseZ, NoiseMode);
	const float sr = SampleChannel(Time, Freq * 0.77f, PhaseR, NoiseMode);

	// 로컬 공간 오프셋
	const FVector LocalLoc = FVector(sx, sy, sz) * (AmplitudeLocation * EnvelopeAlpha);
	const FVector LocalEulerDeg = FVector(sr, sr, sr) * (AmplitudeRotationDegrees * EnvelopeAlpha);

	// 카메라 로컬 → 월드
	const FQuat CamQ = ViewInfo->ViewRotation;
	const FVector WorldOffset = CamQ.RotateVector(LocalLoc);
	const FQuat   RotOffset = FQuat::MakeFromEulerZYX(LocalEulerDeg);

	ViewInfo->ViewLocation += WorldOffset;
	ViewInfo->ViewRotation = (RotOffset * ViewInfo->ViewRotation).GetNormalized();

	
}

void UCamMod_Shake::TickLifetime(float DeltaTime)
{
	// 유한 재생이면 종료 체크
	if (Duration > 0.f && Time >= Duration && EnvelopeAlpha <= 1e-3f)
	{
		bEnabled = false;
	}
}

void UCamMod_Shake::DrawImGui()
{
#ifdef IMGUI_VERSION
	if (ImGui::CollapsingHeader("CameraShake", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::DragFloat("Duration", &Duration, 0.01f, -1.0f, 10.0f);
		ImGui::DragFloat("BlendIn", &BlendIn, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("BlendOut", &BlendOut, 0.005f, 0.0f, 1.0f);
		ImGui::DragFloat("Frequency", &Frequency, 0.1f, 0.1f, 60.0f);
		ImGui::DragFloat("AmpLoc", &AmplitudeLocation, 0.1f, 0.0f, 20.0f);
		ImGui::DragFloat("AmpRotDeg", &AmplitudeRotationDegrees, 0.1f, 0.0f, 30.0f);

		int mode = (int)NoiseMode;
		const char* modes[] = { "Sine", "Perlin", "Mixed" };
		if (ImGui::Combo("NoiseMode", &mode, modes, 3))
			NoiseMode = (EShakeNoise)mode;

		if (NoiseMode == EShakeNoise::Mixed)
			ImGui::SliderFloat("MixRatio (Perlin)", &MixRatio, 0.0f, 1.0f);
	}
#endif
}
