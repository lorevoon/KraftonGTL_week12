#pragma once

#include "Object.h"
#include "UParticleModule.generated.h"

struct FBaseParticle;
struct FParticleEmitterInstance;

// 파티클 모듈 베이스 클래스
// - 실행 단계 플래그 시스템 (bSpawnModule, bUpdateModule, bFinalUpdateModule)
// - 한 모듈이 여러 단계에서 실행 가능하도록 설계
UCLASS(Abstract, DisplayName="파티클 모듈", Description="파티클 동작을 정의하는 모듈 베이스 클래스입니다")
class UParticleModule : public UObject
{
public:
    GENERATED_REFLECTION_BODY()

    UParticleModule();

    // 실행 단계 플래그
    // - ExecutionOrder 대신 각 단계별 bool 플래그 사용
    // - 한 모듈이 여러 단계에서 실행될 수 있음 (예: Color 모듈이 Spawn + Update 모두 실행)

    // Spawn 단계에서 실행 여부 (파티클 생성 시)
    UPROPERTY()
    bool bSpawnModule;

    // Update 단계에서 실행 여부 (매 프레임)
    UPROPERTY()
    bool bUpdateModule;

    // FinalUpdate 단계에서 실행 여부 (렌더링 직전)
    UPROPERTY()
    bool bFinalUpdateModule;

    // 모듈 활성화 여부
    UPROPERTY(EditAnywhere, Category="Module")
    bool bEnabled;

    // 모듈 이름 (디버깅/에디터 용)
    UPROPERTY(EditAnywhere, Category="Module")
    FString ModuleName;

public:
    // 가상 함수 인터페이스

    // Spawn: 파티클 생성 시 호출
    // - Lifetime, InitialVelocity, InitialColor 등 설정
    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
    {
        // 기본 구현 없음 - 서브클래스에서 오버라이드
    }

    // Update: 매 프레임 호출
    // - Acceleration, ColorOverLife 등 업데이트
    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
    {
        // 기본 구현 없음 - 서브클래스에서 오버라이드
    }

    // FinalUpdate: 렌더링 직전 호출
    // - 카메라 정렬, 최종 트랜스폼 등
    virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
    {
        // 기본 구현 없음 - 서브클래스에서 오버라이드
    }

    // 에디터 전용: 프로퍼티 변경 시 호출
    virtual void PostEditChangeProperty()
    {
        // 기본 구현 없음
    }

    // 모듈이 파티클 하나당 추가로 요구하는 바이트 수
    // - 기본값 0, 페이로드를 사용하는 모듈이 override
    virtual uint32 RequiredBytes() const
    {
        return 0;
    }
};
