#pragma once

#include "Vector.h"
#include "Picking.h"

// Forward declarations
class AActor;
class UPrimitiveComponent;
class UWorld;
struct FAABB;
struct FOBB;
struct FBoundingSphere;

// 파티클 충돌 결과 구조체
// - Ray cast 결과를 담는 구조체
// - 충돌 지점, 노말, 거리 등 정보 포함
struct FParticleHitResult
{
	// 충돌 여부
	bool bHit = false;

	// 충돌한 액터 (nullable)
	AActor* HitActor = nullptr;

	// 충돌한 컴포넌트 (nullable)
	UPrimitiveComponent* HitComponent = nullptr;

	// 충돌 지점 (월드 좌표)
	FVector ImpactPoint = FVector::Zero();

	// 충돌 노말 (정규화됨, 반사 계산용)
	FVector ImpactNormal = FVector(0.0f, 0.0f, 1.0f);

	// 레이 시작점에서 충돌 지점까지의 거리
	float Distance = -1.0f;

	// 결과 초기화
	void Reset()
	{
		bHit = false;
		HitActor = nullptr;
		HitComponent = nullptr;
		ImpactPoint = FVector::Zero();
		ImpactNormal = FVector(0.0f, 0.0f, 1.0f);
		Distance = -1.0f;
	}
};

// 파티클 충돌 쿼리 네임스페이스
// - WorldPartitionManager를 활용한 레이 캐스트 헬퍼 함수들
namespace ParticleCollision
{
	// 단일 레이 캐스트
	// - Start에서 End까지 레이를 쏴서 첫 번째 충돌 검출
	// @param World 쿼리 대상 월드
	// @param Start 레이 시작점 (월드 좌표)
	// @param End 레이 끝점 (월드 좌표)
	// @param OutHit 충돌 결과 (출력)
	// @return 충돌 여부
	bool LineTraceSingle(
		UWorld* World,
		const FVector& Start,
		const FVector& End,
		FParticleHitResult& OutHit
	);

	// AABB에서 충돌 노말 계산
	// @param Box AABB 바운딩 볼륨
	// @param ImpactPoint 충돌 지점
	// @return 충돌 면의 노말 벡터
	FVector CalculateAABBNormal(const FAABB& Box, const FVector& ImpactPoint);

	// OBB에서 충돌 노말 계산
	// @param Box OBB 바운딩 볼륨
	// @param ImpactPoint 충돌 지점
	// @return 충돌 면의 노말 벡터
	FVector CalculateOBBNormal(const FOBB& Box, const FVector& ImpactPoint);

	// BoundingSphere에서 충돌 노말 계산
	// @param Sphere 바운딩 스피어
	// @param ImpactPoint 충돌 지점
	// @return 충돌 면의 노말 벡터 (구 중심에서 충돌점 방향)
	FVector CalculateSphereNormal(const FBoundingSphere& Sphere, const FVector& ImpactPoint);
}
