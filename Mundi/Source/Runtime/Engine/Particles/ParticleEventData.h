#pragma once

#include "Vector.h"

// 파티클 이벤트 타입
enum class EParticleEventType : uint8
{
	Any = 0,        // 모든 이벤트 (필터링용)
	Spawn = 1,      // 파티클 스폰
	Death = 2,      // 파티클 사망
	Collision = 3,  // 파티클 충돌
	Burst = 4       // Burst 스폰
};

// 충돌 이벤트 데이터 구조체
// - 충돌 발생 시 저장되는 이벤트 정보
// - ParticleSystemComponent의 CollisionEvents 배열에 저장
// - 프레임 종료 시 델리게이트로 브로드캐스트
struct FParticleEventCollideData
{
	// 이벤트 타입
	EParticleEventType Type = EParticleEventType::Collision;

	// 이벤트 이름 (필터링용, Phase 4에서 활용)
	FString EventName;

	// 이미터 시간 (이벤트 발생 시점)
	float EmitterTime = 0.0f;

	// 파티클 생명 시간 (0.0 ~ 1.0)
	float ParticleTime = 0.0f;

	// 충돌 위치 (월드 좌표)
	FVector Location = FVector::Zero();

	// 파티클 속도
	FVector Velocity = FVector::Zero();

	// 파티클 이동 방향
	FVector Direction = FVector::Zero();

	// 충돌 면 노말
	FVector Normal = FVector::Zero();

	// 충돌한 본 이름 (Skeletal Mesh용, 현재 미사용)
	FString BoneName;

	FParticleEventCollideData() = default;
};
