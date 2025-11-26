#include "pch.h"
#include "ParticleModuleSpawnPerUnit.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"
#include "ParticleTypes.h"
#include "Math.h"
#include <algorithm>

// Static map 정의
TMap<FParticleEmitterInstance*, FSpawnPerUnitInstancePayload> UParticleModuleSpawnPerUnit::InstanceDataMap;

UParticleModuleSpawnPerUnit::UParticleModuleSpawnPerUnit()
	: SpawnPerUnit(20.0f)
	, MaxFrameDistance(200)
	, bSpawnOnMovementStart(true)
{
	bSpawnModule = true;
	bUpdateModule = true;  // Update 모듈로 동작
	bFinalUpdateModule = false;
	ModuleName = "SpawnPerUnit";
}

FSpawnPerUnitInstancePayload* UParticleModuleSpawnPerUnit::GetInstancePayload(FParticleEmitterInstance* Owner)
{
	if (!Owner)
		return nullptr;

	// Map에서 찾아서 반환 (없으면 생성)
	if (!InstanceDataMap.Contains(Owner))
	{
		InstanceDataMap.Add(Owner, FSpawnPerUnitInstancePayload());
	}

	return &InstanceDataMap[Owner];
}

void UParticleModuleSpawnPerUnit::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (!Owner || !Owner->Component)
		return;

	FSpawnPerUnitInstancePayload* InstanceData = GetInstancePayload(Owner);
	if (!InstanceData)
		return;

	// 현재 컴포넌트 위치
	FVector CurrentLocation = Owner->GetComponentWorldLocation();

	// 첫 업데이트 시 초기화
	if (InstanceData->bFirstUpdate)
	{
		InstanceData->PreviousLocation = CurrentLocation;
		InstanceData->bFirstUpdate = false;

		// bSpawnOnMovementStart가 true면 초기 파티클 스폰
		if (bSpawnOnMovementStart)
		{
			SpawnTrailParticle(Owner, CurrentLocation, Owner->EmitterTime, InstanceData);
		}
		return;
	}

	// 이동 거리 계산
	FVector MovementDelta = CurrentLocation - InstanceData->PreviousLocation;
	float Distance = MovementDelta.Size();

	if (Distance > 0.001f)  // 최소 이동 거리
	{
		SpawnParticlesAlongMovement(Owner, MovementDelta, Distance, InstanceData);
	}

	InstanceData->PreviousLocation = CurrentLocation;
}

void UParticleModuleSpawnPerUnit::SpawnParticlesAlongMovement(
	FParticleEmitterInstance* Owner,
	const FVector& MovementDelta,
	float Distance,
	FSpawnPerUnitInstancePayload* InstanceData
	)
{
	InstanceData->AccumulatedDistance += Distance;

	// SpawnPerUnit 계산
	// 예: SpawnPerUnit=10, UnitScalar=2 → 2m당 10개
	float SpawnCount = (InstanceData->AccumulatedDistance) * SpawnPerUnit;

	if (SpawnCount >= 1.0f)
	{
		int32 ActualSpawnCount = std::min((int32)SpawnCount, MaxFrameDistance);

		// Hermite 보간용 Tangent 계산
		FVector CurrentLocation = Owner->GetComponentWorldLocation();
		FVector CurrentTangent = MovementDelta.GetSafeNormal() * Distance;
		FVector PreviousTangent = InstanceData->LastTangent;

		// 첫 이동 시 Tangent 초기화
		if (PreviousTangent.IsZero())
			PreviousTangent = CurrentTangent;

		// Hermite 보간으로 파티클 스폰
		for (int32 i = 0; i < ActualSpawnCount; ++i)
		{
			float Alpha = (float)(i + 1) / (float)ActualSpawnCount;

			// Hermite Cubic Interpolation
			float Alpha2 = Alpha * Alpha;
			float Alpha3 = Alpha2 * Alpha;
			float H1 = 2.0f * Alpha3 - 3.0f * Alpha2 + 1.0f;
			float H2 = Alpha3 - 2.0f * Alpha2 + Alpha;
			float H3 = -2.0f * Alpha3 + 3.0f * Alpha2;
			float H4 = Alpha3 - Alpha2;

			FVector SpawnLocation = InstanceData->PreviousLocation * H1 + PreviousTangent * H2 + CurrentLocation * H3 + CurrentTangent * H4;

			// SpawnTrailParticle이 HeadParticleDataIndex를 업데이트함
			SpawnTrailParticle(Owner, SpawnLocation, Owner->EmitterTime, InstanceData);
		}

		// Tangent 재계산 (HeadParticleDataIndex는 DataIndex)
		if (InstanceData->HeadParticleDataIndex != -1)
		{
			RecalculateTangents(Owner, InstanceData->HeadParticleDataIndex);
		}

		// 소모한 거리만큼 차감
		float ConsumedDistance = (ActualSpawnCount) / SpawnPerUnit;
		InstanceData->AccumulatedDistance -= ConsumedDistance;

		// Tangent 저장 (다음 프레임용)
		InstanceData->LastTangent = CurrentTangent;
	}
}

