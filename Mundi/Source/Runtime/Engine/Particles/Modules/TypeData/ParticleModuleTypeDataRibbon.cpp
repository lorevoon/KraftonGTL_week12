#include "pch.h"
#include "ParticleModuleTypeDataRibbon.h"
#include "ParticleEmitterInstance.h"
#include "ParticleTypes.h"
#include "Modules/ParticleModuleSpawnPerUnit.h"

UParticleModuleTypeDataRibbon::UParticleModuleTypeDataRibbon()
	: MaxTrailCount(1)
	, MaxParticleInTrailCount(2000)
	, Width(0.5f)
	, RenderAxis(ERibbonRenderAxis::CameraUp)
	, TilingDistance(5.0f)
{
	bSpawnModule = false;
	bUpdateModule = false;
	bFinalUpdateModule = false;
	ModuleName = "Ribbon";
}

void UParticleModuleTypeDataRibbon::OnParticleKilled(FParticleEmitterInstance* Owner, int32 DyingDataIndex)
{
	if (!Owner)
		return;

	// 죽는 파티클의 페이로드 가져오기
	uint8* DyingBytes = Owner->ParticleData + DyingDataIndex * Owner->ParticleStride;
	FRibbonTypeDataPayload* DyingPayload = (FRibbonTypeDataPayload*)(DyingBytes + sizeof(FBaseParticle));
	int32 DyingNext = DyingPayload->Next;

	// Trail 체인에서 죽는 파티클 제거 (이전 파티클의 Next를 DyingNext로)
	for (int32 i = 0; i < Owner->ActiveParticles; ++i)
	{
		int32 DataIndex = Owner->ParticleIndices[i];
		if (DataIndex == DyingDataIndex) continue; // 자기 자신 제외

		uint8* ParticleBytes = Owner->ParticleData + DataIndex * Owner->ParticleStride;
		FRibbonTypeDataPayload* Payload = (FRibbonTypeDataPayload*)(ParticleBytes + sizeof(FBaseParticle));

		// 죽는 파티클을 가리키는 파티클 찾기
		if (Payload->Next == DyingDataIndex)
		{
			Payload->Next = DyingNext;  // 죽는 파티클 건너뛰기
			break;
		}
	}

	// 죽는 파티클의 Next를 -1로 초기화 (순환 참조 방지)
	DyingPayload->Next = -1;

	// SpawnPerUnit 모듈의 HeadParticleDataIndex 업데이트
	if (Owner->CurrentLODLevel)
	{
		for (UParticleModule* Module : Owner->CurrentLODLevel->Modules)
		{
			if (UParticleModuleSpawnPerUnit* SpawnPerUnit = Cast<UParticleModuleSpawnPerUnit>(Module))
			{
				SpawnPerUnit->UpdateHeadOnKill(Owner, DyingDataIndex, DyingNext);
				break;
			}
		}
	}
}
