#pragma once

#include "ParticleModule.h"
#include "Vector.h"
#include "UParticleModuleSpawnPerUnit.generated.h"

// SpawnPerUnit 인스턴스 데이터
// - 각 EmitterInstance마다 별도로 유지되는 데이터
struct FSpawnPerUnitInstancePayload
{
	FVector PreviousLocation;    // 이전 프레임 컴포넌트 위치
	float AccumulatedDistance;   // 누적 이동 거리
	bool bFirstUpdate;           // 첫 업데이트 여부
	int32 HeadParticleDataIndex; // HEAD 파티클 DataIndex (ParticleData 배열의 실제 인덱스, -1이면 없음)

	FSpawnPerUnitInstancePayload()
		: PreviousLocation(FVector::Zero())
		, AccumulatedDistance(0.0f)
		, bFirstUpdate(true)
		, HeadParticleDataIndex(-1)
	{
	}
};

// SpawnPerUnit 모듈
// - Trail/Ribbon 파티클 전용
// - 오브젝트가 이동한 거리에 따라 자동으로 파티클 스폰
// - 시간 기반 스폰이 아닌 거리 기반 스폰
UCLASS(DisplayName="단위당 스폰", Description="오브젝트 이동 거리에 따라 파티클을 스폰합니다 (Trail 전용)")
class UParticleModuleSpawnPerUnit : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleSpawnPerUnit();

	// 단위 거리당 스폰 개수
	// 예: 10.0 = 거리 단위당 10개 파티클
	UPROPERTY(EditAnywhere, Category="Spawn")
	float SpawnPerUnit;

	// 거리 스케일
	// UnitScalar만큼 이동 시 SpawnPerUnit개 스폰 (1유닛 = 1m)
	UPROPERTY(EditAnywhere, Category="Spawn")
	float UnitScalar;

	// 최대 프레임당 스폰 개수 (무한 스폰 방지)
	UPROPERTY(EditAnywhere, Category="Spawn")
	int32 MaxFrameDistance;

	// 처음 움직일 때 초기 파티클 스폰 여부
	UPROPERTY(EditAnywhere, Category="Spawn")
	bool bSpawnOnMovementStart;

	// Spawn 함수는 사용 안 함
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) override {}

	// Update에서 거리 기반 스폰 처리
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

	// KillParticle 시 HeadParticleDataIndex 업데이트
	void UpdateHeadOnKill(FParticleEmitterInstance* Owner, int32 DyingDataIndex, int32 DyingNext);

private:
	// EmitterInstance별 인스턴스 데이터 저장 (static map)
	static TMap<FParticleEmitterInstance*, FSpawnPerUnitInstancePayload> InstanceDataMap;

	// 인스턴스 데이터 가져오기
	FSpawnPerUnitInstancePayload* GetInstancePayload(FParticleEmitterInstance* Owner);

	// 이동 경로를 따라 파티클 스폰
	void SpawnParticlesAlongMovement(FParticleEmitterInstance* Owner, const FVector& MovementDelta, float Distance, FSpawnPerUnitInstancePayload* InstanceData);

	// Trail 파티클 스폰
	int32 SpawnTrailParticle(FParticleEmitterInstance* Owner, const FVector& Location, float SpawnTime, FSpawnPerUnitInstancePayload* InstanceData);

	// Tangent 재계산 (HeadDataIndex는 DataIndex)
	void RecalculateTangents(FParticleEmitterInstance* Owner, int32 HeadDataIndex);

	// Ribbon 페이로드 가져오기 (DataIndex로 접근)
	struct FRibbonTypeDataPayload* GetRibbonPayloadByDataIndex(FParticleEmitterInstance* Owner, int32 DataIndex);
};