int32 UParticleModuleSpawnPerUnit::SpawnTrailParticle(FParticleEmitterInstance* Owner, const FVector& Location, float SpawnTime, FSpawnPerUnitInstancePayload* InstanceData)
{
	if (!Owner)
		return -1;

	// 파티클 풀이 꽉 찼으면 스폰 불가
	if (Owner->ActiveParticles >= Owner->MaxActiveParticles)
		return -1;

	// 새 파티클 인덱스 (ActiveParticles 증가 전)
	const int32 NewActiveIndex = Owner->ActiveParticles;

	// 파티클 메모리 계산 (일반 SpawnParticles와 동일한 방식)
	const int32 NewDataIndex = Owner->ParticleIndices[NewActiveIndex];
	FBaseParticle* Particle = reinterpret_cast<FBaseParticle*>(Owner->ParticleData + NewDataIndex * Owner->ParticleStride);

	// 전체 파티클 메모리를 0으로 초기화 (FBaseParticle + Payload 포함)
	std::memset(Particle, 0, Owner->ParticleStride);

	// 기본 초기화
	Particle->RelativeTime = 0.0f;
	Particle->OneOverMaxLifetime = 0.0f;
	Particle->Location = Location;
	Particle->OldLocation = Location;
	Particle->Flags = 0;

	// Ribbon 페이로드 가져오기 (DataIndex로 직접 접근)
	FRibbonTypeDataPayload* RibbonPayload = GetRibbonPayloadByDataIndex(Owner, NewDataIndex);
	if (RibbonPayload)
	{
		RibbonPayload->SpawnTime = SpawnTime;

		// 기존 HEAD가 있으면 연결 (새 파티클 → 기존 HEAD)
		if (InstanceData->HeadParticleDataIndex != -1)
		{
			FRibbonTypeDataPayload* OldHeadPayload = GetRibbonPayloadByDataIndex(Owner, InstanceData->HeadParticleDataIndex);
			if (OldHeadPayload)
			{
				// 기존 HEAD → MIDDLE로 변경
				OldHeadPayload->Flags &= ~ETrailParticleFlags::Head;
				OldHeadPayload->Flags |= ETrailParticleFlags::Middle;
			}

			// 새 파티클의 Next를 기존 HEAD DataIndex로 설정
			RibbonPayload->Next = InstanceData->HeadParticleDataIndex;
		}
		else
		{
			// 첫 파티클이면 Next 없음 (TAIL)
			RibbonPayload->Next = -1;
		}

		// 새 파티클을 HEAD로 설정 (DataIndex 저장!)
		RibbonPayload->Flags = ETrailParticleFlags::Head;
		InstanceData->HeadParticleDataIndex = NewDataIndex;
	}

	// 다른 Spawn 모듈들 실행 (Lifetime, Size, Color 등)
	if (Owner->CurrentLODLevel)
	{
		for (UParticleModule* Module : Owner->CurrentLODLevel->Modules)
		{
			if (Module && Module != this && Module->bSpawnModule)
			{
				// Offset은 논리 인덱스 (ParticleIndices의 인덱스)
				int32 Offset = NewActiveIndex;
				Module->Spawn(Owner, Offset, SpawnTime, Particle);
			}
		}
	}

	// OldLocation 최종 설정
	Particle->OldLocation = Particle->Location;

	// ActiveParticles 증가 (일반 SpawnParticles와 동일하게 마지막에)
	++Owner->ActiveParticles;
	++Owner->ParticleCounter;

	return NewActiveIndex;
}

