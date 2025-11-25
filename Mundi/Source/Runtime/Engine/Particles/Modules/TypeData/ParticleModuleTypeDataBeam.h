#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "UParticleModuleTypeDataBeam.generated.h"

// 빔 소스/타겟 결정 방식
enum class EBeamMethod : uint8
{
    // 에미터 위치에서 시작
    Emitter,
    // 특정 액터/위치 지정
    Target,
    // 파티클 속도 방향으로
    Distance
};

// Beam 파티클 TypeData
// - 시작점과 끝점을 연결하는 빔 파티클
// - 레이저, 번개, 전기 효과 등에 사용
UCLASS(DisplayName="빔 타입 데이터", Description="시작점과 끝점을 연결하는 빔 파티클입니다")
class UParticleModuleTypeDataBeam : public UParticleModuleTypeDataBase
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModuleTypeDataBeam()
        : BeamMethod(EBeamMethod::Distance)
        , Segments(1)
        , Width(10.0f)
        , Length(100.0f)
        , TaperFactor(0.0f)
        , NoiseStrength(0.0f)
        , NoiseFrequency(1.0f)
    {}

    // 빔 방식 (현재는 Distance만 지원)
    UPROPERTY(EditAnywhere, Category="Beam")
    EBeamMethod BeamMethod;

    // 세그먼트 수 (1 = 직선, 많을수록 노이즈/곡선 표현 가능)
    UPROPERTY(EditAnywhere, Category="Beam")
    int32 Segments;

    // 빔 폭
    UPROPERTY(EditAnywhere, Category="Beam")
    float Width;

    // 빔 길이 (Distance 모드에서 사용)
    UPROPERTY(EditAnywhere, Category="Beam")
    float Length;

    // 테이퍼 (0 = 균일, 1 = 끝점에서 완전히 가늘어짐)
    UPROPERTY(EditAnywhere, Category="Beam")
    float TaperFactor;

    // 노이즈 강도 (0 = 직선)
    UPROPERTY(EditAnywhere, Category="Noise")
    float NoiseStrength;

    // 노이즈 주파수
    UPROPERTY(EditAnywhere, Category="Noise")
    float NoiseFrequency;
};
