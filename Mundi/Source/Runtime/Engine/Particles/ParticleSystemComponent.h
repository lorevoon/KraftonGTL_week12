#pragma once

#include "ParticleHelper.h"
#include "PrimitiveComponent.h"
#include "ParticleSystem.h"
#include "ParticleTypes.h"
#include "UParticleSystemComponent.generated.h"

// 파티클 시스템 컴포넌트 (런타임)
// - UParticleSystem 에셋을 인스턴스화하여 실행
// - FParticleEmitterInstance 배열 관리
// - 외부에서 UpdateParticles() 호출하여 파티클 업데이트 수행
UCLASS(DisplayName="파티클 시스템 컴포넌트", Description="파티클 시스템을 렌더링하는 컴포넌트입니다")
class UParticleSystemComponent : public UPrimitiveComponent
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleSystemComponent();
    virtual ~UParticleSystemComponent();

    // 파티클 시스템 에셋 참조
    UPROPERTY(EditAnywhere, Category="ParticleSystem")
    UParticleSystem* Template;

    // 런타임 이미터 인스턴스 배열
    // - 각 UParticleEmitter에 대응하는 FParticleEmitterInstance
    TArray<FParticleEmitterInstance*> EmitterInstances;

    // 자동 활성화 여부
    UPROPERTY(EditAnywhere, Category="ParticleSystem")
    bool bAutoActivate;

    // 활성화 상태
    UPROPERTY()
    bool bIsActive;

public:
    // 생명주기 함수

    // 컴포넌트 등록: 에셋에서 런타임 인스턴스 생성
    void OnRegister(UWorld* InWorld) override;

    // 컴포넌트 등록 해제: 메모리 정리
    void OnUnregister() override;

public:
    // 제어 함수

    // 파티클 시스템 활성화
    void Activate();

    // 파티클 시스템 비활성화
    void Deactivate();

    // 즉시 정지 및 파티클 제거
    void Stop();

    // 파티클 시스템 재시작
    void Restart();

    // 파티클 시스템 에셋 설정
    void SetTemplate(UParticleSystem* NewTemplate);

    // 파티클 업데이트 (외부에서 호출)
    void UpdateParticles(float DeltaTime);

    //@TODO Prepare과 Get 로직 분리
    // 렌더링 데이터 가져오기 (렌더러에서 호출)
    TArray<FDynamicEmitterDataBase*> GetRenderData(FSceneView* View);
private:
    // 내부 함수

    // 이미터 인스턴스 생성
    void CreateEmitterInstances();

    // 이미터 인스턴스 제거
    void DestroyEmitterInstances();

    // 단일 이미터 업데이트
    void UpdateEmitterInstance(FParticleEmitterInstance* Instance, float DeltaTime);

    // 파티클 스폰 처리
    void SpawnParticles(FParticleEmitterInstance* Instance, float DeltaTime);

    // 파티클 제거 처리
    void KillDeadParticles(FParticleEmitterInstance* Instance);
};
