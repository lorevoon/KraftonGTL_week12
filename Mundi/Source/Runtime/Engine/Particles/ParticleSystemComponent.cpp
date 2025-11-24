#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleHelper.h"
#include "SceneView.h"
#include "Modules/TypeData/ParticleModuleTypeDataBase.h"
#include "Modules/TypeData/ParticleModuleTypeDataMesh.h"

UParticleSystemComponent::UParticleSystemComponent()
    : Template(nullptr)
    , bAutoActivate(true)
    , bIsActive(false)
{
}

UParticleSystemComponent::~UParticleSystemComponent()
{
    DestroyEmitterInstances();
}

void UParticleSystemComponent::OnRegister(UWorld* InWorld)
{
    UPrimitiveComponent::OnRegister(InWorld);

    if (Template && bAutoActivate)
    {
		CreateEmitterInstances();
        Activate();
    }
}

void UParticleSystemComponent::OnUnregister()
{
    DestroyEmitterInstances();
    UPrimitiveComponent::OnUnregister();
}

void UParticleSystemComponent::Activate()
{
    if (!Template || bIsActive)
        return;

	// 템플릿 설정만 되어 있고 인스턴스가 없으면 생성
    if(Template && EmitterInstances.Num() == 0)
    {
        CreateEmitterInstances();
	}

    bIsActive = true;
}

void UParticleSystemComponent::Deactivate()
{
    bIsActive = false;
}

void UParticleSystemComponent::Stop()
{
    Deactivate();
    ResetInstances();
}

void UParticleSystemComponent::Restart()
{
	Stop(); // Deactivate + ResetInstances
    Activate();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate)
{
    if (Template != NewTemplate)
    {
        DestroyEmitterInstances();
        Template = NewTemplate;

        if (Template)
        {
            CreateEmitterInstances();
        }
    }
}

void UParticleSystemComponent::UpdateParticles(float DeltaTime)
{
    if (!bIsActive || !Template)
    {
        return;
    }

    // Day 2에서 구현 예정
    // - 각 이미터 인스턴스 업데이트
    // - 파티클 스폰, 업데이트, 제거
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            UpdateEmitterInstance(Instance, DeltaTime);
        }
    }
}

TArray<FDynamicEmitterDataBase*> UParticleSystemComponent::GetRenderData(FSceneView* View)
{
    TArray<FDynamicEmitterDataBase*> RenderDataArray;

    if (!bIsActive || !Template)
    {
        return RenderDataArray;
    }

    // 각 이미터 인스턴스의 렌더 데이터 수집
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (!Instance || !Instance->CurrentLODLevel || !Instance->CurrentLODLevel->RequiredModule)
            continue;

        UParticleModuleRequired* RequiredModule = Instance->CurrentLODLevel->RequiredModule;

        // Material이 없으면 렌더링 불가
        if (!RequiredModule->Material)
            continue;

        // 파티클이 없으면 스킵
        if (Instance->ActiveParticles == 0)
            continue;

        // 이미터 정렬 (필요한 경우)
        Instance->Sort(RequiredModule->SortMode, &View->ViewLocation);

        // 타입별 DynamicData 생성 (매 프레임 생성, 렌더러에서 삭제)
        FDynamicEmitterDataBase* DynamicData = nullptr;
        UParticleLODLevel* LODLevel = Instance->CurrentLODLevel;

        if (LODLevel->TypeDataModule == nullptr)
        {
            // TypeData가 없으면 기본 Sprite
            DynamicData = new FDynamicSpriteEmitterData(Instance);
        }
        else if (Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule))
        {
            // Mesh TypeData
            DynamicData = new FDynamicMeshEmitterData(Instance);
        }
        else
        {
            continue;
        }

        if (DynamicData)
        {
            DynamicData->UpdateRenderData(View);
            RenderDataArray.Add(DynamicData);
        }
    }

    return RenderDataArray;
}

void UParticleSystemComponent::CreateEmitterInstances()
{
    if (!Template)
    {
		UE_LOG("WARNING: UParticleSystemComponent::CreateEmitterInstances called with null Template. Ensure UParticleSystem Template is set.");
        return;
    }

	// Ensure previous instances are cleared
    DestroyEmitterInstances();

    for (int32 i = 0; i < Template->GetEmitterCount(); ++i)
    {
        UParticleEmitter* EmitterTemplate = Template->GetEmitter(i);
        if (!EmitterTemplate)
        {
            UE_LOG("WARNING: Emitter %d is null", i);
            continue;
        }

        // LOD0 필요
        UParticleLODLevel* LOD0 = EmitterTemplate->GetLODLevel(0);
        if (!LOD0)
        {
			UE_LOG("WARNING: Emitter %d has no valid LOD0.", i);
            continue;
        }
		if (!LOD0->RequiredModule)
        {
            UE_LOG("WARNING: Emitter %d has no valid RequiredModule", i);
            continue;
        }

        if (LOD0->TypeDataModule)
        {
            if (UParticleModuleTypeDataMesh* MeshType = Cast<UParticleModuleTypeDataMesh>(LOD0->TypeDataModule))
            {
                if (!MeshType->Mesh)
                {
                    UE_LOG("WARNING: Emitter %d has Mesh TypeData but no mesh assigned.", i);
                }
            }
        }

        // Max 파티클 결정: Emitter 설정 우선
        const int32 MaxParticles = (EmitterTemplate->MaxParticleCount > 0)
            ? EmitterTemplate->MaxParticleCount
            : 0;
        if (MaxParticles <= 0)
        {
            continue;
        }

        FParticleEmitterInstance* Instance = new FParticleEmitterInstance();
        Instance->Initialize(EmitterTemplate, this, 0, MaxParticles);
        EmitterInstances.Add(Instance);
	}
}

void UParticleSystemComponent::DestroyEmitterInstances()
{
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            delete Instance;
        }
    }
    EmitterInstances.Empty();
}

void UParticleSystemComponent::ResetInstances()
{
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            Instance->Reset();
        }
    }
}

void UParticleSystemComponent::UpdateEmitterInstance(FParticleEmitterInstance* Instance, float DeltaTime)
{
    // Day 2에서 구현 예정
    // 1. SpawnParticles()
    // 2. UpdateParticles()
    // 3. KillDeadParticles()
}

void UParticleSystemComponent::SpawnParticles(FParticleEmitterInstance* Instance, float DeltaTime)
{
    // Day 2에서 구현 예정
    // - SpawnRate 기반 스폰 계산
    // - SpawnModules 실행
}

void UParticleSystemComponent::KillDeadParticles(FParticleEmitterInstance* Instance)
{
    // Day 2에서 구현 예정
    // - RelativeTime >= 1.0인 파티클 제거
    // - ParticleIndices 배열 재정렬
}
