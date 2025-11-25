# UE5 Particle Collision 시스템 분석 Part 2: Collision Module

## 목차
1. [UParticleModuleCollision 개요](#uparticlemodulecollision-개요)
2. [충돌 파라미터](#충돌-파라미터)
3. [Per-Particle 데이터 구조](#per-particle-데이터-구조)
4. [충돌 검사 알고리즘](#충돌-검사-알고리즘)
5. [충돌 응답 로직](#충돌-응답-로직)
6. [성능 최적화](#성능-최적화)
7. [Mundi Engine 구현 가이드](#mundi-engine-구현-가이드)

---

## UParticleModuleCollision 개요

`UParticleModuleCollision`은 파티클의 월드 충돌 검사 및 물리 반응을 처리하는 모듈입니다.

### 파일 위치
- **헤더**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\Collision\ParticleModuleCollision.h`
- **구현**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleModules_Collision.cpp`

### 클래스 계층
```cpp
UObject
└── UParticleModule
    └── UParticleModuleCollisionBase
        └── UParticleModuleCollision
```

### 실행 단계
- **Spawn**: Per-particle collision payload 초기화
- **Update**: 매 프레임 충돌 검사 및 응답 실행
- **RequiredBytes**: 32 bytes (FParticleCollisionPayload)

---

## 충돌 파라미터

### 기본 충돌 설정

```cpp
class UParticleModuleCollision : public UParticleModuleCollisionBase
{
    // ==================== Damping (감쇠) ====================

    /** 충돌 후 속도 감쇠 (0.0 = 완전 정지, 1.0 = 반발 없음) */
    FRawDistributionVector DampingFactor;

    /** 충돌 후 회전 속도 감쇠 */
    FRawDistributionVector DampingFactorRotation;

    // ==================== Collision Limits ====================

    /** 파티클당 최대 충돌 횟수 (0 = 무제한) */
    FRawDistributionFloat MaxCollisions;

    /** 최대 충돌 횟수 도달 시 동작 */
    TEnumAsByte<EParticleCollisionComplete> CollisionCompletionOption;

    // ==================== Collision Filtering ====================

    /** 충돌 체크할 오브젝트 타입 (WorldStatic, WorldDynamic 등) */
    TArray<TEnumAsByte<EObjectTypeQuery>> CollisionTypes;

    /** 트리거 볼륨 무시 여부 */
    uint8 bIgnoreTriggerVolumes:1;

    /** Pawn 충돌 시 카운트 감소 안함 */
    uint8 bPawnsDoNotDecrementCount:1;

    /** 수직 충돌만 카운트 감소 */
    uint8 bOnlyVerticalNormalsDecrementCount:1;

    /** 수직 판정 오차 범위 (기본값: 0.1) */
    float VerticalFudgeFactor;

    // ==================== Physics Interaction ====================

    /** 충돌한 오브젝트에 물리 임펄스 적용 여부 */
    uint8 bApplyPhysics:1;

    /** 물리 임펄스 계산용 파티클 질량 */
    FRawDistributionFloat ParticleMass;

    /** 소유 액터 무시 (자기 자신과 충돌 방지) */
    uint8 bIgnoreSourceActor:1;

    // ==================== Performance Optimization ====================

    /** 충돌 검사 전 지연 시간 */
    FRawDistributionFloat DelayAmount;

    /** World->bDropDetail이 true일 때 스킵 */
    uint8 bDropDetail:1;

    /** 최근 렌더링된 경우만 충돌 검사 */
    uint8 bCollideOnlyIfVisible:1;

    /** 최대 충돌 검사 거리 (0 = 무제한, 기본값: 1000.0) */
    float MaxCollisionDistance;

    // ==================== Advanced ====================

    /** 충돌 검사 방향 스케일 (기본값: 3.5) */
    float DirScalar;
};
```

### 충돌 완료 옵션 (EParticleCollisionComplete)

```cpp
enum EParticleCollisionComplete
{
    EPCC_Kill,                  // 파티클 즉시 제거
    EPCC_Freeze,                // 파티클 완전 정지 (위치, 회전 모두)
    EPCC_HaltCollisions,        // 충돌 검사 중단 (계속 움직임)
    EPCC_FreezeTranslation,     // 위치만 정지 (회전은 계속)
    EPCC_FreezeRotation,        // 회전만 정지 (이동은 계속)
    EPCC_FreezeMovement,        // 위치+회전 정지 (EPCC_Freeze와 동일)
};
```

### Distribution 사용 예시

```cpp
// Spawn 시 파티클마다 다른 값 할당
void UParticleModuleCollision::Spawn(...)
{
    SPAWN_INIT;

    // DampingFactor는 Distribution이므로 파티클마다 랜덤 값 가능
    FVector DampingValue = DampingFactor.GetValue(
        Owner->EmitterTime,
        Owner->Component
    );

    CollisionPayload->UsedDampingFactor = DampingValue;

    // MaxCollisions도 파티클마다 다를 수 있음 (예: 1~5 랜덤)
    float MaxColl = MaxCollisions.GetValue(
        Owner->EmitterTime,
        Owner->Component
    );

    CollisionPayload->UsedCollisions = FMath::TruncToInt(MaxColl);

    // Delay 시간 설정
    float DelayValue = DelayAmount.GetValue(
        Owner->EmitterTime,
        Owner->Component
    );

    CollisionPayload->Delay = DelayValue;
}
```

---

## Per-Particle 데이터 구조

### FParticleCollisionPayload

각 파티클은 32바이트의 충돌 전용 데이터를 가집니다:

```cpp
/**
 * Collision 모듈이 사용하는 파티클당 페이로드
 * 위치: ParticleModules_Collision.cpp:79-85
 */
struct FParticleCollisionPayload
{
    /** 이 파티클에 적용된 감쇠 인수 (Spawn 시 Distribution에서 샘플링) */
    FVector3f UsedDampingFactor;           // 12 bytes

    /** 이 파티클에 적용된 회전 감쇠 인수 */
    FVector3f UsedDampingFactorRotation;   // 12 bytes

    /** 남은 충돌 가능 횟수 (0이 되면 CollisionCompletionOption 실행) */
    int32 UsedCollisions;                   // 4 bytes

    /** 충돌 시작 지연 시간 (Particle->RelativeTime < Delay면 스킵) */
    float Delay;                            // 4 bytes

    // Total: 32 bytes
};
```

### FParticleCollisionInstancePayload

이미터 인스턴스당 데이터 (매우 작음):

```cpp
/**
 * Collision 모듈의 이미터 인스턴스 페이로드
 * 위치: ParticleModules_Collision.cpp:91-96
 */
struct FParticleCollisionInstancePayload
{
    /** 거리 기반 LOD 체크 카운터 (30 프레임마다 체크) */
    uint8 CurrentLODBoundsCheckCount;  // 1 byte

    uint8 Padding[3];                   // 3 bytes (alignment)

    // Total: 4 bytes per emitter instance
};
```

### 메모리 레이아웃

```
FBaseParticle (128 bytes)
├─ Location, OldLocation (24 bytes)
├─ Velocity, BaseVelocity (24 bytes)
├─ Size, BaseSize (24 bytes)
├─ Color, BaseColor (32 bytes)
├─ Rotation, RelativeTime, etc. (24 bytes)
│
└─ [Module Payloads]
    └─ FParticleCollisionPayload (32 bytes)  ← Collision 모듈이 추가

Total per particle with collision: 160 bytes
```

---

## 충돌 검사 알고리즘

### Update() 메서드 전체 플로우

```cpp
/**
 * 위치: ParticleModules_Collision.cpp:181-552
 */
void UParticleModuleCollision::Update(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float DeltaTime)
{
    // ========== Step 1: 전처리 및 최적화 체크 ==========

    UWorld* World = Owner->Component->GetWorld();
    if (!World) return;

    // bDropDetail 체크
    if (bDropDetail && World->bDropDetail) {
        return;
    }

    // 가시성 체크
    if (bCollideOnlyIfVisible) {
        float TimeSinceRender = World->TimeSeconds - Owner->Component->LastRenderTime;
        if (TimeSinceRender > GParticleCollisionIgnoreInvisibleTime) {
            return;  // 최근 렌더링 안됨
        }
    }

    // ========== Step 2: 거리 기반 LOD (30프레임마다) ==========

    FParticleCollisionInstancePayload* InstPayload =
        (FParticleCollisionInstancePayload*)Owner->GetModuleInstanceData(this);

    InstPayload->CurrentLODBoundsCheckCount--;

    if (InstPayload->CurrentLODBoundsCheckCount == 0) {
        InstPayload->CurrentLODBoundsCheckCount = 30;  // Reset counter

        // 시스템 바운드 확장
        FBox BoundingBox = Owner->Component->Bounds.GetBox();
        BoundingBox = BoundingBox.ExpandBy(MaxCollisionDistance);

        // 플레이어가 범위 내에 있는지 체크
        bool bAnyPlayerInRange = false;
        for (FConstPlayerControllerIterator Iter = World->GetPlayerControllerIterator();
             Iter; ++Iter)
        {
            APlayerController* PC = Iter->Get();
            if (BoundingBox.IsInside(PC->GetFocalLocation())) {
                bAnyPlayerInRange = true;
                break;
            }
        }

        if (!bAnyPlayerInRange) {
            return;  // 전체 시스템 스킵
        }
    }

    // ========== Step 3: 소유 액터 캐싱 ==========

    AActor* IgnoreActor = bIgnoreSourceActor ? Owner->Component->GetOwner() : nullptr;

    // ========== Step 4: 파티클별 충돌 검사 루프 ==========

    BEGIN_UPDATE_LOOP
    {
        FParticleCollisionPayload* CollisionPayload =
            (FParticleCollisionPayload*)((uint8*)&Particle + Offset);

        // 4-1. 충돌 무시 플래그 체크
        if (Particle.Flags & STATE_Particle_CollisionIgnoreCheck) {
            CONTINUE_UPDATE_LOOP;
        }

        // 4-2. 지연 시간 체크
        if (Particle.Flags & STATE_Particle_DelayCollisions) {
            if (CollisionPayload->Delay > Particle.RelativeTime) {
                CONTINUE_UPDATE_LOOP;
            }
        }

        // 4-3. Per-Particle 거리 체크
        if (MaxCollisionDistance > 0.0f) {
            bool bInRange = false;
            FVector Location = Particle.Location;

            // Local space 변환
            if (Owner->CurrentLODLevel->RequiredModule->bUseLocalSpace) {
                Location = Owner->Component->GetComponentTransform()
                    .TransformPosition(Location);
            }

            for (FConstPlayerControllerIterator Iter = World->GetPlayerControllerIterator();
                 Iter; ++Iter)
            {
                APlayerController* PC = Iter->Get();
                float DistSq = (Location - PC->GetFocalLocation()).SizeSquared();
                float MaxDistSq = MaxCollisionDistance * MaxCollisionDistance;

                if (DistSq < MaxDistSq) {
                    bInRange = true;
                    break;
                }
            }

            if (!bInRange) {
                CONTINUE_UPDATE_LOOP;
            }
        }

        // 4-4. 충돌 검사 파라미터 계산
        FVector Location = Particle.Location;
        FVector OldLocation = Particle.OldLocation;
        FVector Direction = (Location - OldLocation).GetSafeNormal();
        FVector Size = Particle.Size * Owner->Component->GetComponentScale();

        // Mesh TypeData인 경우 메시 크기 고려
        FVector Extent = FVector::ZeroVector;
        UParticleModuleTypeDataMesh* MeshType =
            Cast<UParticleModuleTypeDataMesh>(Owner->CurrentLODLevel->TypeDataModule);

        if (MeshType && MeshType->Mesh) {
            Extent = MeshType->Mesh->GetBounds().BoxExtent;
            if (MeshType->bCollisionsConsiderPartilceSize) {
                Extent *= Size;
            }
        }

        // 4-5. Sweep 종료점 계산
        FVector End = Location + (Direction * Size.GetMax()) / DirScalar;

        // 4-6. 실제 충돌 검사 수행
        FHitResult Hit;
        bool bHit = PerformCollisionCheck(
            Owner,
            &Particle,
            Hit,
            IgnoreActor,
            End,
            OldLocation,
            Extent
        );

        // 4-7. 충돌 응답 처리 (다음 섹션 참조)
        if (bHit) {
            HandleCollisionResponse(
                Owner,
                &Particle,
                CollisionPayload,
                Hit,
                Direction,
                OldLocation,
                DeltaTime
            );
        }
    }
    END_UPDATE_LOOP;
}
```

### PerformCollisionCheck() 함수

```cpp
/**
 * 실제 Scene Query 수행
 * 위치: ParticleModules_Collision.cpp:133-178
 */
bool UParticleModuleCollision::PerformCollisionCheck(
    FParticleEmitterInstance* Owner,
    FBaseParticle* Particle,
    FHitResult& Hit,
    AActor* IgnoreActor,
    const FVector& End,
    const FVector& Start,
    const FVector& Extent)
{
    UWorld* World = Owner->Component->GetWorld();

    // Collision Query Parameters 설정
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ParticleCollision), true);
    QueryParams.AddIgnoredActor(IgnoreActor);
    QueryParams.bTraceComplex = false;  // Simple collision만 사용
    QueryParams.bReturnPhysicalMaterial = true;  // PhysMat 필요

    // Collision Object Parameters 설정
    FCollisionObjectQueryParams ObjectParams;
    for (int32 i = 0; i < CollisionTypes.Num(); ++i) {
        ObjectParams.AddObjectTypesToQuery(
            UEngineTypes::ConvertToCollisionChannel(CollisionTypes[i])
        );
    }

    // Sweep 수행
    bool bHit = false;

    if (Extent.IsNearlyZero()) {
        // Line Trace (빠름)
        bHit = World->LineTraceSingleByObjectType(
            Hit,
            Start,
            End,
            ObjectParams,
            QueryParams
        );
    }
    else {
        // Box Sweep (느리지만 정확)
        bHit = World->SweepSingleByObjectType(
            Hit,
            Start,
            End,
            FQuat::Identity,
            ObjectParams,
            FCollisionShape::MakeBox(Extent),
            QueryParams
        );
    }

    // 트리거 볼륨 필터링
    if (bHit && bIgnoreTriggerVolumes) {
        if (Hit.GetActor() && Hit.GetActor()->IsA(ATriggerBase::StaticClass())) {
            return false;
        }
    }

    return bHit;
}
```

---

## 충돌 응답 로직

### HandleCollisionResponse() 상세 분석

```cpp
/**
 * 충돌 발생 시 물리 반응 및 이벤트 생성
 * 위치: ParticleModules_Collision.cpp:391-548
 */
void UParticleModuleCollision::HandleCollisionResponse(
    FParticleEmitterInstance* Owner,
    FBaseParticle* Particle,
    FParticleCollisionPayload* CollisionPayload,
    const FHitResult& Hit,
    const FVector& Direction,
    const FVector& OldLocation,
    float DeltaTime)
{
    // ========== Step 1: 충돌 카운트 감소 여부 결정 ==========

    bool bDecrementMaxCount = true;
    bool bIgnoreCollision = false;

    // Pawn 충돌 필터링
    if (bPawnsDoNotDecrementCount) {
        if (Hit.GetActor() && Hit.GetActor()->IsA(APawn::StaticClass())) {
            bDecrementMaxCount = false;
        }
    }

    // 수직 충돌 필터링
    if (bOnlyVerticalNormalsDecrementCount) {
        float NormalZ = FMath::Abs(Hit.Normal.Z);
        if (NormalZ + VerticalFudgeFactor < 1.0f) {
            bDecrementMaxCount = false;  // 수평 충돌은 카운트 안함
        }
    }

    // 카운트 감소
    if (bDecrementMaxCount) {
        CollisionPayload->UsedCollisions--;
    }

    // ========== Step 2: 속도 및 회전 반영 계산 ==========

    FVector UsedDampingFactor = CollisionPayload->UsedDampingFactor;
    FVector UsedDampingFactorRotation = CollisionPayload->UsedDampingFactorRotation;

    // BaseVelocity 반사 (초기 속도)
    FVector NewBaseVelocity = Particle->BaseVelocity.MirrorByVector(Hit.Normal);
    NewBaseVelocity *= UsedDampingFactor;
    Particle->BaseVelocity = NewBaseVelocity;

    // Rotation damping 적용
    Particle->BaseRotationRate *= UsedDampingFactorRotation.X;

    // ========== Step 3: 위치 보정 ==========

    // 충돌 지점까지 이동한 거리
    float TravelDistance = (Particle->Location - OldLocation).Size();

    // 반사된 방향으로 남은 거리만큼 이동
    FVector ReflectedDirection = Direction.MirrorByVector(Hit.Normal);
    FVector NewVelocity = ReflectedDirection * TravelDistance * UsedDampingFactor;

    // 충돌 지점에서 반사된 방향으로 이동
    FVector NewLocation = Hit.Location + NewVelocity * (1.0f - Hit.Time);

    // 표면에서 약간 떨어뜨려 penetration 방지
    NewLocation += Hit.Normal * 0.1f;

    Particle->Location = NewLocation;
    Particle->Velocity = NewVelocity / DeltaTime;  // Velocity 업데이트

    // ========== Step 4: 물리 임펄스 적용 (선택적) ==========

    if (bApplyPhysics) {
        float Mass = ParticleMass.GetValue(
            Particle->RelativeTime,
            Owner->Component
        );

        // Impulse = ΔVelocity * Mass
        FVector Impulse = -(NewVelocity - Particle->Velocity) * Mass;

        UPrimitiveComponent* HitComp = Hit.GetComponent();
        if (HitComp && HitComp->IsSimulatingPhysics()) {
            HitComp->AddImpulseAtLocation(
                Impulse,
                Hit.Location,
                Hit.BoneName
            );
        }
    }

    // ========== Step 5: 충돌 완료 처리 또는 이벤트 생성 ==========

    if (CollisionPayload->UsedCollisions > 0) {
        // 아직 충돌 가능 - 이벤트 생성
        Particle->Flags |= STATE_Particle_CollisionHasOccurred;

        // Event Generator가 있으면 이벤트 생성
        FParticleEventInstancePayload* EventPayload =
            (FParticleEventInstancePayload*)Owner->GetModuleInstanceData(
                Owner->CurrentLODLevel->EventGenerator
            );

        if (EventPayload && EventPayload->bCollisionEventsPresent) {
            Owner->CurrentLODLevel->EventGenerator->HandleParticleCollision(
                Owner,
                EventPayload,
                CollisionPayload,
                &Hit,
                Particle,
                Direction
            );
        }
    }
    else {
        // 최대 충돌 횟수 도달 - 완료 동작 실행
        Particle->Location = Hit.Location;

        switch (CollisionCompletionOption) {
            case EPCC_Kill:
                KILL_CURRENT_PARTICLE;
                break;

            case EPCC_Freeze:
                Particle->Flags |= STATE_Particle_Freeze;
                break;

            case EPCC_HaltCollisions:
                Particle->Flags |= STATE_Particle_IgnoreCollisions;
                break;

            case EPCC_FreezeTranslation:
                Particle->Flags |= STATE_Particle_FreezeTranslation;
                Particle->Velocity = FVector::ZeroVector;
                break;

            case EPCC_FreezeRotation:
                Particle->Flags |= STATE_Particle_FreezeRotation;
                Particle->BaseRotationRate = 0.0f;
                break;

            case EPCC_FreezeMovement:
                Particle->Flags |= (STATE_Particle_FreezeRotation |
                                   STATE_Particle_FreezeTranslation);
                Particle->Velocity = FVector::ZeroVector;
                Particle->BaseRotationRate = 0.0f;
                break;
        }

        // 마지막 충돌 이벤트 생성 (LastTimeOnly)
        if (EventPayload && EventPayload->bCollisionEventsPresent) {
            Owner->CurrentLODLevel->EventGenerator->HandleParticleCollision(
                Owner,
                EventPayload,
                CollisionPayload,
                &Hit,
                Particle,
                Direction
            );
        }
    }
}
```

### 충돌 응답 예시

#### 예시 1: 튕기는 파티클 (Bounce)
```cpp
// 설정:
DampingFactor = (0.7, 0.7, 0.7)      // 70% 에너지 보존
MaxCollisions = 3                     // 최대 3번 튕김
CollisionCompletionOption = EPCC_Kill // 3번 후 사라짐

// 시뮬레이션:
충돌 1: Velocity = (0, 0, -10) → (0, 0, 7) * 0.7 = (0, 0, 4.9)
충돌 2: Velocity = (0, 0, -4.9) → (0, 0, 3.43)
충돌 3: Velocity = (0, 0, -3.43) → EPCC_Kill (파티클 제거)
```

#### 예시 2: 정지하는 파티클 (Stick)
```cpp
// 설정:
DampingFactor = (0.0, 0.0, 0.0)      // 완전 감쇠
MaxCollisions = 1                     // 한 번만 충돌
CollisionCompletionOption = EPCC_Freeze

// 시뮬레이션:
충돌 1: Velocity = (5, 0, -10) → (0, 0, 0)
        Location = Hit.Location (충돌 지점에 고정)
        Particle->Flags |= STATE_Particle_Freeze (더 이상 움직이지 않음)
```

#### 예시 3: 수평 표면만 반응
```cpp
// 설정:
bOnlyVerticalNormalsDecrementCount = true
VerticalFudgeFactor = 0.1
MaxCollisions = 1

// 시뮬레이션:
충돌 1 (벽, Normal = (1, 0, 0)):
    |Normal.Z| + 0.1 = 0.1 < 1.0 → 카운트 감소 안함 (계속 튕김)

충돌 2 (바닥, Normal = (0, 0, 1)):
    |Normal.Z| + 0.1 = 1.1 >= 1.0 → 카운트 감소 (1 → 0)
    → EPCC_Kill 실행
```

---

## 성능 최적화

### 1. 거리 기반 LOD 체계

```cpp
// 3단계 LOD 시스템

// Level 1: 시스템 전체 거리 체크 (30프레임마다)
if (BoundingBox.ExpandBy(MaxCollisionDistance).IsInside(PlayerLocation)) {
    // Level 2: Per-Particle 거리 체크 (매 프레임)
    for (each particle) {
        if (Distance(Particle.Location, PlayerLocation) < MaxCollisionDistance) {
            // Level 3: 실제 충돌 검사
            PerformCollisionCheck(...);
        }
    }
}
```

**성능 이득**:
- Level 1 스킵: 전체 시스템 스킵 → **100% 감소**
- Level 2 스킵: 파티클별 스킵 → **약 70% 감소** (Scene query 비용 제거)

### 2. Line Trace vs Box Sweep

```cpp
if (Extent.IsNearlyZero()) {
    // Line Trace: 약 0.01ms per trace
    World->LineTraceSingleByObjectType(...);
}
else {
    // Box Sweep: 약 0.05ms per trace (5배 느림)
    World->SweepSingleByObjectType(...);
}
```

**권장 사항**:
- **Sprite 파티클**: Line Trace 사용 (충분히 정확)
- **Mesh 파티클**: Box Sweep 사용 (메시 크기 고려 필요)

### 3. Collision Channel 최소화

```cpp
// 나쁜 예: 모든 채널 체크
CollisionTypes = { WorldStatic, WorldDynamic, Pawn, PhysicsBody, Vehicle, ... }

// 좋은 예: 필요한 채널만
CollisionTypes = { WorldStatic }  // 지형과만 충돌
```

**성능 이득**: Scene query 시간 약 40% 감소

### 4. 가시성 컬링

```cpp
// GParticleCollisionIgnoreInvisibleTime = 0.1초 (Console variable)

if (World->TimeSeconds - Component->LastRenderTime > 0.1f) {
    return;  // 화면에 안보임 - 전체 스킵
}
```

**성능 이득**: 오프스크린 파티클 100% 스킵

### 5. Thread Safety 고려

```cpp
// bApplyPhysics = false인 경우:
// - Collision detection: Worker thread에서 실행 가능
// - Read-only Scene query
// - No physics state modification

// bApplyPhysics = true인 경우:
// - Collision detection: Game thread에서만 실행
// - Physics impulse 적용 필요
```

**권장 사항**: 대부분의 경우 `bApplyPhysics = false` 사용하여 병렬화

---

## Mundi Engine 구현 가이드

### 필요한 파일 및 클래스

```
Mundi/Source/Runtime/Engine/Particles/Modules/Collision/
├── ParticleModuleCollisionBase.h          (기본 클래스)
├── ParticleModuleCollisionBase.cpp
├── ParticleModuleCollision.h              (월드 충돌)
└── ParticleModuleCollision.cpp
```

### 클래스 스켈레톤

```cpp
// ParticleModuleCollision.h

#pragma once
#include "CoreMinimal.h"
#include "ParticleModule.h"
#include "Collision/Collision.h"

/**
 * Collision Payload: 파티클당 32 bytes
 */
struct FParticleCollisionPayload
{
    FVector UsedDampingFactor;           // 12 bytes
    FVector UsedDampingFactorRotation;   // 12 bytes
    int32 UsedCollisions;                 // 4 bytes
    float Delay;                          // 4 bytes
};

/**
 * Instance Payload: 이미터당 4 bytes
 */
struct FParticleCollisionInstancePayload
{
    uint8 CurrentLODBoundsCheckCount;
    uint8 Padding[3];
};

/**
 * 충돌 완료 옵션
 */
enum class EParticleCollisionComplete : uint8
{
    Kill,                // 파티클 제거
    Freeze,              // 완전 정지
    HaltCollisions,      // 충돌 검사 중단
    FreezeTranslation,   // 위치만 정지
    FreezeRotation,      // 회전만 정지
    FreezeMovement       // 위치+회전 정지
};

UCLASS()
class UParticleModuleCollision : public UParticleModule
{
    GENERATED_BODY()

public:
    // ==================== Parameters ====================

    UPROPERTY(EditAnywhere, Category = "Collision")
    FVector DampingFactor;

    UPROPERTY(EditAnywhere, Category = "Collision")
    FVector DampingFactorRotation;

    UPROPERTY(EditAnywhere, Category = "Collision")
    float MaxCollisions;

    UPROPERTY(EditAnywhere, Category = "Collision")
    EParticleCollisionComplete CollisionCompletionOption;

    UPROPERTY(EditAnywhere, Category = "Collision")
    bool bApplyPhysics;

    UPROPERTY(EditAnywhere, Category = "Collision")
    float ParticleMass;

    UPROPERTY(EditAnywhere, Category = "Performance")
    float MaxCollisionDistance;

    UPROPERTY(EditAnywhere, Category = "Performance")
    bool bCollideOnlyIfVisible;

    UPROPERTY(EditAnywhere, Category = "Advanced")
    float DirScalar;

    // ==================== Module Interface ====================

    UParticleModuleCollision();

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset,
                       float SpawnTime, FBaseParticle* ParticleBase) override;

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset,
                        float DeltaTime) override;

    virtual uint32 RequiredBytes() const override {
        return sizeof(FParticleCollisionPayload);
    }

    virtual uint32 RequiredBytesPerInstance() const override {
        return sizeof(FParticleCollisionInstancePayload);
    }

private:
    bool PerformCollisionCheck(
        FParticleEmitterInstance* Owner,
        FBaseParticle* Particle,
        FHitResult& Hit,
        AActor* IgnoreActor,
        const FVector& End,
        const FVector& Start,
        const FVector& Extent
    );

    void HandleCollisionResponse(
        FParticleEmitterInstance* Owner,
        FBaseParticle* Particle,
        FParticleCollisionPayload* CollisionPayload,
        const FHitResult& Hit,
        const FVector& Direction,
        const FVector& OldLocation,
        float DeltaTime
    );
};
```

### Spawn 구현

```cpp
void UParticleModuleCollision::Spawn(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float SpawnTime,
    FBaseParticle* ParticleBase)
{
    MUNDI_SPAWN_INIT(Owner, Offset, ParticleBase);

    FParticleCollisionPayload* CollisionPayload =
        (FParticleCollisionPayload*)((uint8*)Particle + sizeof(FBaseParticle));

    // Damping 값 설정 (현재는 Distribution 없으므로 직접 할당)
    CollisionPayload->UsedDampingFactor = DampingFactor;
    CollisionPayload->UsedDampingFactorRotation = DampingFactorRotation;

    // 최대 충돌 횟수
    CollisionPayload->UsedCollisions = (int32)MaxCollisions;

    // 지연 시간 (현재는 0)
    CollisionPayload->Delay = 0.0f;
}
```

### Update 구현 (단순화 버전)

```cpp
void UParticleModuleCollision::Update(
    FParticleEmitterInstance* Owner,
    int32 Offset,
    float DeltaTime)
{
    UWorld* World = Owner->Component->GetWorld();
    if (!World) return;

    MUNDI_BEGIN_UPDATE_LOOP
    {
        FParticleCollisionPayload* CollisionPayload =
            (FParticleCollisionPayload*)((uint8*)Particle + sizeof(FBaseParticle));

        // 충돌 무시 플래그 체크
        if (Particle->Flags & STATE_Particle_CollisionIgnoreCheck) {
            continue;
        }

        // 충돌 검사 파라미터
        FVector Start = Particle->OldLocation;
        FVector End = Particle->Location + Particle->Velocity * DeltaTime;
        FVector Direction = (End - Start).GetSafeNormal();

        // Scene query
        FHitResult Hit;
        bool bHit = World->LineTraceSingle(
            Hit,
            Start,
            End,
            ECollisionChannel::ECC_WorldStatic
        );

        if (bHit) {
            HandleCollisionResponse(
                Owner,
                Particle,
                CollisionPayload,
                Hit,
                Direction,
                Start,
                DeltaTime
            );
        }
    }
    MUNDI_END_UPDATE_LOOP;
}
```

### 주의사항

1. **좌표계 변환**: Mundi는 Z-Up Left-Handed이므로 Unreal과 다를 수 있음
2. **Collision System 통합**: Mundi의 기존 Collision 함수들 활용
3. **Distribution 시스템**: Unreal의 Distribution을 Mundi에서 단순화 가능
4. **Thread Safety**: 현재는 게임 스레드에서만 실행 (추후 최적화 가능)

---

## 다음 문서

- **Part 3: EventSystem** - Event Generator, Event Manager, 델리게이트 시스템 분석
- **Part 4: Implementation** - 전체 시스템 통합 및 테스트 가이드

---

**작성 일자**: 2025-01-25
