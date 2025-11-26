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
#include "Modules/TypeData/ParticleModuleTypeDataBeam.h"
#include "World.h"
#include "PlayerCameraManager.h"
#include "CameraActor.h"
#include "CameraComponent.h"
#include "PlatformTime.h"

UParticleSystemComponent::UParticleSystemComponent()
    : Template(nullptr)
    , bAutoActivate(true)
    , bIsActive(false)
    , SystemTime(0.0f)
    , CompletedLoops(0)
    , CustomPlaybackRate(1.0f)
    , LODDistanceCheckTime(0.25f)
    , LODMethod(ParticleSystemLODMethod::Automatic)
	, AccumLODDistanceCheckTime(0.0f)
	, bLODUpdatePending(false)
{
	bCanEverTick = true;
    bTickEnabled = true;
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
        AccumLODDistanceCheckTime = 0.0f;
        bLODUpdatePending = (LODMethod == ParticleSystemLODMethod::ActivateAutomatic);

		CreateEmitterInstances();
        Activate();
    }
}

void UParticleSystemComponent::OnUnregister()
{
    DestroyEmitterInstances();
    UPrimitiveComponent::OnUnregister();
}

void UParticleSystemComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();

    // EmitterInstances는 런타임 데이터이므로 복제 시 초기화
    // 복제된 컴포넌트는 OnRegister에서 새로운 인스턴스를 생성할 것
    EmitterInstances.clear();

    // 런타임 상태 초기화
    SystemTime = 0.0f;
    CompletedLoops = 0;
    AccumLODDistanceCheckTime = 0.0f;
    bLODUpdatePending = false;
    bIsActive = false;

    // 이벤트 데이터 초기화
    CollisionEvents.clear();
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

    SystemTime = 0.0f;
    CompletedLoops = 0;
    AccumLODDistanceCheckTime = 0.0f;

    if (LODMethod == ParticleSystemLODMethod::ActivateAutomatic)
    {
        bLODUpdatePending = true;
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

void UParticleSystemComponent::ResetToDefaultState()
{
    CustomPlaybackRate = 1.0f;
    Restart();
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate)
{
    if (Template != NewTemplate)
    {
        DestroyEmitterInstances();
        Template = NewTemplate;
        SystemTime = 0.0f;
        CompletedLoops = 0;
        AccumLODDistanceCheckTime = 0.0f;
        bLODUpdatePending = false;
        bIsActive = false;

        if (Template)
        {
			LODDistanceCheckTime = Template->LODDistanceCheckTime;
			LODMethod = Template->LODMethod;
            CreateEmitterInstances();

            // bAutoActivate가 true이면 자동으로 활성화
            if (bAutoActivate)
            {
                Activate();
            }
        }
    }
}

void UParticleSystemComponent::SetLODIndex(int32 LODIndex)
{
    if (LODMethod != ParticleSystemLODMethod::DirectSet)
    {
        UE_LOG("WARNING: SetLODIndex called but LODMethod is not DirectSet.");
        return;
    }
    if(!Template)
    {
        UE_LOG("WARNING: SetLODIndex called but Template is null.");
        return;
	}

    if (LODIndex < 0 || LODIndex >= Template->GetLODLevelCount())
    {
        UE_LOG("WARNING: SetLODIndex called with invalid LODIndex %d.", LODIndex);
        return;
	}

    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
			Instance->SetLODLevel(LODIndex);
        }
    }
}

void UParticleSystemComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    if (bInIsLoading)
    {
        int32 LODMethodValue = static_cast<int32>(ParticleSystemLODMethod::Automatic);
        FJsonSerializer::ReadInt32(InOutHandle, "LODMethod", LODMethodValue, static_cast<int32>(ParticleSystemLODMethod::Automatic), false);
        LODMethod = static_cast<ParticleSystemLODMethod>(LODMethodValue);
    }
    else
    {
        InOutHandle["LODMethod"] = static_cast<long>(LODMethod);
    }
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
    if (!bIsActive || !Template)
    {
        return;
    }

    const float EffectiveRate = FMath::Max(CustomPlaybackRate, 0.0f);
    if (EffectiveRate <= 0.0f)
    {
        return;
    }

    TIME_PROFILE(ParticleSystemComponentTick);

    const float ScaledDeltaTime = DeltaTime * EffectiveRate;

    // 프레임 시작 - 이벤트 배열 클리어
    CollisionEvents.Empty();

    if (Template->SystemDuration > 0.0f)
    {
        SystemTime += ScaledDeltaTime;
        while (SystemTime >= Template->SystemDuration)
        {
            SystemTime -= Template->SystemDuration;
            ++CompletedLoops;
            if (Template->SystemLoops > 0 && CompletedLoops >= Template->SystemLoops)
            {
                Stop();
                return;
            }
        }
    }

    if (LODMethod == ParticleSystemLODMethod::Automatic)
    {
        if (LODDistanceCheckTime > 0.0f)
        {
            AccumLODDistanceCheckTime += DeltaTime; // 재생시간 영향 없이 실제 시간 기준 

            while (AccumLODDistanceCheckTime >= LODDistanceCheckTime)
            {
                bLODUpdatePending = true;
                AccumLODDistanceCheckTime -= LODDistanceCheckTime;
            }
        }
        else
        {
            bLODUpdatePending = true; // 0 또는 음수면 매 프레임 체크로 간주
        }
    }

    int32 NewLOD = 0;
    if (bLODUpdatePending)
    {
        const FVector EffectLocation = GetWorldLocation();
		NewLOD = DetermineLODLevelForLocation(EffectLocation);
    }
    

    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        if (Instance)
        {
            if (bLODUpdatePending)
            {
				Instance->SetLODLevel(NewLOD);
            }
            UpdateEmitterInstance(Instance, ScaledDeltaTime);
        }
    }

    // 프레임 종료 - 축적된 이벤트 브로드캐스트
    for (const FParticleEventCollideData& Event : CollisionEvents)
    {
        OnParticleCollide.Broadcast(
            Event.EventName,
            Event.EmitterTime,
            Event.ParticleTime,
            Event.Location,
            Event.Velocity,
            Event.Direction,
            Event.Normal,
            Event.BoneName
        );
    }

    bLODUpdatePending = false;
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

        // Editor visibility check - skip if not visible
        if (!Instance->bEditorVisible)
            continue;

        // Editor render mode - skip if set to None
        if (Instance->EditorRenderMode == EEmitterRenderMode::None)
            continue;

        UParticleModuleRequired* RequiredModule = Instance->CurrentLODLevel->RequiredModule;

        // Material이 없으면 렌더링 불가 (단, 메시 파티클이 bUseMeshMaterials 사용 시 예외)
        UParticleModuleTypeDataMesh* MeshTypeData = Cast<UParticleModuleTypeDataMesh>(Instance->CurrentLODLevel->TypeDataModule);
        bool bMeshUsesMeshMaterials = MeshTypeData && MeshTypeData->bUseMeshMaterials && MeshTypeData->Mesh;

        if (!RequiredModule->Material && !bMeshUsesMeshMaterials)
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
        else if (Cast<UParticleModuleTypeDataBeam>(LODLevel->TypeDataModule))
        {
            // Beam TypeData
            DynamicData = new FDynamicBeamEmitterData(Instance);
        }
        else
        {
            continue;
        }

        if (DynamicData)
        {
            // Store editor render mode in dynamic data for the renderer to use
            DynamicData->EditorRenderMode = Instance->EditorRenderMode;
            DynamicData->UpdateRenderData(View);
            RenderDataArray.Add(DynamicData);
        }
    }

    return RenderDataArray;
}

