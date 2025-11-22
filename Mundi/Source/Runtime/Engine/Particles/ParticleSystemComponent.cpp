#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleHelper.h"
#include "SceneView.h"

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
    if (bIsActive)
    {
        return;
    }

    CreateEmitterInstances();
    bIsActive = true;
}

void UParticleSystemComponent::Deactivate()
{
    bIsActive = false;
}

void UParticleSystemComponent::Stop()
{
    DestroyEmitterInstances();
    bIsActive = false;
}

void UParticleSystemComponent::Restart()
{
    Stop();
    Activate();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate)
{
    if (Template != NewTemplate)
    {
        DestroyEmitterInstances();
        Template = NewTemplate;

        if (bIsActive && Template)
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
        if (RequiredModule->EmitterType == EDynamicEmitterType::Sprite)
        {
            DynamicData = new FDynamicSpriteEmitterData(Instance);
        }
        else if (RequiredModule->EmitterType == EDynamicEmitterType::Mesh)
        {
            DynamicData = new FDynamicMeshEmitterData(Instance);
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
        return;
    }

    DestroyEmitterInstances();

    // Day 2에서 구현 예정
    // - Template의 각 Emitter에 대해 FParticleEmitterInstance 생성
    // - 메모리 풀 할당
    // - CurrentLODLevel 설정
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
