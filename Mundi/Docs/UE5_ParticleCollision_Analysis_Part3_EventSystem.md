# UE5 Particle Collision 시스템 분석 Part 3: Event System

## 목차
1. [이벤트 시스템 개요](#이벤트-시스템-개요)
2. [UParticleModuleEventGenerator](#uparticlemoduleeventgenerator)
3. [FParticleEventCollideData](#fparticleeventcollidedata)
4. [UParticleSystemComponent 이벤트 저장](#uparticlesystemcomponent-이벤트-저장)
5. [AParticleEventManager](#aparticleeventmanager)
6. [델리게이트 시스템](#델리게이트-시스템)
7. [이벤트 플로우 상세](#이벤트-플로우-상세)
8. [EventReceiver (파티클 간 통신)](#eventreceiver-파티클-간-통신)
9. [Mundi Engine 구현 가이드](#mundi-engine-구현-가이드)

---

## 이벤트 시스템 개요

Unreal Engine의 Particle Event 시스템은 **생성(Generation) → 저장(Storage) → 디스패치(Dispatch) → 수신(Reception)** 4단계로 구성됩니다.

### 설계 원칙

1. **Separation of Concerns**: 물리 시뮬레이션과 이벤트 알림 분리
2. **Batch Processing**: 프레임당 이벤트를 축적 후 일괄 처리
3. **Thread Safety**: 워커 스레드에서 이벤트 생성, 게임 스레드에서 디스패치
4. **Flexibility**: 다양한 필터링 옵션 및 커스텀 페이로드 지원

### 이벤트 타입

```cpp
enum EParticleEventType
{
    EPET_Any        = 0,  // 모든 이벤트 (수신자 필터링용)
    EPET_Spawn      = 1,  // 파티클 스폰 이벤트
    EPET_Death      = 2,  // 파티클 사망 이벤트
    EPET_Collision  = 3,  // 파티클 충돌 이벤트
    EPET_Burst      = 4,  // Burst 스폰 이벤트
    EPET_Blueprint  = 5   // 블루프린트 커스텀 이벤트
};
```

---

## UParticleModuleEventGenerator

### 파일 위치
- **헤더**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\Event\ParticleModuleEventGenerator.h`
- **구현**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleModules_Event.cpp`

### 클래스 구조

```cpp
/**
 * 다양한 파티클 이벤트를 생성하는 모듈
 * 한 LOD Level에 하나만 존재 가능
 */
UCLASS()
class UParticleModuleEventGenerator : public UParticleModuleEventBase
{
    GENERATED_BODY()

public:
    /** 이벤트 설정 배열 - 여러 이벤트 타입을 동시에 생성 가능 */
    UPROPERTY(EditAnywhere, Category = "Events")
    TArray<FParticleEvent_GenerateInfo> Events;

    // ==================== Module Interface ====================

    virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset,
                       float SpawnTime, FBaseParticle* ParticleBase) override;

    virtual void Update(FParticleEmitterInstance* Owner, int32 Offset,
                        float DeltaTime) override;

    virtual uint32 RequiredBytesPerInstance() const override;

    // ==================== Event Generation Methods ====================

    /** 충돌 이벤트 생성 (Collision 모듈에서 호출) */
    bool HandleParticleCollision(
        FParticleEmitterInstance* Owner,
        FParticleEventInstancePayload* EventPayload,
        FParticleCollisionPayload* CollisionPayload,
        FHitResult* Hit,
        FBaseParticle* Particle,
        const FVector& CollideDirection
    );

    /** 스폰 이벤트 생성 */
    bool HandleParticleSpawned(
        FParticleEmitterInstance* Owner,
        FParticleEventInstancePayload* EventPayload,
        FBaseParticle* Particle
    );

    /** 사망 이벤트 생성 */
    bool HandleParticleDeath(
        FParticleEmitterInstance* Owner,
        FParticleEventInstancePayload* EventPayload,
        FBaseParticle* Particle
    );
};
```

### 이벤트 설정 구조체

```cpp
/**
 * 단일 이벤트 생성 규칙
 * 위치: ParticleModuleEventGenerator.h:20-50
 */
USTRUCT()
struct FParticleEvent_GenerateInfo
{
    GENERATED_BODY()

    /** 이벤트 타입 */
    UPROPERTY(EditAnywhere, Category = "Event")
    TEnumAsByte<EParticleEventType> Type;

    /**
     * 이벤트 생성 빈도
     * 0 = 매번, 1 = 매번, 2 = 2번째마다, 3 = 3번째마다, ...
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    int32 Frequency;

    /**
     * 파티클별 빈도
     * 0 = 파티클마다 추적 안함, N = 파티클당 N번째마다
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    int32 ParticleFrequency;

    /**
     * 첫 번째 충돌만 이벤트 생성 (Collision만 해당)
     * true = 파티클당 첫 충돌만, false = 모든 충돌
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    uint32 FirstTimeOnly:1;

    /**
     * 마지막 충돌만 이벤트 생성 (Collision만 해당)
     * true = UsedCollisions == 0일 때만
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    uint32 LastTimeOnly:1;

    /**
     * 반사된 충돌 벡터 사용 (Collision만 해당)
     * true = Direction.MirrorByVector(Normal), false = Direction
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    uint32 UseReflectedImpactVector:1;

    /**
     * Orbit 오프셋 포함 여부
     * true = 파티클 위치에 Orbit 오프셋 추가
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    uint32 bUseOrbitOffset:1;

    /**
     * 커스텀 이벤트 이름 (수신자 필터링용)
     * 예: "GroundHit", "WaterSplash", "CriticalHit"
     */
    UPROPERTY(EditAnywhere, Category = "Event")
    FName CustomName;

    /**
     * 게임 특화 페이로드 데이터
     * 블루프린트/C++로 전달할 커스텀 데이터
     */
    UPROPERTY(EditAnywhere, Instanced, Category = "Event")
    TArray<UParticleModuleEventSendToGame*> ParticleModuleEventsToSendToGame;
};
```

### Per-Instance 데이터

```cpp
/**
 * 이미터 인스턴스당 이벤트 추적 데이터
 * 위치: ParticleModules_Event.cpp:31-39
 */
struct FParticleEventInstancePayload
{
    // 이벤트 타입별 플래그
    uint32 bSpawnEventsPresent:1;      // Spawn 이벤트 생성 가능?
    uint32 bDeathEventsPresent:1;      // Death 이벤트 생성 가능?
    uint32 bCollisionEventsPresent:1;  // Collision 이벤트 생성 가능?
    uint32 bBurstEventsPresent:1;      // Burst 이벤트 생성 가능?

    // 빈도 추적 카운터 (Frequency > 0인 경우 사용)
    int32 SpawnTrackingCount;          // 총 스폰 이벤트 발생 횟수
    int32 DeathTrackingCount;          // 총 사망 이벤트 발생 횟수
    int32 CollisionTrackingCount;      // 총 충돌 이벤트 발생 횟수
    int32 BurstTrackingCount;          // 총 버스트 이벤트 발생 횟수
};
```

### HandleParticleCollision() 상세 분석

```cpp
/**
 * 충돌 이벤트 생성 로직
 * 위치: ParticleModules_Event.cpp:177-231
 */
bool UParticleModuleEventGenerator::HandleParticleCollision(
    FParticleEmitterInstance* Owner,
    FParticleEventInstancePayload* EventPayload,
    FParticleCollisionPayload* CollisionPayload,
    FHitResult* Hit,
    FBaseParticle* CollideParticle,
    FVector& CollideDirection)
{
    // 충돌 카운터 증가
    EventPayload->CollisionTrackingCount++;

    bool bProcessed = false;

    // 모든 이벤트 설정을 순회
    for (int32 EventIndex = 0; EventIndex < Events.Num(); EventIndex++)
    {
        FParticleEvent_GenerateInfo& EventGenInfo = Events[EventIndex];

        // ========== Step 1: 이벤트 타입 체크 ==========
        if (EventGenInfo.Type != EPET_Collision) {
            continue;  // Collision 이벤트만 처리
        }

        // ========== Step 2: FirstTimeOnly 필터 ==========
        if (EventGenInfo.FirstTimeOnly)
        {
            // 이미 충돌한 적 있으면 스킵
            if (CollideParticle->Flags & STATE_Particle_CollisionHasOccurred)
            {
                continue;
            }
        }

        // ========== Step 3: LastTimeOnly 필터 ==========
        if (EventGenInfo.LastTimeOnly)
        {
            // 아직 충돌 횟수가 남아있으면 스킵
            if (CollisionPayload->UsedCollisions != 0)
            {
                continue;
            }
        }

        // ========== Step 4: Frequency 필터 ==========
        if (EventGenInfo.Frequency > 0)
        {
            // N번째 충돌마다만 이벤트 생성
            if ((EventPayload->CollisionTrackingCount % EventGenInfo.Frequency) != 0)
            {
                continue;
            }
        }

        // ========== Step 5: 이벤트 방향 계산 ==========
        FVector EventDirection = CollideDirection;

        if (EventGenInfo.UseReflectedImpactVector)
        {
            // 반사된 방향 사용
            EventDirection = CollideDirection.MirrorByVector(Hit->Normal);
        }

        // ========== Step 6: 이벤트 위치 계산 ==========
        FVector EventLocation = Hit->Location;

        if (EventGenInfo.bUseOrbitOffset)
        {
            // Orbit 오프셋 적용 (Orbit 모듈이 있는 경우)
            // ... (Orbit 계산 로직)
        }

        // ========== Step 7: 이벤트 보고 ==========
        Owner->Component->ReportEventCollision(
            EventGenInfo.CustomName,                          // 이벤트 이름
            Owner->EmitterTime,                               // 이미터 시간
            EventLocation,                                    // 충돌 위치
            EventDirection,                                   // 충돌 방향
            CollideParticle->Velocity,                        // 파티클 속도
            EventGenInfo.ParticleModuleEventsToSendToGame,   // 페이로드
            CollideParticle->RelativeTime,                    // 파티클 생명 시간
            Hit->Normal,                                      // 충돌 노말
            Hit->Time,                                        // Hit time (0-1)
            Hit->Item,                                        // Primitive item
            Hit->BoneName,                                    // 본 이름
            Hit->PhysMaterial.Get()                           // 물리 머티리얼
        );

        bProcessed = true;
    }

    return bProcessed;
}
```

### 이벤트 필터링 예시

#### 예시 1: 첫 충돌만 이벤트 생성
```cpp
FParticleEvent_GenerateInfo EventConfig;
EventConfig.Type = EPET_Collision;
EventConfig.FirstTimeOnly = true;
EventConfig.CustomName = "FirstImpact";

// 결과:
// 충돌 1: 이벤트 생성 ✓
// 충돌 2: 스킵 (FirstTimeOnly)
// 충돌 3: 스킵 (FirstTimeOnly)
```

#### 예시 2: 마지막 충돌만 이벤트 생성
```cpp
FParticleEvent_GenerateInfo EventConfig;
EventConfig.Type = EPET_Collision;
EventConfig.LastTimeOnly = true;
EventConfig.CustomName = "FinalImpact";

// ParticleModuleCollision::MaxCollisions = 3

// 결과:
// 충돌 1 (UsedCollisions=3→2): 스킵 (LastTimeOnly)
// 충돌 2 (UsedCollisions=2→1): 스킵 (LastTimeOnly)
// 충돌 3 (UsedCollisions=1→0): 이벤트 생성 ✓
```

#### 예시 3: 2번째 충돌마다 이벤트 생성
```cpp
FParticleEvent_GenerateInfo EventConfig;
EventConfig.Type = EPET_Collision;
EventConfig.Frequency = 2;
EventConfig.CustomName = "EveryOtherHit";

// 결과:
// 충돌 1 (Count=1, 1%2=1): 스킵
// 충돌 2 (Count=2, 2%2=0): 이벤트 생성 ✓
// 충돌 3 (Count=3, 3%2=1): 스킵
// 충돌 4 (Count=4, 4%2=0): 이벤트 생성 ✓
```

#### 예시 4: 복합 필터 (첫 충돌 + 반사 방향)
```cpp
FParticleEvent_GenerateInfo EventConfig;
EventConfig.Type = EPET_Collision;
EventConfig.FirstTimeOnly = true;
EventConfig.UseReflectedImpactVector = true;
EventConfig.CustomName = "FirstBounce";

// 결과:
// 충돌 1:
//   - Direction = Original → Reflected
//   - 이벤트 생성 ✓
// 충돌 2: 스킵 (FirstTimeOnly)
```

---

## FParticleEventCollideData

### 데이터 구조 계층

```cpp
/**
 * 기본 이벤트 데이터 (모든 이벤트 타입 공통)
 * 위치: ParticleSystemComponent.h:250-265
 */
USTRUCT()
struct FParticleEventData
{
    GENERATED_BODY()

    /** 이벤트 타입 (EPET_Collision 등) */
    UPROPERTY()
    int32 Type;

    /** 커스텀 이벤트 이름 */
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

    /** 게임 특화 페이로드 데이터 */
    UPROPERTY()
    TArray<UParticleModuleEventSendToGame*> EventData;
};

/**
 * 기존 파티클 이벤트 데이터 (Collision, Death)
 * 위치: ParticleSystemComponent.h:270-280
 */
USTRUCT()
struct FParticleExistingData : public FParticleEventData
{
    GENERATED_BODY()

    /** 파티클 생명 시간 (0.0 ~ 1.0) */
    UPROPERTY()
    float ParticleTime;

    /** 파티클 이동 방향 */
    UPROPERTY()
    FVector Direction;
};

/**
 * 충돌 이벤트 데이터
 * 위치: ParticleSystemComponent.h:286-310
 */
USTRUCT()
struct FParticleEventCollideData : public FParticleExistingData
{
    GENERATED_BODY()

    /** 충돌 표면 노말 벡터 */
    UPROPERTY()
    FVector Normal;

    /** Hit time (0.0 ~ 1.0, ray 상의 충돌 지점) */
    UPROPERTY()
    float Time;

    /** Primitive item 인덱스 (복잡한 메시의 경우) */
    UPROPERTY()
    int32 Item;

    /** 충돌한 본 이름 (Skeletal Mesh인 경우) */
    UPROPERTY()
    FName BoneName;

    /** 충돌 지점의 물리 머티리얼 */
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMat;
};
```

### 메모리 크기

```cpp
sizeof(FParticleEventCollideData) ≈ 120 bytes

// 세부:
// - Type (int32): 4 bytes
// - EventName (FName): 8 bytes
// - EmitterTime (float): 4 bytes
// - Location (FVector): 12 bytes
// - Velocity (FVector): 12 bytes
// - EventData (TArray): 16 bytes
// - ParticleTime (float): 4 bytes
// - Direction (FVector): 12 bytes
// - Normal (FVector): 12 bytes
// - Time (float): 4 bytes
// - Item (int32): 4 bytes
// - BoneName (FName): 8 bytes
// - PhysMat (TObjectPtr): 8 bytes
// + Padding: ~12 bytes
```

---

## UParticleSystemComponent 이벤트 저장

### 이벤트 버퍼

```cpp
/**
 * ParticleSystemComponent에 저장되는 이벤트 배열
 * 위치: ParticleSystemComponent.h:777-781
 */
class UParticleSystemComponent : public UPrimitiveComponent
{
    /** 프레임당 축적된 이벤트들 */
    UPROPERTY(Transient)
    TArray<FParticleEventSpawnData> SpawnEvents;

    UPROPERTY(Transient)
    TArray<FParticleEventDeathData> DeathEvents;

    UPROPERTY(Transient)
    TArray<FParticleEventCollideData> CollisionEvents;  // ← 충돌 이벤트

    UPROPERTY(Transient)
    TArray<FParticleEventBurstData> BurstEvents;

    UPROPERTY(Transient)
    TArray<FParticleEventKismetData> KismetEvents;
};
```

### ReportEventCollision() 함수

```cpp
/**
 * 충돌 이벤트를 CollisionEvents 배열에 추가
 * 위치: ParticleSystemComponent.cpp:4921-4940
 */
void UParticleSystemComponent::ReportEventCollision(
    const FName InEventName,
    const float InEmitterTime,
    const FVector InLocation,
    const FVector InDirection,
    const FVector InVelocity,
    const TArray<UParticleModuleEventSendToGame*>& InEventData,
    const float InParticleTime,
    const FVector InNormal,
    const float InTime,
    const int32 InItem,
    const FName InBoneName,
    UPhysicalMaterial* PhysMat)
{
    // 새 이벤트를 배열에 추가 (placement new)
    FParticleEventCollideData* CollideData = new(CollisionEvents) FParticleEventCollideData;

    // 기본 이벤트 데이터 설정
    CollideData->Type = EPET_Collision;
    CollideData->EventName = InEventName;
    CollideData->EmitterTime = InEmitterTime;
    CollideData->Location = InLocation;
    CollideData->Velocity = InVelocity;
    CollideData->EventData = InEventData;

    // 파티클 기존 데이터
    CollideData->Direction = InDirection;
    CollideData->ParticleTime = InParticleTime;

    // 충돌 특화 데이터
    CollideData->Normal = InNormal;
    CollideData->Time = InTime;
    CollideData->Item = InItem;
    CollideData->BoneName = InBoneName;
    CollideData->PhysMat = PhysMat;
}
```

### 이벤트 축적 및 초기화

```cpp
/**
 * 프레임 시작 시 이벤트 버퍼 초기화
 * 위치: ParticleSystemComponent.cpp:2413-2416
 */
void UParticleSystemComponent::ComputeTickComponent_Concurrent(...)
{
    // 이전 프레임 이벤트 클리어
    SpawnEvents.Reset();
    DeathEvents.Reset();
    CollisionEvents.Reset();
    BurstEvents.Reset();

    // ... 파티클 업데이트 ...
    // 업데이트 중 ReportEventCollision() 호출로 이벤트 축적

    // 프레임 종료 - 이벤트는 FinalizeTickComponent()에서 디스패치
}
```

---

## AParticleEventManager

### 파일 위치
- **헤더**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\ParticleEventManager.h`
- **구현**: `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleEventManager.cpp`

### 클래스 구조

```cpp
/**
 * 파티클 이벤트를 게임 코드로 디스패치하는 중앙 관리자
 * World에 하나만 존재 (Singleton 패턴)
 */
UCLASS()
class AParticleEventManager : public AActor
{
    GENERATED_BODY()

public:
    AParticleEventManager(const FObjectInitializer& ObjectInitializer);

    // ==================== Event Dispatch Methods ====================

    /** 스폰 이벤트 처리 */
    virtual void HandleParticleSpawnEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventSpawnData>& SpawnEvents
    );

    /** 사망 이벤트 처리 */
    virtual void HandleParticleDeathEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventDeathData>& DeathEvents
    );

    /** 충돌 이벤트 처리 */
    virtual void HandleParticleCollisionEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventCollideData>& CollisionEvents
    );

    /** 버스트 이벤트 처리 */
    virtual void HandleParticleBurstEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventBurstData>& BurstEvents
    );

    // ==================== Lifecycle ====================

    virtual void BeginDestroy() override;
};
```

### World 통합

```cpp
/**
 * UWorld에 ParticleEventManager 저장
 * 위치: World.h:1450
 */
class UWorld : public UObject
{
    /** 파티클 이벤트 매니저 (World당 하나) */
    UPROPERTY(Transient)
    TObjectPtr<AParticleEventManager> MyParticleEventManager;

    /** EventManager 가져오기 (없으면 생성) */
    AParticleEventManager* GetParticleEventManager();
};

/**
 * GetParticleEventManager() 구현
 * 위치: World.cpp
 */
AParticleEventManager* UWorld::GetParticleEventManager()
{
    if (MyParticleEventManager == nullptr)
    {
        // Spawn event manager actor
        FActorSpawnParameters SpawnParams;
        SpawnParams.Name = TEXT("ParticleEventManager");
        SpawnParams.ObjectFlags |= RF_Transient;

        MyParticleEventManager = SpawnActor<AParticleEventManager>(
            AParticleEventManager::StaticClass(),
            SpawnParams
        );
    }

    return MyParticleEventManager;
}
```

### HandleParticleCollisionEvents() 구현

```cpp
/**
 * 충돌 이벤트 배열을 받아 델리게이트 브로드캐스트
 * 위치: ParticleEventManager.cpp:90-120
 */
void AParticleEventManager::HandleParticleCollisionEvents(
    UParticleSystemComponent* Component,
    const TArray<FParticleEventCollideData>& CollisionEvents)
{
    if (!Component) return;

    // 소유 액터 가져오기 (AEmitter 등)
    AEmitter* Emitter = Cast<AEmitter>(Component->GetOwner());

    // 모든 충돌 이벤트 순회
    for (int32 EventIndex = 0; EventIndex < CollisionEvents.Num(); ++EventIndex)
    {
        const FParticleEventCollideData& CollisionEvent = CollisionEvents[EventIndex];

        // Component 델리게이트 브로드캐스트
        Component->OnParticleCollide.Broadcast(
            CollisionEvent.EventName,
            CollisionEvent.EmitterTime,
            CollisionEvent.ParticleTime,
            CollisionEvent.Location,
            CollisionEvent.Velocity,
            CollisionEvent.Direction,
            CollisionEvent.Normal,
            CollisionEvent.BoneName,
            CollisionEvent.PhysMat
        );

        // Emitter Actor 델리게이트 브로드캐스트 (존재하는 경우)
        if (Emitter)
        {
            Emitter->OnParticleCollide.Broadcast(
                CollisionEvent.EventName,
                CollisionEvent.EmitterTime,
                CollisionEvent.ParticleTime,
                CollisionEvent.Location,
                CollisionEvent.Velocity,
                CollisionEvent.Direction,
                CollisionEvent.Normal,
                CollisionEvent.BoneName,
                CollisionEvent.PhysMat
            );
        }
    }
}
```

---

## 델리게이트 시스템

### 델리게이트 선언

```cpp
/**
 * Collision 이벤트 델리게이트 시그니처
 * 위치: ParticleSystemComponent.h:230-242
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams(
    FParticleCollisionSignature,
    FName, EventName,              // 이벤트 이름
    float, EmitterTime,            // 이미터 시간
    float, ParticleTime,           // 파티클 생명 시간
    FVector, Location,             // 충돌 위치
    FVector, Velocity,             // 파티클 속도
    FVector, Direction,            // 이동 방향
    FVector, Normal,               // 충돌 노말
    FName, BoneName,               // 본 이름
    UPhysicalMaterial*, PhysMat    // 물리 머티리얼
);
```

### Component 델리게이트

```cpp
/**
 * UParticleSystemComponent의 델리게이트
 * 위치: ParticleSystemComponent.h:830
 */
class UParticleSystemComponent : public UPrimitiveComponent
{
    /** 충돌 이벤트 델리게이트 */
    UPROPERTY(BlueprintAssignable, Category = "Particles|Collision")
    FParticleCollisionSignature OnParticleCollide;
};
```

### Emitter Actor 델리게이트

```cpp
/**
 * AEmitter 액터의 델리게이트
 * 위치: Emitter.h:50
 */
class AEmitter : public AActor
{
    /** 충돌 이벤트 델리게이트 (액터 레벨) */
    UPROPERTY(BlueprintAssignable, Category = "Particles|Collision")
    FParticleCollisionSignature OnParticleCollide;
};
```

### 사용 예시

#### C++ 바인딩
```cpp
// .h
UCLASS()
class AMyActor : public AActor
{
    UPROPERTY()
    UParticleSystemComponent* ParticleComp;

    UFUNCTION()
    void OnParticleCollision(
        FName EventName,
        float EmitterTime,
        float ParticleTime,
        FVector Location,
        FVector Velocity,
        FVector Direction,
        FVector Normal,
        FName BoneName,
        UPhysicalMaterial* PhysMat
    );
};

// .cpp
void AMyActor::BeginPlay()
{
    Super::BeginPlay();

    // 델리게이트 바인딩
    ParticleComp->OnParticleCollide.AddDynamic(
        this,
        &AMyActor::OnParticleCollision
    );
}

void AMyActor::OnParticleCollision(
    FName EventName,
    float EmitterTime,
    float ParticleTime,
    FVector Location,
    FVector Velocity,
    FVector Direction,
    FVector Normal,
    FName BoneName,
    UPhysicalMaterial* PhysMat)
{
    UE_LOG(LogTemp, Log, TEXT("Particle collision at %s"), *Location.ToString());

    // 충돌 지점에 사운드 재생
    UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);

    // 충돌 지점에 데칼 생성
    UGameplayStatics::SpawnDecalAtLocation(this, ImpactDecal, FVector(10, 10, 10),
                                           Location, Normal.Rotation());

    // 이벤트 이름별 분기
    if (EventName == "CriticalHit")
    {
        // 크리티컬 히트 처리
    }
    else if (EventName == "WaterSplash")
    {
        // 물 충돌 처리
    }
}
```

#### Blueprint 바인딩
```
1. ParticleSystemComponent 선택
2. Details 패널 → Events → On Particle Collide
3. 이벤트 그래프에 노드 생성
4. Location, Normal 등 파라미터 사용
5. Play Sound at Location, Spawn Emitter at Location 등 연결
```

---

## 이벤트 플로우 상세

### 전체 타임라인

```
FRAME N
════════════════════════════════════════════════════════════

Time 0.000s: ComputeTickComponent_Concurrent() START
────────────────────────────────────────────────────────────
│ CollisionEvents.Reset();  // 이전 프레임 이벤트 클리어
│
│ For each EmitterInstance:
│   FParticleEmitterInstance::Tick(DeltaTime)
│   ├─ SpawnParticles()
│   ├─ RunUpdateModules()  ← Collision 검사 수행
│   │   └─ UParticleModuleCollision::Update()
│   │       └─ For each particle:
│   │           └─ If collision detected:
│   │               ├─ HandleCollisionResponse()
│   │               └─ EventGenerator->HandleParticleCollision()
│   │                   └─ Component->ReportEventCollision()
│   │                       └─ new(CollisionEvents) FParticleEventCollideData
│   │                           ▲
│   │                           │ Event 축적
│   ├─ RunFinalUpdateModules()
│   └─ KillDeadParticles()
│
│ // 이 시점에 CollisionEvents에 모든 이벤트 축적됨
│
Time 0.016s: ComputeTickComponent_Concurrent() END
════════════════════════════════════════════════════════════

Time 0.016s: FinalizeTickComponent() START (Game Thread)
────────────────────────────────────────────────────────────
│ // 이벤트 디스패치
│ AParticleEventManager* EventManager = World->GetParticleEventManager();
│
│ if (CollisionEvents.Num() > 0) {
│     EventManager->HandleParticleCollisionEvents(this, CollisionEvents);
│     ├─ For each event in CollisionEvents:
│     │   ├─ Component->OnParticleCollide.Broadcast(...)
│     │   │   └─ 바인딩된 모든 리스너 호출
│     │   │       ├─ Blueprint 이벤트
│     │   │       ├─ C++ 델리게이트
│     │   │       └─ Lua 스크립트 (Mundi의 경우)
│     │   │
│     │   └─ Emitter->OnParticleCollide.Broadcast(...)
│     │       └─ 액터 레벨 리스너 호출
│ }
│
│ // 이벤트 클리어 (다음 프레임 준비)
│ CollisionEvents.Reset();
│ SpawnEvents.Reset();
│ DeathEvents.Reset();
│ BurstEvents.Reset();
│
Time 0.017s: FinalizeTickComponent() END
════════════════════════════════════════════════════════════

// 다음 프레임에서 반복
```

### Thread Safety 보장

```cpp
// Worker Thread (병렬 가능)
// - 충돌 검사
// - 이벤트 생성
// - TArray::Add() (thread-safe append)

// Game Thread (직렬 필수)
// - 델리게이트 브로드캐스트
// - Blueprint 이벤트 실행
// - Game logic (사운드, VFX, 데미지 등)
```

---

## EventReceiver (파티클 간 통신)

### 개념

EventReceiver 모듈은 **한 이미터의 이벤트를 받아 다른 이미터에 영향을 주는** 시스템입니다.

### UParticleModuleEventReceiver

```cpp
/**
 * 다른 이미터의 이벤트를 수신하여 파티클 생성/제거
 * 위치: ParticleModuleEventReceiver.h
 */
UCLASS()
class UParticleModuleEventReceiver : public UParticleModuleEventBase
{
    /** 수신할 이벤트 타입 */
    UPROPERTY(EditAnywhere, Category = "Event")
    TEnumAsByte<EParticleEventType> EventType;

    /** 특정 이벤트 이름만 수신 (빈 값이면 모든 이벤트) */
    UPROPERTY(EditAnywhere, Category = "Event")
    FName EventName;

    /** 이벤트 발생 시 수행할 동작 */
    UPROPERTY(EditAnywhere, Category = "Event")
    TEnumAsByte<EParticleEventReceiverAction> Action;

    /**
     * 이벤트 처리
     * Component->CollisionEvents에서 이벤트를 가져와 처리
     */
    virtual void ProcessParticleEvent(
        FParticleEmitterInstance* Owner,
        FParticleEventData& EventData,
        float DeltaTime
    );
};

enum EParticleEventReceiverAction
{
    EPERA_SpawnParticles,    // 이벤트 위치에 파티클 스폰
    EPERA_KillParticles,     // 반경 내 파티클 제거
    EPERA_ModifyParticles    // 파티클 속성 변경
};
```

### EventReceiver 실행 흐름

```cpp
/**
 * FParticleEmitterInstance::Tick()에서 EventReceiver 실행
 * 위치: ParticleEmitterInstance.cpp:2747-2753
 */
void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
    // ... 파티클 업데이트 ...

    // EventReceiver 모듈이 있으면 실행
    if (CurrentLODLevel->EventReceiverModules.Num() > 0)
    {
        for (int32 ModuleIdx = 0; ModuleIdx < CurrentLODLevel->EventReceiverModules.Num(); ++ModuleIdx)
        {
            UParticleModuleEventReceiver* EventRcvr = CurrentLODLevel->EventReceiverModules[ModuleIdx];

            // Collision 이벤트 처리
            if (EventRcvr->WillProcessParticleEvent(EPET_Collision))
            {
                TArray<FParticleEventCollideData>& CollisionEvents = Component->CollisionEvents;

                if (CollisionEvents.Num() > 0)
                {
                    for (int32 EventIndex = 0; EventIndex < CollisionEvents.Num(); ++EventIndex)
                    {
                        // 이벤트 이름 필터링
                        if (EventRcvr->EventName.IsNone() ||
                            EventRcvr->EventName == CollisionEvents[EventIndex].EventName)
                        {
                            // 이벤트 처리
                            EventRcvr->ProcessParticleEvent(
                                this,
                                CollisionEvents[EventIndex],
                                DeltaTime
                            );
                        }
                    }
                }
            }
        }
    }
}
```

### 사용 예시: 충돌 시 불꽃 파티클 생성

```
ParticleSystem "Projectile"
├─ Emitter 0: "Projectile Trail" (메인 투사체)
│   ├─ Collision Module
│   └─ EventGenerator Module
│       └─ Event: Type=Collision, Name="Impact"
│
└─ Emitter 1: "Impact Sparks" (충돌 시 불꽃)
    └─ EventReceiver Module
        ├─ EventType = EPET_Collision
        ├─ EventName = "Impact"
        └─ Action = EPERA_SpawnParticles (충돌 위치에 스폰)

결과:
- Emitter 0의 파티클이 충돌 시 "Impact" 이벤트 생성
- Emitter 1이 "Impact" 이벤트를 받아 충돌 위치에 불꽃 파티클 생성
```

---

## Mundi Engine 구현 가이드

### 필요한 파일 및 클래스

```
Mundi/Source/Runtime/Engine/Particles/
├── Event/
│   ├── ParticleModuleEventBase.h/.cpp          (기본 클래스)
│   ├── ParticleModuleEventGenerator.h/.cpp     (이벤트 생성)
│   └── ParticleModuleEventReceiver.h/.cpp      (이벤트 수신, 선택적)
│
├── ParticleSystemComponent.h/.cpp
│   ├── TArray<FParticleEventCollideData> CollisionEvents 추가
│   └── ReportEventCollision() 메서드 추가
│
└── GameFramework/
    └── ParticleEventManager.h/.cpp             (이벤트 디스패치)
```

### 델리게이트 선언 (Mundi 스타일)

```cpp
// ParticleSystemComponent.h

/**
 * Collision 이벤트 델리게이트
 * Lua 바인딩 가능하도록 설계
 */
DECLARE_MULTICAST_DELEGATE_NineParams(
    FOnParticleCollision,
    FName,                  // EventName
    float,                  // EmitterTime
    float,                  // ParticleTime
    const FVector&,         // Location
    const FVector&,         // Velocity
    const FVector&,         // Direction
    const FVector&,         // Normal
    FName,                  // BoneName
    class UPhysicalMaterial* // PhysMat (Mundi에는 없을 수 있음)
);

UCLASS()
class UParticleSystemComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    /** 충돌 이벤트 배열 (프레임당 축적) */
    TArray<FParticleEventCollideData> CollisionEvents;

    /** 충돌 이벤트 델리게이트 */
    FOnParticleCollision OnParticleCollide;

    /** 충돌 이벤트 보고 */
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
};
```

### AParticleEventManager 스켈레톤

```cpp
// ParticleEventManager.h

UCLASS()
class AParticleEventManager : public AActor
{
    GENERATED_BODY()

public:
    AParticleEventManager();

    /** 충돌 이벤트 처리 */
    virtual void HandleParticleCollisionEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventCollideData>& CollisionEvents
    );

    // Spawn, Death, Burst 이벤트도 유사하게 구현
};

// ParticleEventManager.cpp

void AParticleEventManager::HandleParticleCollisionEvents(
    UParticleSystemComponent* Component,
    const TArray<FParticleEventCollideData>& CollisionEvents)
{
    if (!Component) return;

    for (const FParticleEventCollideData& Event : CollisionEvents)
    {
        // 델리게이트 브로드캐스트
        Component->OnParticleCollide.Broadcast(
            Event.EventName,
            Event.EmitterTime,
            Event.ParticleTime,
            Event.Location,
            Event.Velocity,
            Event.Direction,
            Event.Normal,
            Event.BoneName,
            nullptr  // PhysMat (Mundi에는 없을 수 있음)
        );
    }
}
```

### Lua 바인딩

```lua
-- Lua 스크립트 예시

function OnParticleCollision(eventName, emitterTime, particleTime, location, velocity, direction, normal, boneName)
    print("Particle collision at " .. location:ToString())

    if eventName == "GroundHit" then
        -- 지면 충돌 처리
        Audio.PlaySound("impact_ground.wav", location)
    elseif eventName == "WaterSplash" then
        -- 물 충돌 처리
        ParticleSystem.Spawn("WaterSplash", location)
    end
end

-- 델리게이트 바인딩
local particleComp = Actor:GetComponent("ParticleSystemComponent")
particleComp:OnParticleCollide_Add(OnParticleCollision)
```

### 통합 플로우

```cpp
// UParticleSystemComponent::UpdateParticles()

void UParticleSystemComponent::UpdateParticles(float DeltaTime)
{
    // 프레임 시작 - 이벤트 클리어
    CollisionEvents.Reset();

    // 모든 이미터 업데이트
    for (FParticleEmitterInstance* Instance : EmitterInstances)
    {
        Instance->Tick(DeltaTime);
        // ↑ 이 안에서 ReportEventCollision() 호출로 이벤트 축적
    }

    // 프레임 종료 - 이벤트 디스패치
    if (CollisionEvents.Num() > 0)
    {
        UWorld* World = GetWorld();
        AParticleEventManager* EventManager = World->GetParticleEventManager();

        if (EventManager)
        {
            EventManager->HandleParticleCollisionEvents(this, CollisionEvents);
        }

        // 이벤트 클리어 (다음 프레임 준비)
        CollisionEvents.Reset();
    }
}
```

---

## 다음 문서

- **Part 4: Implementation** - 전체 시스템 통합, 테스트, 디버깅 가이드

---

**작성 일자**: 2025-01-25
