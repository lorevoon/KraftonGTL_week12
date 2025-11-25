#pragma once

#include "ParticleModule.h"
#include "ParticleEventData.h"
#include "UParticleModuleEventGenerator.generated.h"

struct FParticleHitResult;
struct FParticleCollisionPayload;
struct FParticleEmitterInstance;

// 이벤트 생성 설정 구조체
struct FParticleEvent_GenerateInfo
{
	// 이벤트 타입 (현재는 Collision만 지원)
	EParticleEventType Type = EParticleEventType::Collision;

	// 이벤트 생성 빈도 (0 = 매번, N = N번째마다)
	int32 Frequency = 0;

	// 첫 번째 충돌만 이벤트 생성
	bool bFirstTimeOnly = false;

	// 마지막 충돌만 이벤트 생성 (MaxCollisions 도달 시)
	bool bLastTimeOnly = false;

	// 반사된 충돌 벡터 사용 (Direction에 반사 벡터 사용)
	bool bUseReflectedImpactVector = false;

	// 커스텀 이벤트 이름 (빈 문자열이면 "Collision" 사용)
	FString CustomName;

	FParticleEvent_GenerateInfo()
		: Type(EParticleEventType::Collision)
		, Frequency(0)
		, bFirstTimeOnly(false)
		, bLastTimeOnly(false)
		, bUseReflectedImpactVector(false)
	{
	}
};

// 인스턴스별 이벤트 트래킹 페이로드
struct FParticleEventInstancePayload
{
	// Collision 이벤트 존재 여부 (빠른 체크용)
	uint32 bCollisionEventsPresent : 1;

	// 전체 충돌 트래킹 카운터 (Frequency 필터용)
	int32 CollisionTrackingCount;

	FParticleEventInstancePayload()
		: bCollisionEventsPresent(0)
		, CollisionTrackingCount(0)
	{
	}
};

// 파티클 이벤트 생성 모듈
// - Collision 모듈과 연동하여 필터링된 이벤트 생성
// - LODLevel의 EventGenerator로 설정하여 사용
UCLASS(DisplayName="이벤트 생성기", Description="파티클 이벤트 생성 조건을 설정합니다")
class UParticleModuleEventGenerator : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleEventGenerator();

	// 이벤트 생성 설정 배열
	// - 여러 설정을 추가하여 다양한 조건의 이벤트 동시 생성 가능
	UPROPERTY(EditAnywhere, Category="Events")
	TArray<FParticleEvent_GenerateInfo> Events;

	// 인스턴스당 필요한 바이트 수
	virtual uint32 RequiredBytes() const override;

	// 충돌 이벤트 처리 (Collision 모듈에서 호출)
	// @param Owner 이미터 인스턴스
	// @param EventPayload 인스턴스 페이로드
	// @param CollisionPayload 충돌 페이로드 (CollisionCount 확인용)
	// @param HitResult 충돌 결과
	// @param Particle 파티클 데이터
	// @param CollideDirection 충돌 방향
	// @return 이벤트가 하나 이상 생성되었는지 여부
	bool HandleParticleCollision(
		FParticleEmitterInstance* Owner,
		FParticleEventInstancePayload* EventPayload,
		FParticleCollisionPayload* CollisionPayload,
		const FParticleHitResult& HitResult,
		FBaseParticle* Particle,
		const FVector& CollideDirection
	);
};
