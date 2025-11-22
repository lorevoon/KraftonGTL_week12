#include "pch.h"
#include "ParticleSystemComponent.h"
#include "ParticleSystem.h"
#include "ParticleEmitter.h"
#include "ParticleLODLevel.h"
#include "ParticleModule.h"
#include "ParticleHelper.h"

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

 TArray<FParticleEmitterInstance*> UParticleSystemComponent::GetParticleData()
{
    return EmitterInstances;
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
