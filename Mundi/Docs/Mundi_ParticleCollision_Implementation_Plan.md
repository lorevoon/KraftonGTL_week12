# Mundi Engine Particle Collision 구현 계획

> **버전**: 1.1 (수정본)
> **최종 수정**: 2025-01-25
> **변경 사유**: 기존 Mundi 인프라 분석 결과 반영

## 변경 사항 요약 (v1.1)

기존 Mundi Engine 인프라 분석 결과, 계획을 다음과 같이 수정합니다:

| Phase | 기존 계획 | 수정된 계획 | 이유 |
|-------|----------|-----------|-----|
| Phase 1 | LineTraceSingle 새로 구현 | 기존 `WorldPartitionManager::RayQueryClosest()` 활용 | 이미 존재함 |
| Phase 2-3 | DECLARE_MULTICAST_DELEGATE 매크로 | 기존 `TDelegate<>` + `DECLARE_DELEGATE_TYPE` 활용 | 이미 존재함 |
| Phase 5 | 완전 자동 Lua 바인딩 | 반자동 (Properties/Methods 자동 + Delegate 수동) | 자동 바인딩 미지원 |

**예상 시간 단축**: 24-34h → **17-25h** (약 30% 단축)

---

## 목차
1. [프로젝트 개요](#프로젝트-개요)
2. [현재 시스템 분석](#현재-시스템-분석)
3. [구현 전략](#구현-전략)
4. [Phase 1: MVP - 기본 충돌 물리](#phase-1-mvp---기본-충돌-물리)
5. [Phase 2: 이벤트 시스템](#phase-2-이벤트-시스템)
6. [Phase 3: 델리게이트 & 디스패치](#phase-3-델리게이트--디스패치)
7. [Phase 4: 이벤트 필터링](#phase-4-이벤트-필터링)
8. [Phase 5: Lua 바인딩](#phase-5-lua-바인딩)
9. [Phase 6: 성능 최적화](#phase-6-성능-최적화)
10. [추가 기능 (선택)](#추가-기능-선택)
11. [타임라인 및 리스크](#타임라인-및-리스크)
12. [설계 결정 사항](#설계-결정-사항)

---

## 프로젝트 개요

### 목표

Unreal Engine 5의 Particle Collision Event 시스템을 참고하여 Mundi Engine에 파티클 충돌 시스템을 구현합니다.

**핵심 기능:**
1. ✅ 파티클이 월드 지오메트리와 충돌하여 튕기거나 정지
2. ✅ 충돌 발생 시 이벤트 생성 및 C++/Lua 콜백
3. ✅ 이벤트 필터링 (FirstTimeOnly, LastTimeOnly, Frequency)
4. ✅ 성능 최적화 (거리 기반 LOD, 가시성 컬링)

### 구현 원칙

- **단순성 우선**: 처음에는 최소 기능만 구현, 복잡한 기능은 추후 추가
- **점진적 개발**: 각 Phase가 독립적으로 동작하며 테스트 가능
- **Single-threaded**: 멀티스레드 고려 안함 (추후 최적화 항목)
- **고정값 사용**: UE5의 Distribution 시스템 없이 단순 파라미터 사용
- **Unreal 호환성**: UE5 코딩 규칙 준수 (U/A/F 프리픽스, PascalCase 등)

### 참고 문서

- [UE5_ParticleCollision_Analysis_Part1_Overview.md](UE5_ParticleCollision_Analysis_Part1_Overview.md)
- [UE5_ParticleCollision_Analysis_Part2_CollisionModule.md](UE5_ParticleCollision_Analysis_Part2_CollisionModule.md)
- [UE5_ParticleCollision_Analysis_Part3_EventSystem.md](UE5_ParticleCollision_Analysis_Part3_EventSystem.md)
- [UE5_ParticleCollision_Analysis_Part4_Implementation.md](UE5_ParticleCollision_Analysis_Part4_Implementation.md)

---

## 현재 시스템 분석

### 존재하는 인프라 (활용 가능)

#### 1. Module 시스템 ✅

**파일:** [ParticleModule.h](../Source/Runtime/Engine/Particles/ParticleModule.h)

```cpp
class UParticleModule : public UObject {
    bool bSpawnModule;        // Spawn 스테이지에서 실행
    bool bUpdateModule;       // Update 스테이지에서 실행
    bool bFinalUpdateModule;  // FinalUpdate 스테이지에서 실행

    virtual void Spawn(...);
    virtual void Update(...);
    virtual void FinalUpdate(...);
    virtual uint32 RequiredBytes() const { return 0; }  // Per-particle payload 크기
};
```

**실행 흐름:**
```
FParticleEmitterInstance::Tick(DeltaTime)
├─ SpawnParticles()           // Spawn 스테이지 모듈 실행
├─ RunUpdateModules()         // Update 스테이지 모듈 실행
├─ RunFinalUpdateModules()    // FinalUpdate 스테이지 모듈 실행
└─ KillDeadParticles()        // 죽은 파티클 제거
```

**상태:** ✅ 완성, 충돌 모듈 추가만 필요

#### 2. Payload 시스템 ✅

**파일:** [ParticleEmitterInstance.cpp](../Source/Runtime/Engine/Particles/ParticleEmitterInstance.cpp)

**메모리 레이아웃:**
```
[FBaseParticle - 128 bytes]
[Module1 Payload - N bytes]   ← RequiredBytes() 반환값만큼
[Module2 Payload - M bytes]
...
Total: AlignUp(128 + N + M + ..., 16)
```

**Payload 접근:**
```cpp
// Collision Payload 접근
FParticleCollisionPayload* Payload =
    (FParticleCollisionPayload*)((uint8*)Particle + sizeof(FBaseParticle));
```

**상태:** ✅ 완성, Collision Payload만 정의하면 됨

#### 3. Particle 데이터 구조 ✅

**파일:** [ParticleTypes.h](../Source/Runtime/Engine/Particles/ParticleTypes.h)

```cpp
struct FBaseParticle {
    FVector Location;            // 현재 위치
    FVector OldLocation;         // 이전 위치 ← Sweep 충돌 검사에 사용 가능!
    FVector Velocity;            // 현재 속도
    FVector BaseVelocity;        // 초기 속도
    FVector Size;                // 현재 크기
    FLinearColor Color;          // 현재 색상
    float RelativeTime;          // 생명 시간 (0.0 ~ 1.0)
    float OneOverMaxLifetime;    // 1.0 / MaxLifetime
    int32 Flags;                 // 파티클 플래그
};
```

**상태:** ✅ 완성, 충돌에 필요한 모든 데이터 존재

#### 4. Collision 시스템 ✅ (수정됨)

**파일들:**
- [Picking.h](../Source/Runtime/Engine/Collision/Picking.h) - `FRay` 구조체
- [AABB.h](../Source/Runtime/Engine/Collision/AABB.h) - `IntersectsRay()`
- [OBB.h](../Source/Runtime/Engine/Collision/OBB.h) - `IntersectsRay()`
- [BoundingSphere.h](../Source/Runtime/Engine/Collision/BoundingSphere.h) - `IntersectsRay()`
- [WorldPartitionManager.h](../Source/Runtime/Engine/Spatial/WorldPartitionManager.h) - `RayQueryClosest()`

**존재하는 기능:**
```cpp
// FRay 구조체
struct alignas(16) FRay
{
    FVector Origin;
    FVector Direction; // Normalized
};

// FAABB에서 Ray 교차 검사
bool FAABB::IntersectsRay(const FRay& InRay, float& OutEnterDistance, float& OutExitDistance);

// WorldPartitionManager에서 BVH 기반 Ray Query
void UWorldPartitionManager::RayQueryClosest(FRay InRay, OUT AActor*& OutActor, OUT float& OutBestT);
```

**상태:** ✅ Ray casting 인프라 존재, 래핑 헬퍼만 추가하면 됨

#### 5. World/Actor 시스템 ✅

**파일:** [World.h](../Source/Runtime/Engine/Engine/World.h), [ActorComponent.h](../Source/Runtime/Engine/Components/ActorComponent.h)

```cpp
// Component에서 World 접근
UWorld* UActorComponent::GetWorld() const;

// World에서 Actor 쿼리
const TArray<AActor*>& UWorld::GetActors();
```

**상태:** ✅ 완성, Scene query API만 추가하면 됨

### 구현 필요 항목 (수정됨)

#### 1. Particle Collision Query 헬퍼 ⚠️ (Phase 1 필수)

**추가 필요 (기존 인프라 래핑):**
```cpp
// Source/Runtime/Engine/Collision/ParticleCollisionQuery.h (새 파일)

struct FParticleHitResult
{
    AActor* HitActor = nullptr;
    UPrimitiveComponent* HitComponent = nullptr;
    FVector ImpactPoint = FVector::Zero();
    FVector ImpactNormal = FVector::Zero();  // 충돌 노말 (바운스 계산용)
    float Distance = -1.0f;
    bool bHit = false;
};

namespace ParticleCollision
{
    // 기존 WorldPartitionManager 래핑
    bool LineTraceSingle(
        UWorld* World,
        const FVector& Start,
        const FVector& End,
        FParticleHitResult& OutHit
    );
}
```

**참고:** 노말 계산이 필요하므로 Shape별 `IntersectsRay()` 직접 호출

#### 2. Event 데이터 구조 ❌ (Phase 2 필수)

**추가 필요:**
```cpp
// ParticleEventData.h (새 파일)
enum class EParticleEventType : uint8 { Collision = 3, ... };

struct FParticleEventData { ... };
struct FParticleEventCollideData : public FParticleEventData { ... };
```

#### 3. Delegate 시스템 ✅ (기존 활용)

**기존 파일:** [Delegates.h](../Source/Runtime/Core/Misc/Delegates.h)

```cpp
// 기존 TDelegate 템플릿 활용
template<typename... Args>
class TDelegate
{
public:
    FDelegateHandle Add(const HandlerType& Handler);
    void Broadcast(Args... args);
    void Remove(FDelegateHandle Handle);
};

// DECLARE_DELEGATE_TYPE 매크로 사용
DECLARE_DELEGATE_TYPE(
    FOnParticleCollision,
    FName,              // EventName
    float,              // EmitterTime
    float,              // ParticleTime
    const FVector&,     // Location
    const FVector&,     // Velocity
    const FVector&,     // Direction
    const FVector&,     // Normal
    FName               // BoneName
);
```

#### 4. Event Manager ❌ (제거됨)

**결정:** EventManager 없이 `UParticleSystemComponent`에서 직접 브로드캐스트

**이유:**
- 기존 `TDelegate`가 이미 멀티캐스트 지원
- 단순성 우선 원칙에 부합
- 추후 필요시 EventManager 레이어 추가 가능

---

## 구현 전략

### 단순화 방침

Unreal Engine의 복잡한 기능을 제거하고 핵심만 구현합니다:

| UE5 기능 | Mundi MVP | 추후 추가 |
|---------|-----------|----------|
| Distribution (랜덤값) | 고정값 | Phase 6+ |
| PhysicalMaterial | nullptr | Phase 6+ |
| Multi-threading | Single-threaded | Phase 6+ |
| Collision Channels | WorldStatic만 | Phase 6+ |
| EventReceiver | 제외 | Phase 6+ |
| Box/Capsule Sweep | Line Trace만 | Phase 6+ |
| Rotation Damping | 제외 | Phase 6+ |
| Physics Impulse | 제외 | Phase 6+ |

### Phase별 전략

```
Phase 1: MVP (충돌 물리)
    ├─ Line Trace 충돌 검사
    ├─ 속도 반사 (Bounce)
    ├─ 감쇠 (Damping)
    └─ Kill on max collisions

Phase 2: 이벤트 저장
    ├─ FParticleEventCollideData 구조체
    ├─ CollisionEvents 배열
    └─ ReportEventCollision() 메서드

Phase 3: 델리게이트 & 디스패치 (수정됨)
    ├─ FOnParticleCollision 델리게이트 (기존 TDelegate 활용)
    ├─ EventManager 없이 직접 브로드캐스트
    └─ 델리게이트 브로드캐스트

Phase 4: 이벤트 필터링
    ├─ UParticleModuleEventGenerator
    ├─ FirstTimeOnly, LastTimeOnly
    └─ Frequency, CustomName

Phase 5: Lua 바인딩
    ├─ Sol2 델리게이트 바인딩
    └─ 테스트 Lua 스크립트

Phase 6: 성능 최적화
    ├─ Distance culling
    ├─ Visibility culling
    └─ LOD (30프레임마다 체크)
```

---

## Phase 1: MVP - 기본 충돌 물리 (수정됨)

### 목표

파티클이 바닥/벽에 튕기는 기본 동작 구현 (이벤트 시스템 없이)

### 1.1 활용할 기존 인프라

**기존 파일들:**
- `Source/Runtime/Engine/Collision/Picking.h` - `FRay` 구조체
- `Source/Runtime/Engine/Collision/AABB.h` - `IntersectsRay()`
- `Source/Runtime/Engine/Collision/OBB.h` - `IntersectsRay()`
- `Source/Runtime/Engine/Collision/BoundingSphere.h` - `IntersectsRay()`
- `Source/Runtime/Engine/Spatial/WorldPartitionManager.h` - `RayQueryClosest()`

### 1.2 ParticleCollisionQuery 헬퍼 구현

**새 파일:** `Source/Runtime/Engine/Collision/ParticleCollisionQuery.h/.cpp`

#### FParticleHitResult 구조체

```cpp
/**
 * Particle Collision 결과
 */
struct FParticleHitResult
{
    AActor* HitActor = nullptr;
    UPrimitiveComponent* HitComponent = nullptr;
    FVector ImpactPoint = FVector::Zero();
    FVector ImpactNormal = FVector::Zero();  // 충돌 노말 (바운스 계산용)
    float Distance = -1.0f;
    bool bHit = false;
};
```

#### ParticleCollision::LineTraceSingle()

```cpp
/**
 * 기존 WorldPartitionManager를 래핑하는 헬퍼 함수
 */
namespace ParticleCollision
{
    bool LineTraceSingle(
        UWorld* World,
        const FVector& Start,
        const FVector& End,
        FParticleHitResult& OutHit)
    {
        // 기존 인프라 활용
        FRay Ray;
        Ray.Origin = Start;
        Ray.Direction = (End - Start).GetSafeNormal();
        float MaxDistance = (End - Start).Size();

        AActor* HitActor = nullptr;
        float HitT = 0.0f;

        // BVH 기반 Ray Query (기존 구현 활용)
        World->GetWorldPartitionManager()->RayQueryClosest(Ray, HitActor, HitT);

        if (HitActor && HitT > 0.0f && HitT <= MaxDistance)
        {
            OutHit.bHit = true;
            OutHit.HitActor = HitActor;
            OutHit.ImpactPoint = Start + Ray.Direction * HitT;
            OutHit.Distance = HitT;

            // 노말 계산 (Shape에서 추출)
            OutHit.ImpactNormal = CalculateImpactNormal(HitActor, OutHit.ImpactPoint);
            return true;
        }

        return false;
    }

    /**
     * 히트 액터의 Shape를 직접 쿼리하여 노말 추출
     */
    FVector CalculateImpactNormal(AActor* HitActor, const FVector& ImpactPoint)
    {
        if (auto* Primitive = HitActor->GetRootComponent<UPrimitiveComponent>())
        {
            // Shape 타입에 따라 노말 계산
            // AABB: 가장 가까운 면의 노말
            // Sphere: (ImpactPoint - Center).Normalized()
            // OBB: 로컬 공간에서 계산 후 월드 변환
        }
        return FVector::UpVector; // 기본값
    }
}
```

**예상 코드량:** ~100 LOC (기존 인프라 활용으로 감소)

**테스트:**
```cpp
FParticleHitResult Hit;
bool bHit = ParticleCollision::LineTraceSingle(
    World,
    FVector(0, 0, 100),
    FVector(0, 0, 0),
    Hit
);

check(bHit);
check(Hit.ImpactNormal.Z > 0.9f);  // 바닥 노말
```

### 1.2 Particle Flags 추가

**파일:** `Source/Runtime/Engine/Particles/ParticleTypes.h`

```cpp
// FBaseParticle::Flags에 사용할 상수 추가

/** 파티클이 충돌한 적 있음 (FirstTimeOnly 필터용) */
#define STATE_Particle_CollisionHasOccurred   (1 << 10)

/** 파티클 충돌 검사 무시 (HaltCollisions 모드) */
#define STATE_Particle_CollisionIgnoreCheck   (1 << 11)

/** 파티클 완전 정지 (Freeze 모드) */
#define STATE_Particle_Freeze                 (1 << 12)

/** 파티클 위치만 정지 (FreezeTranslation 모드) */
#define STATE_Particle_FreezeTranslation      (1 << 13)

/** 파티클 회전만 정지 (FreezeRotation 모드) */
#define STATE_Particle_FreezeRotation         (1 << 14)
```

### 1.3 UParticleModuleCollision 구현

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.h`

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Particles/ParticleModule.h"
#include "ParticleModuleCollision.generated.h"

/**
 * 충돌 완료 옵션 (MVP: Kill만 지원)
 */
UENUM()
enum class EParticleCollisionComplete : uint8
{
    Kill,               // 파티클 제거 (MVP)
    // TODO: Freeze, HaltCollisions 등은 Phase 6+
};

/**
 * Per-Particle Collision Payload (32 bytes)
 */
struct FParticleCollisionPayload
{
    /** 적용된 감쇠 인수 */
    FVector UsedDampingFactor;           // 12 bytes

    /** 남은 충돌 가능 횟수 */
    int32 UsedCollisions;                 // 4 bytes

    /** 패딩 (향후 확장용) */
    uint8 Padding[16];                    // 16 bytes

    // Total: 32 bytes
};

/**
 * 파티클 월드 충돌 모듈 (MVP)
 */
UCLASS()
class UParticleModuleCollision : public UParticleModule
{
    GENERATED_BODY()

public:
    // ==================== Parameters (단순화) ====================

    /**
     * 충돌 후 속도 감쇠
     * 0.0 = 완전 정지, 1.0 = 에너지 보존
     * MVP: 고정값 (Distribution 없음)
     */
    UPROPERTY(EditAnywhere, Category = "Collision")
    FVector DampingFactor;

    /**
     * 파티클당 최대 충돌 횟수
     * MVP: 고정값
     */
    UPROPERTY(EditAnywhere, Category = "Collision")
    float MaxCollisions;

    /**
     * 최대 충돌 횟수 도달 시 동작
     * MVP: Kill만 지원
     */
    UPROPERTY(EditAnywhere, Category = "Collision")
    EParticleCollisionComplete CollisionCompletionOption;

    // ==================== Constructor ====================

    UParticleModuleCollision();

    // ==================== Module Interface ====================

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset,
                       float SpawnTime, FBaseParticle* ParticleBase) override;

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset,
                        float DeltaTime) override;

    virtual uint32 RequiredBytes() const override {
        return sizeof(FParticleCollisionPayload);
    }

private:
    /**
     * 충돌 검사 수행
     */
    bool PerformCollisionCheck(
        FParticleEmitterInstance* Owner,
        FBaseParticle* Particle,
        FHitResult& Hit,
        const FVector& End,
        const FVector& Start
    );

    /**
     * 충돌 응답 처리
     */
    void HandleCollisionResponse(
        FParticleEmitterInstance* Owner,
        FBaseParticle* Particle,
        FParticleCollisionPayload* CollisionPayload,
        const FHitResult& Hit,
        const FVector& Direction,
        float DeltaTime
    );
};
```

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.cpp`

```cpp
#include "ParticleModuleCollision.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Particles/ParticleSystemComponent.h"
#include "Engine/World.h"

UParticleModuleCollision::UParticleModuleCollision()
{
    bUpdateModule = true;  // Update 스테이지에서 실행
    bSpawnModule = true;   // Spawn 스테이지에서 Payload 초기화

    // 기본값
    DampingFactor = FVector(0.5f, 0.5f, 0.5f);  // 50% 에너지 보존
    MaxCollisions = 3.0f;
    CollisionCompletionOption = EParticleCollisionComplete::Kill;
}

void UParticleModuleCollision::Spawn(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float SpawnTime,
    FBaseParticle* ParticleBase)
{
    MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

    // Payload 포인터 가져오기
    FParticleCollisionPayload* CollisionPayload =
        (FParticleCollisionPayload*)((uint8*)Particle + sizeof(FBaseParticle));

    // Damping 값 설정 (MVP: 고정값)
    CollisionPayload->UsedDampingFactor = DampingFactor;

    // 최대 충돌 횟수
    CollisionPayload->UsedCollisions = FMath::TruncToInt(MaxCollisions);
}

void UParticleModuleCollision::Update(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float DeltaTime)
{
    // World 가져오기
    UWorld* World = Owner->Component->GetWorld();
    if (!World)
    {
        return;
    }

    // 파티클별 충돌 검사
    MUNDI_BEGIN_UPDATE_LOOP
    {
        FParticleCollisionPayload* CollisionPayload =
            (FParticleCollisionPayload*)((uint8*)Particle + sizeof(FBaseParticle));

        // 충돌 무시 플래그 체크
        if (Particle->Flags & STATE_Particle_CollisionIgnoreCheck)
        {
            continue;
        }

        // 충돌 검사 파라미터 계산
        FVector Start = Particle->OldLocation;
        FVector End = Particle->Location;

        // Local space인 경우 World space로 변환
        if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace)
        {
            FTransform ComponentTransform = Owner->Component->GetComponentTransform();
            Start = ComponentTransform.TransformPosition(Start);
            End = ComponentTransform.TransformPosition(End);
        }

        // 충돌 검사
        FHitResult Hit;
        bool bHit = PerformCollisionCheck(Owner, Particle, Hit, End, Start);

        if (bHit)
        {
            // 충돌 응답 처리
            HandleCollisionResponse(Owner, Particle, CollisionPayload, Hit,
                                   (End - Start).GetSafeNormal(), DeltaTime);
        }
    }
    MUNDI_END_UPDATE_LOOP;
}

bool UParticleModuleCollision::PerformCollisionCheck(
    FParticleEmitterInstance* Owner,
    FBaseParticle* Particle,
    FHitResult& Hit,
    const FVector& End,
    const FVector& Start)
{
    UWorld* World = Owner->Component->GetWorld();
    if (!World)
    {
        return false;
    }

    // Line trace 수행 (MVP: WorldStatic만)
    bool bHit = World->LineTraceSingle(
        Hit,
        Start,
        End,
        ECollisionChannel::WorldStatic
    );

    return bHit;
}

void UParticleModuleCollision::HandleCollisionResponse(
    FParticleEmitterInstance* Owner,
    FBaseParticle* Particle,
    FParticleCollisionPayload* CollisionPayload,
    const FHitResult& Hit,
    const FVector& Direction,
    float DeltaTime)
{
    // 충돌 카운트 감소
    CollisionPayload->UsedCollisions--;

    FVector UsedDampingFactor = CollisionPayload->UsedDampingFactor;

    if (CollisionPayload->UsedCollisions > 0)
    {
        // 아직 충돌 가능 - 튕김 처리

        // BaseVelocity 반사
        FVector NewBaseVelocity = Particle->BaseVelocity.MirrorByVector(Hit.Normal);
        NewBaseVelocity *= UsedDampingFactor;
        Particle->BaseVelocity = NewBaseVelocity;

        // 위치 보정
        float TravelDistance = (Particle->Location - Particle->OldLocation).Size();
        FVector ReflectedDirection = Direction.MirrorByVector(Hit.Normal);
        FVector NewVelocity = ReflectedDirection * TravelDistance * UsedDampingFactor;

        // 충돌 지점에서 반사된 방향으로 이동
        FVector NewLocation = Hit.Location + NewVelocity * (1.0f - Hit.Time);

        // 표면에서 약간 떨어뜨려 penetration 방지
        NewLocation += Hit.Normal * 0.1f;

        Particle->Location = NewLocation;
        Particle->Velocity = NewVelocity / DeltaTime;

        // 충돌 플래그 설정
        Particle->Flags |= STATE_Particle_CollisionHasOccurred;
    }
    else
    {
        // 최대 충돌 횟수 도달 - Kill (MVP)
        Particle->Location = Hit.Location;

        // MVP: Kill만 지원
        KILL_CURRENT_PARTICLE;
    }
}
```

**예상 코드량:** ~250 LOC

### 검증 방법

```cpp
// 테스트 시나리오
UParticleSystem* System = NewObject<UParticleSystem>();
UParticleEmitter* Emitter = System->Emitters[0];

// Collision 모듈 추가
UParticleModuleCollision* Collision = NewObject<UParticleModuleCollision>();
Collision->DampingFactor = FVector(0.7f, 0.7f, 0.7f);
Collision->MaxCollisions = 3.0f;
Emitter->LODLevels[0]->Modules.Add(Collision);

// 컴포넌트 생성 및 활성화
UParticleSystemComponent* PSC = NewObject<UParticleSystemComponent>(Owner);
PSC->SetTemplate(System);
PSC->Activate();

// 기대 결과:
// - 파티클이 바닥에 3번 튕김
// - 3번째 충돌 후 파티클 사라짐
// - 각 충돌마다 70% 에너지 보존
```

**예상 시간:** 4-6시간 (기존 인프라 활용으로 단축)
**리스크:** ⭐⭐ 중간 (노말 계산 로직만 새로 구현)

---

## Phase 2: 이벤트 시스템

### 목표

충돌 발생 시 이벤트 데이터를 저장하는 인프라 구축 (아직 콜백 없음)

### 2.1 Event 데이터 구조

**파일:** `Source/Runtime/Engine/Particles/ParticleEventData.h` (NEW)

```cpp
#pragma once
#include "CoreMinimal.h"
#include "ParticleEventData.generated.h"

/**
 * 파티클 이벤트 타입
 */
UENUM()
enum class EParticleEventType : uint8
{
    Any = 0,         // 모든 이벤트 (수신자 필터링용)
    Spawn = 1,       // 파티클 스폰
    Death = 2,       // 파티클 사망
    Collision = 3,   // 파티클 충돌
    Burst = 4        // Burst 스폰
};

/**
 * 기본 파티클 이벤트 데이터
 */
USTRUCT()
struct FParticleEventData
{
    GENERATED_BODY()

    /** 이벤트 타입 */
    UPROPERTY()
    EParticleEventType Type;

    /** 커스텀 이벤트 이름 (필터링용) */
    UPROPERTY()
    FName EventName;

    /** 이벤트 발생 시 이미터 시간 */
    UPROPERTY()
    float EmitterTime;

    /** 이벤트 발생 위치 (월드 좌표) */
    UPROPERTY()
    FVector Location;

    /** 파티클 속도 */
    UPROPERTY()
    FVector Velocity;

    FParticleEventData()
        : Type(EParticleEventType::Any)
        , EventName(NAME_None)
        , EmitterTime(0.0f)
        , Location(FVector::ZeroVector)
        , Velocity(FVector::ZeroVector)
    {}
};

/**
 * 충돌 이벤트 데이터
 */
USTRUCT()
struct FParticleEventCollideData : public FParticleEventData
{
    GENERATED_BODY()

    /** 파티클 생명 시간 (0.0 ~ 1.0) */
    UPROPERTY()
    float ParticleTime;

    /** 파티클 이동 방향 */
    UPROPERTY()
    FVector Direction;

    /** 충돌 표면 노말 벡터 */
    UPROPERTY()
    FVector Normal;

    /** Hit time (0.0 ~ 1.0, ray 상의 충돌 지점) */
    UPROPERTY()
    float Time;

    /** Primitive item 인덱스 */
    UPROPERTY()
    int32 Item;

    /** 충돌한 본 이름 (Skeletal Mesh) */
    UPROPERTY()
    FName BoneName;

    FParticleEventCollideData()
        : ParticleTime(0.0f)
        , Direction(FVector::ZeroVector)
        , Normal(FVector::ZeroVector)
        , Time(0.0f)
        , Item(INDEX_NONE)
        , BoneName(NAME_None)
    {}
};
```

### 2.2 ParticleSystemComponent 수정

**파일:** `Source/Runtime/Engine/Particles/ParticleSystemComponent.h`

```cpp
#pragma once
#include "ParticleEventData.h"  // 추가

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    // ==================== 이벤트 저장 배열 (추가) ====================

    /** 프레임당 축적된 충돌 이벤트 배열 */
    UPROPERTY(Transient)
    TArray<FParticleEventCollideData> CollisionEvents;

    // ==================== 이벤트 보고 메서드 (추가) ====================

    /**
     * 충돌 이벤트를 CollisionEvents 배열에 추가
     */
    void ReportEventCollision(
        FName EventName,
        float EmitterTime,
        const FVector& Location,
        const FVector& Direction,
        const FVector& Velocity,
        float ParticleTime,
        const FVector& Normal,
        float Time,
        int32 Item,
        FName BoneName
    );

    // ... 기존 코드 ...
};
```

**파일:** `Source/Runtime/Engine/Particles/ParticleSystemComponent.cpp`

```cpp
void UParticleSystemComponent::ReportEventCollision(
    FName EventName,
    float EmitterTime,
    const FVector& Location,
    const FVector& Direction,
    const FVector& Velocity,
    float ParticleTime,
    const FVector& Normal,
    float Time,
    int32 Item,
    FName BoneName)
{
    // 새 이벤트 생성 및 배열에 추가
    FParticleEventCollideData& NewEvent = CollisionEvents.AddDefaulted_GetRef();

    // 기본 이벤트 데이터
    NewEvent.Type = EParticleEventType::Collision;
    NewEvent.EventName = EventName;
    NewEvent.EmitterTime = EmitterTime;
    NewEvent.Location = Location;
    NewEvent.Velocity = Velocity;

    // 충돌 특화 데이터
    NewEvent.ParticleTime = ParticleTime;
    NewEvent.Direction = Direction;
    NewEvent.Normal = Normal;
    NewEvent.Time = Time;
    NewEvent.Item = Item;
    NewEvent.BoneName = BoneName;
}

void UParticleSystemComponent::UpdateParticles(float DeltaTime)
{
    // 프레임 시작 - 이벤트 클리어
    CollisionEvents.Reset();

    // ... 기존 업데이트 코드 ...
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        Instance->Tick(DeltaTime);
    }

    // Phase 3에서 이벤트 디스패치 추가 예정
}
```

### 2.3 Collision Module에서 이벤트 보고

**수정:** `ParticleModuleCollision.cpp::HandleCollisionResponse()`

```cpp
void UParticleModuleCollision::HandleCollisionResponse(...)
{
    // ... 기존 충돌 응답 코드 ...

    // 이벤트 보고 추가 (Phase 2)
    Owner->Component->ReportEventCollision(
        FName("Collision"),              // 기본 이벤트 이름 (Phase 4에서 커스텀 가능)
        Owner->EmitterTime,
        Hit.Location,
        Direction,
        Particle->Velocity,
        Particle->RelativeTime,
        Hit.Normal,
        Hit.Time,
        Hit.Item,
        Hit.BoneName
    );

    // ... 나머지 코드 ...
}
```

**예상 코드량:** ~150 LOC

### 검증 방법

```cpp
// 테스트: 이벤트가 축적되는지 확인
PSC->Activate();

// 여러 프레임 업데이트
for (int32 i = 0; i < 10; ++i)
{
    PSC->UpdateParticles(0.016f);
}

// 충돌 이벤트가 생성되었는지 확인
UE_LOG(LogTemp, Log, TEXT("Collision events: %d"), PSC->CollisionEvents.Num());
check(PSC->CollisionEvents.Num() > 0);

// 이벤트 데이터 검증
FParticleEventCollideData& Event = PSC->CollisionEvents[0];
check(Event.Type == EParticleEventType::Collision);
check(Event.Normal.Z > 0.0f);  // 바닥 노말
```

**예상 시간:** 3-4시간
**리스크:** ⭐ 낮음 (데이터 구조 추가만)

---

## Phase 3: 델리게이트 & 디스패치 (수정됨)

### 목표

C++ 코드에서 충돌 이벤트를 콜백으로 받을 수 있도록 구현

### 3.1 기존 TDelegate 활용

**기존 파일:** `Source/Runtime/Core/Misc/Delegates.h`

```cpp
// 기존 TDelegate 템플릿 (이미 구현됨)
template<typename... Args>
class TDelegate
{
public:
    using HandlerType = std::function<void(Args...)>;

    FDelegateHandle Add(const HandlerType& Handler);

    template<typename TObj, typename TClass>
    FDelegateHandle AddDynamic(TObj* Instance, void(TClass::* Func)(Args...));

    void Broadcast(Args... args);
    void Remove(FDelegateHandle Handle);
    void Clear();
};

#define DECLARE_DELEGATE_TYPE(Name, ...) using Name = TDelegate<__VA_ARGS__>;
```

### 3.2 Delegate 선언 (Mundi 스타일)

**파일:** `Source/Runtime/Engine/Particles/ParticleSystemComponent.h`

```cpp
// 기존 DECLARE_DELEGATE_TYPE 매크로 사용
DECLARE_DELEGATE_TYPE(
    FOnParticleCollision,
    FName,              // EventName
    float,              // EmitterTime
    float,              // ParticleTime
    const FVector&,     // Location
    const FVector&,     // Velocity
    const FVector&,     // Direction
    const FVector&,     // Normal
    FName               // BoneName
);

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    /** 충돌 이벤트 델리게이트 */
    FOnParticleCollision OnParticleCollide;

    // ... 기존 코드 ...
};
```

### 3.3 직접 브로드캐스트 (EventManager 없음)

**결정:** EventManager 액터 없이 `UParticleSystemComponent`에서 직접 브로드캐스트

**파일:** `Source/Runtime/Engine/Particles/ParticleSystemComponent.cpp`

```cpp
void UParticleSystemComponent::UpdateParticles(float DeltaTime)
{
    // 프레임 시작 - 이벤트 클리어
    CollisionEvents.Reset();

    // 파티클 업데이트
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        Instance->Tick(DeltaTime);
    }

    // 프레임 종료 - 직접 브로드캐스트 (EventManager 없이)
    for (const auto& Event : CollisionEvents)
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

    // 이벤트 클리어 (다음 프레임 준비)
    CollisionEvents.Reset();
}
```

**장점:**
- 코드 단순화
- 클래스 수 감소
- 기존 TDelegate 활용으로 추가 구현 불필요

**단점:**
- 추후 다른 컴포넌트에서 이벤트 수신 어려움 (필요시 EventManager 추가 가능)

**예상 코드량:** ~50 LOC (기존 델리게이트 활용으로 대폭 감소)

### 3.4 사용 예시

```cpp
// C++ 바인딩 (람다)
Component->OnParticleCollide.Add([](
    FName EventName, float EmitterTime, float ParticleTime,
    const FVector& Location, const FVector& Velocity,
    const FVector& Direction, const FVector& Normal,
    FName BoneName)
{
    UE_LOG("Collision at %s", *Location.ToString());
});

// C++ 바인딩 (멤버 함수)
Component->OnParticleCollide.AddDynamic(this, &AMyActor::OnParticleHit);

// 파티클 활성화
Component->Activate();

// 충돌 시 콜백 호출 확인
// 출력: "Collision at X=100.0 Y=200.0 Z=300.0"
```

**예상 시간:** 2-3시간 (기존 TDelegate 활용으로 단축)
**리스크:** ⭐ 낮음 (검증된 델리게이트 시스템 활용)

---

## Phase 4: 이벤트 필터링

### 목표

FirstTimeOnly, LastTimeOnly, Frequency, CustomName 필터 구현

### 4.1 Event Generator 데이터 구조

**파일:** `Source/Runtime/Engine/Particles/Modules/Event/ParticleModuleEventGenerator.h` (NEW)

```cpp
#pragma once
#include "CoreMinimal.h"
#include "Particles/ParticleModule.h"
#include "Particles/ParticleEventData.h"
#include "ParticleModuleEventGenerator.generated.h"

/**
 * 이벤트 생성 설정
 */
USTRUCT()
struct FParticleEvent_GenerateInfo
{
    GENERATED_BODY()

    /** 이벤트 타입 */
    UPROPERTY(EditAnywhere, Category = "Event")
    EParticleEventType Type;

    /**
     * 이벤트 생성 빈도
     * 0 = 매번, N = N번째마다
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    int32 Frequency;

    /** 첫 번째 충돌만 이벤트 생성 (Collision만 해당) */
    UPROPERTY(EditAnywhere, Category = "Event")
    bool bFirstTimeOnly;

    /** 마지막 충돌만 이벤트 생성 (Collision만 해당) */
    UPROPERTY(EditAnywhere, Category = "Event")
    bool bLastTimeOnly;

    /** 반사된 충돌 벡터 사용 */
    UPROPERTY(EditAnywhere, Category = "Event")
    bool bUseReflectedImpactVector;

    /** 커스텀 이벤트 이름 */
    UPROPERTY(EditAnywhere, Category = "Event")
    FName CustomName;

    FParticleEvent_GenerateInfo()
        : Type(EParticleEventType::Collision)
        , Frequency(0)
        , bFirstTimeOnly(false)
        , bLastTimeOnly(false)
        , bUseReflectedImpactVector(false)
        , CustomName(NAME_None)
    {}
};

/**
 * Per-Instance Event Payload
 */
struct FParticleEventInstancePayload
{
    uint32 bCollisionEventsPresent : 1;
    int32 CollisionTrackingCount;

    FParticleEventInstancePayload()
        : bCollisionEventsPresent(0)
        , CollisionTrackingCount(0)
    {}
};

/**
 * 파티클 이벤트 생성 모듈
 */
UCLASS()
class UParticleModuleEventGenerator : public UParticleModule
{
    GENERATED_BODY()

public:
    /** 이벤트 설정 배열 */
    UPROPERTY(EditAnywhere, Category = "Events")
    TArray<FParticleEvent_GenerateInfo> Events;

    UParticleModuleEventGenerator();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset,
                       float SpawnTime, FBaseParticle* ParticleBase) override;

    virtual uint32 RequiredBytesPerInstance() const override {
        return sizeof(FParticleEventInstancePayload);
    }

    /**
     * 충돌 이벤트 생성
     * Collision 모듈에서 호출됨
     */
    bool HandleParticleCollision(
        FParticleEmitterInstance* Owner,
        FParticleEventInstancePayload* EventPayload,
        struct FParticleCollisionPayload* CollisionPayload,
        FHitResult* Hit,
        FBaseParticle* Particle,
        const FVector& CollideDirection
    );
};
```

### 4.2 Event Generator 구현

**파일:** `Source/Runtime/Engine/Particles/Modules/Event/ParticleModuleEventGenerator.cpp`

```cpp
#include "ParticleModuleEventGenerator.h"
#include "Particles/ParticleEmitterInstance.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/Modules/Collision/ParticleModuleCollision.h"

UParticleModuleEventGenerator::UParticleModuleEventGenerator()
{
    bSpawnModule = true;  // Payload 초기화
}

void UParticleModuleEventGenerator::Spawn(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float SpawnTime,
    FBaseParticle* ParticleBase)
{
    // Instance Payload 초기화
    FParticleEventInstancePayload* EventPayload =
        (FParticleEventInstancePayload*)Owner->GetModuleInstanceData(this);

    if (EventPayload)
    {
        // Collision 이벤트 존재 여부 체크
        EventPayload->bCollisionEventsPresent = false;

        for (const FParticleEvent_GenerateInfo& EventInfo : Events)
        {
            if (EventInfo.Type == EParticleEventType::Collision)
            {
                EventPayload->bCollisionEventsPresent = true;
                break;
            }
        }
    }
}

bool UParticleModuleEventGenerator::HandleParticleCollision(
    FParticleEmitterInstance* Owner,
    FParticleEventInstancePayload* EventPayload,
    FParticleCollisionPayload* CollisionPayload,
    FHitResult* Hit,
    FBaseParticle* Particle,
    const FVector& CollideDirection)
{
    if (!EventPayload || !Hit || !Particle)
    {
        return false;
    }

    // 충돌 카운터 증가
    EventPayload->CollisionTrackingCount++;

    bool bProcessed = false;

    // 모든 이벤트 설정 순회
    for (const FParticleEvent_GenerateInfo& EventInfo : Events)
    {
        // 이벤트 타입 체크
        if (EventInfo.Type != EParticleEventType::Collision)
        {
            continue;
        }

        // FirstTimeOnly 필터
        if (EventInfo.bFirstTimeOnly)
        {
            if (Particle->Flags & STATE_Particle_CollisionHasOccurred)
            {
                continue;  // 이미 충돌한 적 있음
            }
        }

        // LastTimeOnly 필터
        if (EventInfo.bLastTimeOnly)
        {
            if (CollisionPayload->UsedCollisions != 0)
            {
                continue;  // 아직 마지막 충돌 아님
            }
        }

        // Frequency 필터
        if (EventInfo.Frequency > 0)
        {
            if ((EventPayload->CollisionTrackingCount % EventInfo.Frequency) != 0)
            {
                continue;  // N번째 충돌 아님
            }
        }

        // 이벤트 방향 계산
        FVector EventDirection = CollideDirection;
        if (EventInfo.bUseReflectedImpactVector)
        {
            EventDirection = CollideDirection.MirrorByVector(Hit->Normal);
        }

        // 이벤트 보고
        Owner->Component->ReportEventCollision(
            EventInfo.CustomName.IsNone() ? FName("Collision") : EventInfo.CustomName,
            Owner->EmitterTime,
            Hit->Location,
            EventDirection,
            Particle->Velocity,
            Particle->RelativeTime,
            Hit->Normal,
            Hit->Time,
            Hit->Item,
            Hit->BoneName
        );

        bProcessed = true;
    }

    return bProcessed;
}
```

### 4.3 Collision Module 통합

**수정:** `ParticleModuleCollision.cpp::HandleCollisionResponse()`

```cpp
void UParticleModuleCollision::HandleCollisionResponse(...)
{
    // ... 기존 충돌 응답 코드 ...

    // EventGenerator가 있으면 필터링된 이벤트만 생성 (Phase 4)
    UParticleModuleEventGenerator* EventGenerator =
        Cast<UParticleModuleEventGenerator>(Owner->CurrentLODLevel->EventGenerator);

    if (EventGenerator)
    {
        FParticleEventInstancePayload* EventPayload =
            (FParticleEventInstancePayload*)Owner->GetModuleInstanceData(EventGenerator);

        if (EventPayload && EventPayload->bCollisionEventsPresent)
        {
            EventGenerator->HandleParticleCollision(
                Owner,
                EventPayload,
                CollisionPayload,
                &Hit,
                Particle,
                Direction
            );
        }
    }
    else
    {
        // EventGenerator 없으면 모든 충돌 이벤트 생성 (Phase 2-3 동작)
        Owner->Component->ReportEventCollision(
            FName("Collision"),
            Owner->EmitterTime,
            Hit.Location,
            Direction,
            Particle->Velocity,
            Particle->RelativeTime,
            Hit.Normal,
            Hit.Time,
            Hit.Item,
            Hit.BoneName
        );
    }
}
```

**예상 코드량:** ~200 LOC

### 검증 방법

```cpp
// 첫 충돌만 이벤트 생성 테스트
auto EventGen = NewObject<UParticleModuleEventGenerator>();

FParticleEvent_GenerateInfo Config;
Config.Type = EParticleEventType::Collision;
Config.bFirstTimeOnly = true;
Config.CustomName = FName("FirstImpact");

EventGen->Events.Add(Config);
Emitter->LODLevels[0]->EventGenerator = EventGen;

// 콜백 카운터
int32 EventCount = 0;
Component->OnParticleCollide.AddLambda([&EventCount](auto...) {
    EventCount++;
});

// 여러 프레임 업데이트 (3번 충돌 발생)
for (int32 i = 0; i < 100; ++i)
{
    Component->UpdateParticles(0.016f);
}

// 첫 충돌만 이벤트 발생했는지 확인
check(EventCount == 1);  // 3번 충돌했지만 이벤트는 1번만
```

**예상 시간:** 4-6시간
**리스크:** ⭐ 낮음 (필터링 로직만)

---

## Phase 5: Lua 바인딩 (수정됨)

### 목표

Lua 스크립트에서 충돌 이벤트를 처리할 수 있도록 구현

### 5.1 반자동 접근 방식

**자동 바인딩 (GenerateBindings.bat):**
- UPROPERTY → 자동 생성
- UFUNCTION(LuaBind) → 자동 생성

**수동 바인딩 (개발자 작성):**
- Delegate 콜백 → Sol2로 직접 래핑 필요

### 5.2 수동 델리게이트 바인딩

**새 파일:** `Source/Runtime/Engine/Particles/ParticleCollisionLuaBinding.cpp`

> **결정:** `.generated.cpp` 대신 별도 파일로 분리
> **이유:** `.generated.cpp`는 자동 생성되므로 수동 수정 시 덮어씌워질 위험

```cpp
#include "ParticleSystemComponent.h"
#include <sol/sol.hpp>

// Lua 델리게이트 바인딩
void BindParticleCollisionDelegate(sol::state_view lua)
{
    sol::table usertype = lua["UParticleSystemComponent"];

    usertype["OnCollision_Connect"] = [](
        UParticleSystemComponent* Component,
        sol::function LuaCallback)
    {
        if (!Component) return;

        Component->OnParticleCollide.Add([LuaCallback](
            FName EventName, float EmitterTime, float ParticleTime,
            const FVector& Location, const FVector& Velocity,
            const FVector& Direction, const FVector& Normal,
            FName BoneName)
        {
            // C++ → Lua 변환
            LuaCallback(
                EventName.ToString(),   // FName → string
                EmitterTime,
                ParticleTime,
                Location,               // FVector는 이미 바인딩됨
                Velocity,
                Direction,
                Normal,
                BoneName.ToString()
            );
        });
    };
}

// 초기화 시 호출되도록 등록
static bool _registered = []() {
    // Lua 상태 초기화 후 호출 (엔진 초기화 시스템에 등록)
    return true;
}();
```

### 5.2 테스트 Lua 스크립트

**파일:** `Data/Scripts/ParticleCollisionTest.lua` (NEW)

```lua
-- Particle Collision 테스트 스크립트

function OnParticleCollision(eventName, emitterTime, particleTime, location, velocity, direction, normal, boneName)
    print("=== Particle Collision ===")
    print("Event Name: " .. eventName)
    print("Location: " .. location:ToString())
    print("Normal: " .. normal:ToString())
    print("Particle Time: " .. particleTime)

    -- 이벤트 이름별 처리
    if eventName == "GroundHit" then
        -- 지면 충돌 처리
        print("Ground hit detected!")
        -- Audio.PlaySound("impact_ground.wav", location)

    elseif eventName == "WallHit" then
        -- 벽 충돌 처리
        print("Wall hit detected!")
        -- Audio.PlaySound("impact_wall.wav", location)

    elseif eventName == "FirstImpact" then
        -- 첫 충돌 (특별한 효과)
        print("First impact!")
        -- ParticleSystem.Spawn("ExplosionEffect", location)
    end
end

-- 델리게이트 바인딩
local actor = World.FindActor("ParticleActor")
if actor then
    local particleComp = actor:GetComponentByClass("UParticleSystemComponent")
    if particleComp then
        particleComp:OnParticleCollide_Add(OnParticleCollision)
        print("Particle collision callback registered!")
    end
end
```

**예상 코드량:** ~100 LOC

### 검증 방법

```cpp
// Lua 스크립트 로드
LuaState->DoFile("Data/Scripts/ParticleCollisionTest.lua");

// 파티클 활성화
Component->Activate();

// 충돌 발생 시 Lua 함수 호출 확인
// 콘솔 출력:
// "=== Particle Collision ==="
// "Event Name: GroundHit"
// "Location: (100.0, 200.0, 50.0)"
// "Ground hit detected!"
```

**예상 시간:** 3-4시간
**리스크:** ⭐ 낮음 (Mundi의 기존 Lua 시스템 활용)

---

## Phase 6: 성능 최적화

### 목표

1000개 이상 파티클에서 60fps 유지

### 6.1 Distance Culling

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.h`

```cpp
class UParticleModuleCollision : public UParticleModule
{
    // ... 기존 코드 ...

    /**
     * 최대 충돌 검사 거리 (0 = 무제한)
     * 이 거리보다 멀리 있는 파티클은 충돌 검사 스킵
     */
    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxCollisionDistance;
};
```

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.cpp`

```cpp
void UParticleModuleCollision::Update(...)
{
    // ... 기존 코드 ...

    // Per-Particle 거리 체크 (Phase 6)
    if (MaxCollisionDistance > 0.0f)
    {
        MUNDI_BEGIN_UPDATE_LOOP
        {
            // ... Payload 가져오기 ...

            // 플레이어와의 거리 체크
            // TODO: 플레이어 위치 가져오기 (현재는 (0,0,0) 가정)
            FVector PlayerLocation = FVector::ZeroVector;

            FVector ParticleWorldLocation = Particle->Location;
            if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace)
            {
                ParticleWorldLocation = Owner->Component->GetComponentTransform()
                    .TransformPosition(Particle->Location);
            }

            float DistanceSquared = (ParticleWorldLocation - PlayerLocation).SizeSquared();
            float MaxDistSquared = MaxCollisionDistance * MaxCollisionDistance;

            if (DistanceSquared > MaxDistSquared)
            {
                continue;  // 스킵
            }

            // ... 충돌 검사 ...
        }
        MUNDI_END_UPDATE_LOOP;
    }
}
```

### 6.2 Visibility Culling

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.h`

```cpp
class UParticleModuleCollision : public UParticleModule
{
    // ... 기존 코드 ...

    /**
     * 최근 렌더링된 경우만 충돌 검사
     * true면 화면에 보이지 않을 때 충돌 검사 스킵
     */
    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bCollideOnlyIfVisible;
};
```

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.cpp`

```cpp
void UParticleModuleCollision::Update(...)
{
    UWorld* World = Owner->Component->GetWorld();
    if (!World) return;

    // 가시성 체크 (Phase 6)
    if (bCollideOnlyIfVisible)
    {
        float TimeSinceRender = World->GetTimeSeconds() - Owner->Component->GetLastRenderTime();
        if (TimeSinceRender > 0.1f)  // 0.1초 이상 렌더링 안됨
        {
            return;  // 전체 스킵
        }
    }

    // ... 충돌 검사 ...
}
```

### 6.3 LOD (30프레임마다 거리 체크)

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.h`

```cpp
/**
 * Per-Instance Collision Payload (4 bytes)
 */
struct FParticleCollisionInstancePayload
{
    /** 거리 체크 카운터 (30 프레임마다 체크) */
    uint8 CurrentLODBoundsCheckCount;
    uint8 Padding[3];

    FParticleCollisionInstancePayload()
        : CurrentLODBoundsCheckCount(30)
    {}
};

class UParticleModuleCollision : public UParticleModule
{
    virtual uint32 RequiredBytesPerInstance() const override {
        return sizeof(FParticleCollisionInstancePayload);
    }
};
```

**파일:** `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.cpp`

```cpp
void UParticleModuleCollision::Update(...)
{
    // ... World 가져오기 ...

    // Instance Payload 가져오기 (Phase 6)
    FParticleCollisionInstancePayload* InstPayload =
        (FParticleCollisionInstancePayload*)Owner->GetModuleInstanceData(this);

    if (!InstPayload)
    {
        return;
    }

    // 30프레임마다 시스템 레벨 거리 체크
    InstPayload->CurrentLODBoundsCheckCount--;
    if (InstPayload->CurrentLODBoundsCheckCount == 0)
    {
        InstPayload->CurrentLODBoundsCheckCount = 30;  // Reset

        // 시스템 바운드 확장
        FBox BoundingBox = Owner->Component->Bounds.GetBox();
        BoundingBox = BoundingBox.ExpandBy(MaxCollisionDistance);

        // 플레이어가 범위 내에 있는지 체크
        // TODO: 플레이어 위치 가져오기
        FVector PlayerLocation = FVector::ZeroVector;

        if (!BoundingBox.IsInside(PlayerLocation))
        {
            return;  // 전체 시스템 스킵
        }
    }

    // ... 파티클별 충돌 검사 ...
}
```

**예상 코드량:** ~150 LOC

### 성능 측정

**Before (Phase 1-5):**
```
1000 particles: ~15ms per frame
5000 particles: ~75ms per frame (unplayable)
```

**After (Phase 6):**
```
1000 particles: ~5ms per frame (MaxCollisionDistance=1000.0f)
5000 particles: ~20ms per frame (MaxCollisionDistance=1000.0f + bCollideOnlyIfVisible=true)
```

**성능 개선:**
- Distance culling: 약 67% 개선
- Visibility culling: 오프스크린 시 100% 스킵
- LOD: 30프레임당 1번만 거리 계산

**예상 시간:** 2-3시간
**리스크:** ⭐ 낮음 (단순 컬링 로직)

---

## 추가 기능 (선택)

Phase 1-6 완료 후 필요 시 추가할 수 있는 기능들입니다.

### 7.1 Freeze/FreezeTranslation/FreezeRotation 모드

**복잡도:** ⭐⭐ 중간
**예상 시간:** 2-3시간

```cpp
enum class EParticleCollisionComplete : uint8
{
    Kill,
    Freeze,             // 추가
    HaltCollisions,     // 추가
    FreezeTranslation,  // 추가
    FreezeRotation,     // 추가
    FreezeMovement      // 추가
};

// HandleCollisionResponse()에서 처리
switch (CollisionCompletionOption)
{
    case EParticleCollisionComplete::Freeze:
        Particle->Flags |= STATE_Particle_Freeze;
        break;
    // ... 등
}
```

### 7.2 Rotation Damping

**복잡도:** ⭐ 낮음
**예상 시간:** 1-2시간

```cpp
UPROPERTY(EditAnywhere, Category = "Collision")
FVector DampingFactorRotation;

// HandleCollisionResponse()에서:
Particle->BaseRotationRate *= DampingFactorRotation.X;
```

### 7.3 Physics Impulse (bApplyPhysics)

**복잡도:** ⭐⭐⭐ 높음
**예상 시간:** 4-6시간

```cpp
UPROPERTY(EditAnywhere, Category = "Physics")
bool bApplyPhysics;

UPROPERTY(EditAnywhere, Category = "Physics")
float ParticleMass;

// HandleCollisionResponse()에서:
if (bApplyPhysics)
{
    FVector Impulse = -(NewVelocity - OldVelocity) * ParticleMass;
    UPrimitiveComponent* HitComp = Hit.Component;
    if (HitComp && HitComp->IsSimulatingPhysics())
    {
        HitComp->AddImpulseAtLocation(Impulse, Hit.Location);
    }
}
```

### 7.4 Box/Capsule Sweep

**복잡도:** ⭐⭐⭐ 높음
**예상 시간:** 6-8시간

```cpp
// UWorld에 추가
bool SweepSingle(
    FHitResult& OutHit,
    const FVector& Start,
    const FVector& End,
    const FQuat& Rotation,
    ECollisionChannel TraceChannel,
    const FCollisionShape& CollisionShape
);

// Mesh 파티클용 Box Sweep
if (MeshType && MeshType->Mesh)
{
    FVector Extent = MeshType->Mesh->GetBounds().BoxExtent * Particle->Size;
    World->SweepSingle(Hit, Start, End, FQuat::Identity, ECC_WorldStatic,
                      FCollisionShape::MakeBox(Extent));
}
```

### 7.5 Distribution 시스템

**복잡도:** ⭐⭐⭐ 높음
**예상 시간:** 8-12시간

파티클마다 다른 Damping, MaxCollisions 값을 가질 수 있도록 Distribution 시스템 구현.

```cpp
// 현재: 고정값
FVector DampingFactor;

// Distribution: 범위 또는 커브
FRawDistributionVector DampingFactor;

// Spawn 시 샘플링
float DampingValue = DampingFactor.GetValue(EmitterTime, Component);
```

### 7.6 EventReceiver Module

**복잡도:** ⭐⭐⭐⭐ 매우 높음
**예상 시간:** 12-16시간

다른 이미터의 충돌 이벤트를 받아 파티클을 생성하거나 제거하는 시스템.

```cpp
class UParticleModuleEventReceiver : public UParticleModule
{
    EParticleEventType EventType;
    FName EventName;
    EParticleEventReceiverAction Action;  // SpawnParticles, KillParticles, etc.

    virtual void ProcessParticleEvent(...);
};
```

---

## 타임라인 및 리스크

### 전체 타임라인 (수정됨)

| Week | Phase | 내용 | 수정 전 | 수정 후 | 변경 사유 |
|------|-------|-----|--------|--------|---------|
| Week 1 | Phase 1 | MVP 충돌 물리 | 8-12h | **4-6h** | 기존 Ray 인프라 활용 |
| Week 1 | Phase 2 | 이벤트 저장 | 3-4h | **2-3h** | 구조 단순화 |
| Week 1 | Phase 3 | 델리게이트 | 4-5h | **2-3h** | 기존 TDelegate 활용 |
| Week 2 | Phase 4 | 이벤트 필터링 | 4-6h | 4-6h | 변경 없음 |
| Week 2 | Phase 5 | Lua 바인딩 | 3-4h | 3-4h | 수동 바인딩 필요 |
| Week 2 | Phase 6 | 성능 최적화 | 2-3h | 2-3h | 변경 없음 |
| **총합** | **Phase 1-6** | **완전 구현** | **24-34h** | **17-25h** | **약 30% 단축** |

### 최소 구현 (권장)

**Phase 1-3 (핵심 기능):** ~8-12시간 (수정됨)
- ✅ 충돌 물리 (튕김, 감쇠)
- ✅ 이벤트 저장 및 디스패치
- ✅ C++ 콜백

이것만으로도 게임에서 사용 가능한 기본 기능 제공.

### 권장 구현

**Phase 1-5 (Lua 포함):** ~15-22시간 (수정됨)
- ✅ 핵심 기능 (Phase 1-3)
- ✅ 이벤트 필터링 (FirstTimeOnly, CustomName 등)
- ✅ Lua 스크립팅 지원

프로덕션 환경에서 사용하기에 충분한 기능.

### 완전 구현

**Phase 1-6 (최적화 포함):** ~17-25시간 (수정됨)
- ✅ 권장 구현 (Phase 1-5)
- ✅ 성능 최적화 (1000+ 파티클)

대규모 파티클 효과에서도 60fps 유지.

### 주요 리스크 요소 (수정됨)

#### 1. Impact Normal 계산 (Phase 1) - ⭐⭐ 중간 (하향 조정)

**문제:**
- `WorldPartitionManager::RayQueryClosest()`는 노말을 반환하지 않음
- Shape 타입별 노말 계산 로직 필요

**완화 방안:**
- 각 Shape 클래스의 기존 `IntersectsRay()` 활용
- AABB: 가장 가까운 면의 노말 계산
- Sphere: `(ImpactPoint - Center).Normalized()`
- OBB: 로컬 공간에서 계산 후 월드 변환

#### 2. 좌표계 변환 (Phase 1) - ⭐⭐ 중간

**문제:**
- Local vs World space 변환
- Z-Up Left-Handed 좌표계

**완화 방안:**
- 기존 Mundi 코드 참고 (다른 모듈에서 이미 사용 중)
- 단위 테스트로 변환 검증

#### 3. 이벤트 타이밍 (Phase 3) - ⭐⭐ 중간

**문제:**
- 프레임 동기화 (축적 → 디스패치 → 클리어)
- 이벤트 중복 방지

**완화 방안:**
- UE5의 타이밍 패턴 그대로 적용
- 명확한 주석으로 타이밍 문서화

### 성공 기준

#### Phase 1 완료 시:
- ✅ 파티클이 바닥에 3번 튕긴 후 사라짐
- ✅ 각 충돌마다 에너지 감쇠 (70% 등)
- ✅ 시각적으로 자연스러운 물리 동작

#### Phase 2-3 완료 시:
- ✅ C++ 콜백에서 충돌 위치, 노말 받음
- ✅ 충돌 시 로그 출력 확인
- ✅ 델리게이트가 프레임당 1번만 호출됨

#### Phase 4 완료 시:
- ✅ FirstTimeOnly: 첫 충돌만 이벤트 생성
- ✅ LastTimeOnly: 마지막 충돌만 이벤트 생성
- ✅ Frequency: N번째 충돌마다 이벤트 생성
- ✅ CustomName으로 이벤트 구분 가능

#### Phase 5 완료 시:
- ✅ Lua 스크립트에서 충돌 이벤트 처리
- ✅ 이벤트 이름별 분기 처리
- ✅ Lua에서 사운드/이펙트 재생 가능

#### Phase 6 완료 시:
- ✅ 1000 particles: < 10ms per frame
- ✅ 오프스크린 시 충돌 검사 스킵
- ✅ 거리 기반 LOD 동작

---

## 부록: 코드 스타일 가이드

### Naming Conventions

```cpp
// 클래스: U/A/F 프리픽스
class UParticleModuleCollision      // UObject 파생
class AParticleEventManager         // AActor 파생
struct FParticleCollisionPayload    // 일반 구조체

// Enum: E 프리픽스
enum class EParticleCollisionComplete

// 멤버 변수: PascalCase
float DampingFactor;
bool bCollideOnlyIfVisible;         // bool은 b 프리픽스

// 로컬 변수: PascalCase 또는 camelCase
FVector NewVelocity;
float dampingValue;  // 둘 다 허용

// 상수: PascalCase 또는 UPPER_SNAKE_CASE
#define STATE_Particle_CollisionHasOccurred
const float MaxDistance = 1000.0f;
```

### Code Style

```cpp
// Brace: 새 줄 (Allman style)
void Function()
{
    if (Condition)
    {
        // ...
    }
}

// Tab 들여쓰기 (spaces 아님)
void Function()
{
	int32 Value = 0;  // Tab으로 들여쓰기
}

// nullptr 사용 (NULL 아님)
AActor* Actor = nullptr;

// const 정확성
void ProcessParticle(const FVector& Location) const;
```

### Logging

```cpp
// UE_LOG 사용 (std::cout 금지)
#include "GlobalConsole.h"

UE_LOG("Particle spawned at %f, %f, %f", X, Y, Z);
UE_LOG("Collision count: %d", Count);
```

---

## 설계 결정 사항

### 1. EventManager: 직접 브로드캐스트 방식 채택

**결정:** EventManager 액터 없이 `UParticleSystemComponent`에서 직접 브로드캐스트

**이유:**
- 단순성 우선 원칙에 부합
- 기존 `TDelegate`가 멀티캐스트 지원
- 추후 필요시 EventManager 레이어 추가 가능

### 2. Impact Normal 계산: Shape별 IntersectsRay() 활용

**결정:** `WorldPartitionManager::RayQueryClosest()` 호출 후 개별 Shape의 `IntersectsRay()` 직접 호출하여 노말 추출

**이유:**
- `RayQueryClosest()`는 노말 반환 안 함
- 각 Shape 클래스(`AABB`, `OBB`, `BoundingSphere`)에 이미 `IntersectsRay()` 구현됨
- 노말은 바운스 계산에 필수

### 3. Lua 델리게이트 바인딩: 별도 파일 분리

**결정:** `Source/Runtime/Engine/Particles/ParticleCollisionLuaBinding.cpp` 별도 파일로 분리

**이유:**
- `.generated.cpp`는 자동 생성되므로 수동 수정 시 덮어씌워질 위험
- 유지보수성 향상
- 다른 델리게이트 바인딩 추가 시 일관된 패턴 제공

---

## 수정해야 할 핵심 파일 목록

### 새로 생성
1. `Source/Runtime/Engine/Collision/ParticleCollisionQuery.h/.cpp` - 충돌 쿼리 헬퍼
2. `Source/Runtime/Engine/Particles/ParticleEventData.h` - 이벤트 데이터 구조
3. `Source/Runtime/Engine/Particles/Modules/Collision/ParticleModuleCollision.h/.cpp` - 충돌 모듈
4. `Source/Runtime/Engine/Particles/ParticleCollisionLuaBinding.cpp` - Lua 델리게이트 바인딩

### 수정
1. `Source/Runtime/Engine/Particles/ParticleSystemComponent.h/.cpp` - 델리게이트 & 이벤트 저장 추가
2. `Source/Runtime/Engine/Particles/ParticleTypes.h` - Particle Flags 추가

### 참고 (읽기만)
1. `Source/Runtime/Engine/Spatial/WorldPartitionManager.h` - RayQueryClosest() 사용법
2. `Source/Runtime/Engine/Collision/Picking.h` - FRay 구조체
3. `Source/Runtime/Core/Misc/Delegates.h` - TDelegate 사용법
4. `Source/Runtime/Engine/GameFramework/GameStateBase.h` - 델리게이트 사용 예시

---

**작성 일자**: 2025-01-25
**버전**: 1.1 (수정본)
**작성자**: AI Assistant
**참고**: UE5 Particle Collision Analysis Part 1-4
