#include "pch.h"
#include "ParticleModuleEventGenerator.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ParticleModuleCollision.h"
#include "ParticleCollisionQuery.h"

UParticleModuleEventGenerator::UParticleModuleEventGenerator()
{
	bSpawnModule = false;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "EventGenerator";
}

uint32 UParticleModuleEventGenerator::RequiredBytes() const
{
	return sizeof(FParticleEventInstancePayload);
}

bool UParticleModuleEventGenerator::HandleParticleCollision(
	FParticleEmitterInstance* Owner,
	FParticleEventInstancePayload* EventPayload,
	FParticleCollisionPayload* CollisionPayload,
	const FParticleHitResult& HitResult,
	FBaseParticle* Particle,
	const FVector& CollideDirection)
{
	if (!Owner || !Owner->Component || !EventPayload || !Particle)
	{
		return false;
	}

	// 충돌 트래킹 카운터 증가
	EventPayload->CollisionTrackingCount++;

	bool bEventGenerated = false;

	// 모든 이벤트 설정 순회
	for (const FParticleEvent_GenerateInfo& EventInfo : Events)
	{
		// Collision 타입만 처리
		if (EventInfo.Type != EParticleEventType::Collision)
		{
			continue;
		}

		// FirstTimeOnly 필터: 이미 충돌한 적 있으면 스킵
		// (Collided 플래그는 이 함수 호출 전에 설정됨, 하지만 CollisionCount로 체크)
		if (EventInfo.bFirstTimeOnly)
		{
			// CollisionCount가 1이면 첫 번째 충돌
			if (CollisionPayload && CollisionPayload->CollisionCount > 1)
			{
				continue;
			}
		}

		// LastTimeOnly 필터: 마지막 충돌이 아니면 스킵
		if (EventInfo.bLastTimeOnly)
		{
			// Dead 플래그가 설정되어 있으면 마지막 충돌
			if (!(Particle->Flags & EParticleFlags::Dead))
			{
				continue;
			}
		}

		// Frequency 필터: N번째 충돌마다 이벤트 생성
		if (EventInfo.Frequency > 0)
		{
			if ((EventPayload->CollisionTrackingCount % EventInfo.Frequency) != 0)
			{
				continue;
			}
		}

		// 이벤트 방향 계산
		FVector EventDirection = CollideDirection;
		if (EventInfo.bUseReflectedImpactVector)
		{
			// 반사 벡터 계산: V' = V - 2(V·N)N
			float DotProduct = FVector::Dot(CollideDirection, HitResult.ImpactNormal);
			EventDirection = CollideDirection - HitResult.ImpactNormal * 2.0f * DotProduct;
		}

		// 이벤트 이름 결정
		FString EventName = EventInfo.CustomName.empty() ? "Collision" : EventInfo.CustomName;

		// 이벤트 보고
		Owner->Component->ReportEventCollision(
			EventName,
			Owner->EmitterTime,
			Particle->RelativeTime,
			HitResult.ImpactPoint,
			Particle->Velocity,
			EventDirection,
			HitResult.ImpactNormal,
			""  // BoneName (현재 미사용)
		);

		bEventGenerated = true;
	}

	return bEventGenerated;
}