void UParticleModuleSpawnPerUnit::RecalculateTangents(FParticleEmitterInstance* Owner, int32 HeadDataIndex)
{
	if (!Owner || HeadDataIndex < 0 || HeadDataIndex >= Owner->MaxActiveParticles)
		return;

	int32 CurrentDataIndex = HeadDataIndex;
	FVector PrevLocation = FVector::Zero();
	bool bHasPrev = false;

	// HEAD부터 TAIL까지 순회 (DataIndex로 순회)
	while (CurrentDataIndex != -1)
	{
		// DataIndex로 직접 접근
		uint8* CurrentBytes = Owner->ParticleData + CurrentDataIndex * Owner->ParticleStride;
		FBaseParticle* CurrentParticle = (FBaseParticle*)CurrentBytes;
		FRibbonTypeDataPayload* CurrentPayload = GetRibbonPayloadByDataIndex(Owner, CurrentDataIndex);

		if (!CurrentPayload)
			break;

		// Next 파티클 가져오기 (Next는 DataIndex)
		int32 NextDataIndex = CurrentPayload->Next;

		if (NextDataIndex != -1 && NextDataIndex < Owner->MaxActiveParticles)
		{
			uint8* NextBytes = Owner->ParticleData + NextDataIndex * Owner->ParticleStride;
			FBaseParticle* NextParticle = (FBaseParticle*)NextBytes;

			// Tangent = 다음 파티클 방향
			FVector TangentVec = NextParticle->Location - CurrentParticle->Location;
			float TangentLength = TangentVec.Size();
			if (TangentLength > 0.001f)
			{
				CurrentPayload->Tangent = TangentVec / TangentLength;
			}
		}
		else
		{
			// TAIL 파티클 - 이전 파티클에서 오는 방향 사용
			if (bHasPrev)
			{
				FVector TangentVec = CurrentParticle->Location - PrevLocation;
				float TangentLength = TangentVec.Size();
				if (TangentLength > 0.001f)
				{
					CurrentPayload->Tangent = TangentVec / TangentLength;
				}
			}
			else
			{
				// 이전 파티클도 없으면 Velocity 방향 사용
				float VelLength = CurrentParticle->Velocity.Size();
				if (VelLength > 0.001f)
				{
					CurrentPayload->Tangent = CurrentParticle->Velocity / VelLength;
				}
			}
		}

		PrevLocation = CurrentParticle->Location;
		bHasPrev = true;
		CurrentDataIndex = NextDataIndex;
	}
}

FRibbonTypeDataPayload* UParticleModuleSpawnPerUnit::GetRibbonPayloadByDataIndex(FParticleEmitterInstance* Owner, int32 DataIndex)
{
	if (!Owner || DataIndex < 0 || DataIndex >= Owner->MaxActiveParticles)
		return nullptr;

	// DataIndex로 직접 메모리 접근
	uint8* ParticleBytes = Owner->ParticleData + DataIndex * Owner->ParticleStride;

	// FBaseParticle은 128바이트로 16바이트 정렬됨
	FRibbonTypeDataPayload* Payload = (FRibbonTypeDataPayload*)(ParticleBytes + sizeof(FBaseParticle));
	return Payload;
}

void UParticleModuleSpawnPerUnit::UpdateHeadOnKill(FParticleEmitterInstance* Owner, int32 DyingDataIndex, int32 DyingNext)
{
	FSpawnPerUnitInstancePayload* InstanceData = GetInstancePayload(Owner);
	if (!InstanceData)
		return;

	// HEAD가 죽는 경우 Next를 새 HEAD로
	if (InstanceData->HeadParticleDataIndex == DyingDataIndex)
	{
		InstanceData->HeadParticleDataIndex = DyingNext;
	}
}
