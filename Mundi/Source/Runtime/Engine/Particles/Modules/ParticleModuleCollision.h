#pragma once

#include "ParticleModule.h"
#include "ParticleTypes.h"
#include "UParticleModuleCollision.generated.h"

struct FParticleHitResult;

// 파티클 충돌 모듈
// - Update 단계에서 파티클의 이전 위치와 현재 위치 사이에 Line Trace 수행
// - 충돌 시 반응 타입에 따라 Kill/Bounce/Stop 처리
// - 충돌 이벤트 생성 (Phase 2에서 구현 예정)
UCLASS(DisplayName="충돌 모듈", Description="파티클 충돌 검사 및 반응을 처리합니다")
class UParticleModuleCollision : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

	UParticleModuleCollision();

	// Update 단계에서 충돌 검사 수행
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) override;

	// 충돌 반응 타입 (Kill/Bounce/Stop)
	UPROPERTY(EditAnywhere, Category="Collision")
	EParticleCollisionResponse CollisionResponse;

	// 반사 계수 (0.0 ~ 1.0)
	// - 1.0: 완전 탄성 충돌 (에너지 손실 없음)
	// - 0.0: 완전 비탄성 충돌 (모든 에너지 흡수)
	UPROPERTY(EditAnywhere, Category="Collision")
	float Resilience;

	// 마찰 계수 (0.0 ~ 1.0)
	// - 충돌 면에 평행한 속도 성분 감소 비율
	UPROPERTY(EditAnywhere, Category="Collision")
	float Friction;

	// 충돌 활성화 여부
	UPROPERTY(EditAnywhere, Category="Collision")
	bool bCollisionEnabled;

	// 최대 충돌 횟수 (0 = 무제한)
	// - 지정 횟수 초과 시 파티클 제거
	UPROPERTY(EditAnywhere, Category="Collision")
	int32 MaxCollisions;

	// 페이로드로 저장할 충돌 횟수를 위한 추가 바이트
	virtual uint32 RequiredBytes() const override;

private:
	// 단일 파티클에 대한 충돌 검사 수행
	// @param Owner 이미터 인스턴스
	// @param ActiveIndex 활성 파티클 인덱스
	// @param DeltaTime 프레임 델타 타임
	// @return 충돌 발생 여부
	bool PerformCollisionCheck(FParticleEmitterInstance* Owner, int32 ActiveIndex, float DeltaTime);

	// 충돌 반응 적용
	// @param Particle 파티클 포인터
	// @param HitResult 충돌 결과
	// @param CollisionCount 현재 충돌 횟수 (페이로드)
	void ApplyCollisionResponse(FBaseParticle* Particle, const FParticleHitResult& HitResult, int32& CollisionCount);

	// 반사 벡터 계산
	// @param Velocity 입사 속도
	// @param Normal 충돌 면 노말
	// @return 반사된 속도 벡터
	FVector CalculateReflectedVelocity(const FVector& Velocity, const FVector& Normal) const;
};

// 충돌 모듈 페이로드 구조체
// - FBaseParticle 뒤에 저장되는 추가 데이터
struct FParticleCollisionPayload
{
	int32 CollisionCount;  // 현재까지의 충돌 횟수

	FParticleCollisionPayload()
		: CollisionCount(0)
	{
	}
};
