#pragma once

#include "ParticleModuleTypeDataBase.h"
#include "ParticleTypes.h"
#include "UParticleModuleTypeDataRibbon.generated.h"

// 렌더 축 (리본 방향)
enum class ERibbonRenderAxis : uint8
{
	CameraUp,      // 카메라 방향 (빌보드)
	SourceUp,      // 소스 업 벡터 사용
	WorldUp        // 월드 Z축
};

// 리본 파티클 TypeData
// - 오브젝트 움직임을 따라가는 Trail/Ribbon 파티클
// - SpawnPerUnit 모듈 필수
UCLASS(DisplayName="리본 타입 데이터", Description="오브젝트 궤적을 따라가는 리본 파티클입니다")
class UParticleModuleTypeDataRibbon : public UParticleModuleTypeDataBase
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleTypeDataRibbon();

	// 최대 트레일 개수 (멀티 소스 시)
	UPROPERTY(EditAnywhere, Category="Trail")
	int32 MaxTrailCount;

	// 트레일당 최대 파티클 수
	UPROPERTY(EditAnywhere, Category="Trail")
	int32 MaxParticleInTrailCount;
	
	// 리본 너비
	UPROPERTY(EditAnywhere, Category="Ribbon")
	float Width;

	// 렌더 축 (리본 방향)
	UPROPERTY(EditAnywhere, Category="Ribbon")
	ERibbonRenderAxis RenderAxis;

	// UV 타일링 거리 (0 = 비활성화)
	UPROPERTY(EditAnywhere, Category="Rendering")
	float TilingDistance;
	

	// 파티클당 추가 바이트 (FRibbonTypeDataPayload)
	virtual uint32 RequiredBytes() const override
	{
		return sizeof(FRibbonTypeDataPayload);
	}

	// Emitter 타입 반환
	virtual EDynamicEmitterType GetDynamicEmitterType() const
	{
		return EDynamicEmitterType::Ribbon;
	}

	// 파티클이 죽을 때 Trail 체인에서 제거
	virtual void OnParticleKilled(FParticleEmitterInstance* Owner, int32 DyingDataIndex) override;
};
