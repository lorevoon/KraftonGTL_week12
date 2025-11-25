#include "pch.h"
#include "ParticleModuleCollision.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ParticleHelper.h"
#include "ParticleCollisionQuery.h"
#include "World.h"

UParticleModuleCollision::UParticleModuleCollision()
	: CollisionResponse(EParticleCollisionResponse::Bounce)
	, Resilience(0.5f)
	, Friction(0.2f)
	, bCollisionEnabled(true)
	, MaxCollisions(0)
{
	bSpawnModule = false;
	bUpdateModule = true;
	bFinalUpdateModule = false;
	ModuleName = "Collision";
}

uint32 UParticleModuleCollision::RequiredBytes() const
{
	return sizeof(FParticleCollisionPayload);
}

void UParticleModuleCollision::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (!bCollisionEnabled || !Owner || !Owner->Component)
	{
		return;
	}

	// 모든 활성 파티클에 대해 충돌 검사 수행
	for (int32 i = Owner->ActiveParticles - 1; i >= 0; --i)
	{
		PerformCollisionCheck(Owner, i, DeltaTime);
	}
}

bool UParticleModuleCollision::PerformCollisionCheck(FParticleEmitterInstance* Owner, int32 ActiveIndex, float DeltaTime)
{
	FBaseParticle* Particle = Owner->GetParticle(ActiveIndex);
	if (!Particle)
	{
		return false;
	}

	// 이미 충돌 무시 플래그가 설정된 파티클은 스킵
	if (Particle->Flags & EParticleFlags::IgnoreCollision)
	{
		return false;
	}

	// 충돌 페이로드 접근
	// FBaseParticle 뒤에 저장된 페이로드 데이터
	uint8* ParticleBytes = reinterpret_cast<uint8*>(Particle);
	FParticleCollisionPayload* Payload = reinterpret_cast<FParticleCollisionPayload*>(
		ParticleBytes + sizeof(FBaseParticle)
	);

	// 로컬 공간 → 월드 공간 변환
	FVector WorldOldLocation = Particle->OldLocation;
	FVector WorldLocation = Particle->Location;

	if (Owner->UseLocalSpace())
	{
		FVector ComponentLocation = Owner->GetComponentWorldLocation();
		WorldOldLocation = WorldOldLocation + ComponentLocation;
		WorldLocation = WorldLocation + ComponentLocation;
	}

	// 월드 가져오기
	UWorld* World = Owner->Component->GetWorld();
	if (!World)
	{
		return false;
	}

	// Line Trace 수행 (이전 위치 → 현재 위치)
	FParticleHitResult HitResult;
	bool bHit = ParticleCollision::LineTraceSingle(World, WorldOldLocation, WorldLocation, HitResult);

	if (bHit)
	{
		// 충돌 플래그 설정
		Particle->Flags |= EParticleFlags::Collided;

		// 충돌 반응 적용
		ApplyCollisionResponse(Particle, HitResult, Payload->CollisionCount);

		// 최대 충돌 횟수 체크
		if (MaxCollisions > 0 && Payload->CollisionCount >= MaxCollisions)
		{
			// 충돌 횟수 초과 - 파티클 제거 예약
			Particle->Flags |= EParticleFlags::Dead;
			Particle->RelativeTime = 1.0f; // KillDeadParticles에서 제거됨
		}

		return true;
	}

	return false;
}

void UParticleModuleCollision::ApplyCollisionResponse(FBaseParticle* Particle, const FParticleHitResult& HitResult, int32& CollisionCount)
{
	// 충돌 횟수 증가
	++CollisionCount;

	switch (CollisionResponse)
	{
	case EParticleCollisionResponse::Kill:
		// 파티클 즉시 제거
		Particle->Flags |= EParticleFlags::Dead;
		Particle->RelativeTime = 1.0f;
		break;

	case EParticleCollisionResponse::Bounce:
		{
			// 반사 벡터 계산 및 적용
			FVector ReflectedVelocity = CalculateReflectedVelocity(Particle->Velocity, HitResult.ImpactNormal);
			Particle->Velocity = ReflectedVelocity;
			Particle->BaseVelocity = ReflectedVelocity;

			// 충돌 지점으로 위치 조정 (약간의 오프셋 추가하여 재충돌 방지)
			Particle->Location = HitResult.ImpactPoint + HitResult.ImpactNormal * 0.1f;
		}
		break;

	case EParticleCollisionResponse::Stop:
		// 속도를 0으로 설정
		Particle->Velocity = FVector::Zero();
		Particle->BaseVelocity = FVector::Zero();

		// 충돌 지점에 정지
		Particle->Location = HitResult.ImpactPoint + HitResult.ImpactNormal * 0.1f;

		// 이후 충돌 검사 무시
		Particle->Flags |= EParticleFlags::IgnoreCollision;
		break;
	}
}

FVector UParticleModuleCollision::CalculateReflectedVelocity(const FVector& Velocity, const FVector& Normal) const
{
	// 입사 속도를 노말 방향(수직)과 접선 방향(수평)으로 분해
	float NormalComponent = FVector::Dot(Velocity, Normal);

	// 노말 방향 속도 (표면으로 향하는 성분)
	FVector VelocityNormal = Normal * NormalComponent;

	// 접선 방향 속도 (표면에 평행한 성분)
	FVector VelocityTangent = Velocity - VelocityNormal;

	// 반사 계산:
	// - 노말 방향: 방향 반전 + Resilience 계수 적용
	// - 접선 방향: Friction 계수 적용
	FVector ReflectedNormal = -VelocityNormal * Resilience;
	FVector ReflectedTangent = VelocityTangent * (1.0f - Friction);

	return ReflectedNormal + ReflectedTangent;
}