void UParticleSystemComponent::RebuildInstances()
{
    const bool bWasActive = bIsActive;
    // Fully rebuild instances from current Template definition
    DestroyEmitterInstances();
    if (Template)
    {
        CreateEmitterInstances();
    }
    SystemTime = 0.0f;
    CompletedLoops = 0;
    if (bWasActive)
    {
        Activate();
    }
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
    SystemTime = 0.0f;
    CompletedLoops = 0;

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
            UE_LOG("WARNING: Emitter %d skipped due to MaxParticleCount <= 0", i);
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
    SystemTime = 0.0f;
    CompletedLoops = 0;
    AccumLODDistanceCheckTime = 0.0f;
    bLODUpdatePending = false;

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
    if (!Instance)
    {
        return;
    }

	// 틱에서 스폰 -> 갱신 -> 제거 순서로 처리
    Instance->Tick(DeltaTime);
}

void UParticleSystemComponent::SpawnParticles(FParticleEmitterInstance* Instance, float DeltaTime)
{
    if (!Instance)
    {
        return;
    }

    Instance->SpawnParticles(DeltaTime);
}

void UParticleSystemComponent::KillDeadParticles(FParticleEmitterInstance* Instance)
{
    if (!Instance)
    {
        return;
    }

    Instance->KillDeadParticles();
}

void UParticleSystemComponent::ReportEventCollision(
    const FString& EventName,
    float EmitterTime,
    float ParticleTime,
    const FVector& Location,
    const FVector& Velocity,
    const FVector& Direction,
    const FVector& Normal,
    const FString& BoneName)
{
    // 새 이벤트 데이터 생성
    FParticleEventCollideData Event;
    Event.Type = EParticleEventType::Collision;
    Event.EventName = EventName;
    Event.EmitterTime = EmitterTime;
    Event.ParticleTime = ParticleTime;
    Event.Location = Location;
    Event.Velocity = Velocity;
    Event.Direction = Direction;
    Event.Normal = Normal;
    Event.BoneName = BoneName;

    // 배열에 추가
    CollisionEvents.Add(Event);
}

int32 UParticleSystemComponent::DetermineLODLevelForLocation(const FVector& EffectLocation) const
{
    if (!Template || Template->LODDistances.IsEmpty())
    {
        return 0;
    }

    // 뷰 위치 획득: 우선 PlayerCameraManager → EditorCamera → 기본값(효과 위치)
    FVector ViewLocation = EffectLocation;
    // USceneComponent::GetWorld는 const가 아니므로 const_cast로 접근
    if (UWorld* World = const_cast<UParticleSystemComponent*>(this)->GetWorld())
    {
        if (APlayerCameraManager* PCM = World->GetPlayerCameraManager())
        {
            if (FMinimalViewInfo* ViewInfo = PCM->GetCurrentViewInfo())
            {
                ViewLocation = ViewInfo->ViewLocation;
            }
        }
        else if (ACameraActor* EditorCam = World->GetEditorCameraActor())
        {
            if (UCameraComponent* CamComp = EditorCam->GetCameraComponent())
            {
                ViewLocation = CamComp->GetWorldLocation();
            }
        }
    }

    const float Distance = (EffectLocation - ViewLocation).Size();

    // 거리 기준으로 가장 큰 인덱스를 선택 (LODDistances는 오름차순, 0번은 0)
    int32 SelectedLOD = 0;
    const int32 LODCount = Template->LODDistances.Num();
    for (int32 Index = 1; Index < LODCount; ++Index)
    {
        if (Distance >= Template->LODDistances[Index])
        {
            SelectedLOD = Index;
        }
        else
        {
            break;
        }
    }

    return SelectedLOD;
}
