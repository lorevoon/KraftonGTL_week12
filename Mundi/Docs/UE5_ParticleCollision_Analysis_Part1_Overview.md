# UE5 Particle Collision 시스템 분석 Part 1: 개요

## 목차
1. [시스템 개요](#시스템-개요)
2. [전체 아키텍처](#전체-아키텍처)
3. [이벤트 플로우](#이벤트-플로우)
4. [주요 컴포넌트](#주요-컴포넌트)
5. [성능 최적화 전략](#성능-최적화-전략)
6. [다음 문서](#다음-문서)

---

## 시스템 개요

Unreal Engine 5의 Particle Collision 시스템은 **충돌 검사**, **물리 반응**, **이벤트 생성**을 분리한 잘 설계된 아키텍처입니다. 이 시스템은 다음과 같은 특징을 가지고 있습니다:

### 핵심 특징

- **Event-Driven Architecture**: 충돌 발생 시 이벤트를 생성하여 게임 로직에 전달
- **Separation of Concerns**: 충돌 검사, 이벤트 생성, 이벤트 디스패치가 독립적인 모듈로 분리
- **Thread-Safe Design**: 충돌 검사는 워커 스레드에서 실행 가능, 이벤트 디스패치는 게임 스레드에서 실행
- **Performance Optimized**: 거리 기반 LOD, 가시성 컬링, 빈도 필터링 등 다양한 최적화 기법
- **Highly Configurable**: 다양한 충돌 응답 옵션, 필터링 옵션, 물리 인터랙션 설정

### 주요 사용 사례

1. **환경 인터랙션**: 파티클이 지면이나 벽에 부딪혀 튕기거나 정지
2. **Effect Spawning**: 충돌 지점에 새로운 파티클 효과 생성 (불꽃, 파편 등)
3. **Sound Events**: 충돌 시 사운드 재생
4. **Gameplay Logic**: 투사체 파티클이 적에게 충돌 시 데미지 처리
5. **Visual Feedback**: 충돌 지점에 데칼이나 마크 생성

---

## 전체 아키텍처

### 시스템 구성도

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    PARTICLE COLLISION SYSTEM ARCHITECTURE                │
└─────────────────────────────────────────────────────────────────────────┘

                         ┌──────────────────────┐
                         │  UParticleSystem     │ (Asset)
                         │  - Emitters[]        │
                         └──────────┬───────────┘
                                    │
                         ┌──────────▼───────────┐
                         │  UParticleEmitter    │ (Asset)
                         │  - LODLevels[]       │
                         └──────────┬───────────┘
                                    │
                         ┌──────────▼───────────┐
                         │  UParticleLODLevel   │ (Asset)
                         │  - Modules[]         │
                         └──────────┬───────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    │               │               │
         ┌──────────▼──────────┐   │   ┌──────────▼──────────────┐
         │ UParticleModule     │   │   │ UParticleModuleEvent    │
         │ Collision           │   │   │ Generator               │
         │                     │   │   │                         │
         │ - Collision Params  │   │   │ - Event Configs[]       │
         │ - Physics Response  │   │   │ - Filtering Rules       │
         └──────────┬──────────┘   │   └──────────┬──────────────┘
                    │               │              │
                    │    ┌──────────▼──────────┐  │
                    │    │ FParticleEmitter    │  │
                    │    │ Instance (Runtime)  │  │
                    │    │                     │  │
                    │    │ - Particle Pool     │  │
                    │    │ - Active Particles  │  │
                    │    │ - Simulation State  │  │
                    │    └──────────┬──────────┘  │
                    │               │              │
                    └───────────────┼──────────────┘
                                    │
                         ┌──────────▼──────────────┐
                         │ UParticleSystem         │
                         │ Component (Runtime)     │
                         │                         │
                         │ - EmitterInstances[]    │
                         │ - CollisionEvents[]  ←──┼─── Event Storage
                         │ - SpawnEvents[]         │
                         │ - DeathEvents[]         │
                         └──────────┬──────────────┘
                                    │
                                    │ (End of Frame)
                                    │
                         ┌──────────▼──────────────┐
                         │ AParticleEventManager   │
                         │                         │
                         │ - HandleCollision()     │
                         │ - Broadcast Delegates   │
                         └──────────┬──────────────┘
                                    │
                    ┌───────────────┼────────────────┐
                    │               │                │
         ┌──────────▼──────────┐   │   ┌───────────▼────────────┐
         │ Blueprint/C++       │   │   │ EventReceiver Modules  │
         │ Delegates           │   │   │ (Particle-to-Particle) │
         │                     │   │   │                        │
         │ OnParticleCollide   │   │   │ - Spawn new particles  │
         └─────────────────────┘   │   │ - Kill particles       │
                                    │   └────────────────────────┘
                                    │
                         ┌──────────▼──────────────┐
                         │ Game Logic              │
                         │ - Sound playback        │
                         │ - VFX spawning          │
                         │ - Damage application    │
                         └─────────────────────────┘
```

### 주요 클래스 관계

```
Asset Layer (Serializable)
├── UParticleSystem
│   └── TArray<UParticleEmitter*> Emitters
│       └── TArray<UParticleLODLevel*> LODLevels
│           ├── UParticleModuleCollision* (Optional)
│           └── UParticleModuleEventGenerator* (Optional)

Runtime Layer (Non-Serializable)
├── UParticleSystemComponent
│   ├── TArray<FParticleEmitterInstance*> EmitterInstances
│   ├── TArray<FParticleEventCollideData> CollisionEvents  ← Event Buffer
│   └── UParticleSystem* Template (Reference to asset)
│
└── FParticleEmitterInstance
    ├── FBaseParticle* ParticleData (Memory pool)
    ├── FParticleCollisionPayload* (Per-particle collision state)
    └── FParticleEventInstancePayload* (Event generation flags)

Event Management Layer
└── AParticleEventManager (World singleton)
    ├── HandleParticleCollisionEvents()
    ├── HandleParticleSpawnEvents()
    └── HandleParticleDeathEvents()
```

---

## 이벤트 플로우

Particle Collision 이벤트는 **6단계**를 거쳐 처리됩니다:

### 전체 플로우 다이어그램

```
┌─────────────────────────────────────────────────────────────────────────┐
│                     PARTICLE COLLISION EVENT FLOW                        │
└─────────────────────────────────────────────────────────────────────────┘

FRAME N: Simulation Phase (Can run on worker thread)
════════════════════════════════════════════════════════════════════════════

1. COLLISION DETECTION PHASE (Every Frame, Per Particle)
   ┌──────────────────────────────────────────────────────────────┐
   │ UParticleModuleCollision::Update()                          │
   │                                                              │
   │ For each active particle:                                   │
   │   ├─ Check LOD filters (distance, visibility)               │
   │   ├─ Calculate sweep parameters                             │
   │   │   └─ Start: Particle->OldLocation                       │
   │   │   └─ End: Particle->Location + Velocity * DeltaTime     │
   │   │                                                          │
   │   ├─ PerformCollisionCheck() (Scene query)                  │
   │   │   └─ LineTraceSingleByChannel() or SweepSingle()        │
   │   │                                                          │
   │   └─ If collision detected:                                 │
   │       ├─ Apply physics response (bounce/damping)            │
   │       ├─ Decrement UsedCollisions counter                   │
   │       ├─ Apply completion action if max reached             │
   │       │   (Kill, Freeze, HaltCollisions, etc.)              │
   │       │                                                      │
   │       └─ If EventPayload->bCollisionEventsPresent:          │
   │           Call HandleParticleCollision() ─────────────────┐ │
   └──────────────────────────────────────────────────────────│─┘
                                                               │
                                                               │
2. EVENT GENERATION PHASE (On Collision)                       │
   ┌───────────────────────────────────────────────────────────▼─┐
   │ UParticleModuleEventGenerator::HandleParticleCollision()   │
   │                                                              │
   │ For each event configuration in Events[]:                   │
   │   ├─ Check Type == EPET_Collision                           │
   │   ├─ Apply FirstTimeOnly filter                             │
   │   │   └─ Skip if (Particle->Flags & CollisionHasOccurred)   │
   │   ├─ Apply LastTimeOnly filter                              │
   │   │   └─ Skip if (UsedCollisions > 0)                       │
   │   ├─ Check frequency                                        │
   │   │   └─ Skip if (CollisionCount % Frequency != 0)          │
   │   │                                                          │
   │   └─ If pass all filters:                                   │
   │       Component->ReportEventCollision() ──────────────────┐ │
   └──────────────────────────────────────────────────────────│─┘
                                                               │
                                                               │
3. EVENT STORAGE PHASE (Accumulation)                          │
   ┌───────────────────────────────────────────────────────────▼─┐
   │ UParticleSystemComponent::ReportEventCollision()           │
   │                                                              │
   │ FParticleEventCollideData* Event = new(CollisionEvents);    │
   │                                                              │
   │ Event->Type = EPET_Collision;                               │
   │ Event->EventName = CustomName;                              │
   │ Event->EmitterTime = EmitterTime;                           │
   │ Event->Location = Hit.Location;                             │
   │ Event->Velocity = Particle->Velocity;                       │
   │ Event->Direction = CollideDirection;                        │
   │ Event->ParticleTime = Particle->RelativeTime;               │
   │ Event->Normal = Hit.Normal;                                 │
   │ Event->BoneName = Hit.BoneName;                             │
   │ Event->PhysMat = Hit.PhysMaterial;                          │
   │                                                              │
   │ // Events accumulate across all particles this frame        │
   └──────────────────────────────────────────────────────────────┘
                              ▼
   ┌──────────────────────────────────────────────────────────────┐
   │ TArray<FParticleEventCollideData> CollisionEvents           │
   │                                                              │
   │ [ Event1 | Event2 | Event3 | ... | EventN ]                 │
   │                                                              │
   │ (Multiple collisions from multiple emitters)                │
   └──────────────────────────────────────────────────────────────┘

════════════════════════════════════════════════════════════════════════════
FRAME N: Finalization Phase (Must run on game thread)
════════════════════════════════════════════════════════════════════════════

4. EVENT DISPATCH PHASE (End of Frame)
   ┌──────────────────────────────────────────────────────────────┐
   │ UParticleSystemComponent::FinalizeTickComponent()           │
   │                                                              │
   │ if (CollisionEvents.Num() > 0) {                            │
   │     AParticleEventManager* EventManager =                   │
   │         World->GetParticleEventManager();                   │
   │                                                              │
   │     EventManager->HandleParticleCollisionEvents(            │
   │         this, CollisionEvents                               │
   │     ); ────────────────────────────────────────────────────┐│
   │ }                                                           ││
   │                                                             ││
   │ // Clear events for next frame                             ││
   │ CollisionEvents.Reset();                                    ││
   └─────────────────────────────────────────────────────────────┘│
                                                                  │
                                                                  │
5. EVENT BROADCAST PHASE (Gameplay Notification)                 │
   ┌──────────────────────────────────────────────────────────────▼┐
   │ AParticleEventManager::HandleParticleCollisionEvents()       │
   │                                                               │
   │ For each event in CollisionEvents:                           │
   │   ├─ Component->OnParticleCollide.Broadcast(                │
   │   │     EventName, EmitterTime, Location, Velocity, ...      │
   │   │ ); ───────────────────────────────────────────────────┐  │
   │   │                                                        │  │
   │   └─ If (Emitter Actor exists):                           │  │
   │       Emitter->OnParticleCollide.Broadcast(...); ─────────┼┐ │
   └───────────────────────────────────────────────────────────┘│ │
                                                                │ │
                                                                │ │
6. EVENT RECEIVER PHASE (Multiple Targets)                      │ │
   ┌────────────────────────────────────────────────────────────▼─▼─┐
   │ Receivers:                                                     │
   │                                                                │
   │ A) Blueprint/C++ Delegates                                    │
   │    └─ OnParticleCollide.AddDynamic(...)                       │
   │        ├─ Play sound at hit location                          │
   │        ├─ Spawn decal at hit location                         │
   │        ├─ Apply damage to hit actor                           │
   │        └─ Custom gameplay logic                               │
   │                                                                │
   │ B) EventReceiver Modules (Particle-to-Particle)               │
   │    └─ UParticleModuleEventReceiver::ProcessParticleEvent()    │
   │        ├─ Spawn new particles at collision location           │
   │        ├─ Kill particles in radius                            │
   │        └─ Modify particle properties                          │
   │                                                                │
   │ C) Lua Scripts (If bound)                                     │
   │    └─ OnParticleCollision(location, normal, velocity)         │
   └────────────────────────────────────────────────────────────────┘

════════════════════════════════════════════════════════════════════════════
FRAME N+1: Repeat
════════════════════════════════════════════════════════════════════════════
```

### 타이밍 및 스레드 안전성

| 단계 | 스레드 | 타이밍 | 노트 |
|-----|-------|-------|------|
| 1. Collision Detection | Worker Thread* | During Tick | *bApplyPhysics=false일 때만 |
| 2. Event Generation | Worker Thread* | During Tick | Lock-free append to TArray |
| 3. Event Storage | Worker Thread* | During Tick | Thread-safe TArray operations |
| 4. Event Dispatch | Game Thread | FinalizeTickComponent | 모든 업데이트 완료 후 |
| 5. Event Broadcast | Game Thread | After Finalize | Delegate invocation |
| 6. Event Receiver | Game Thread | After Broadcast | 게임 로직 실행 |

**중요**:
- 물리 임펄스 적용(`bApplyPhysics=true`)이 활성화되면 Collision Detection이 게임 스레드에서만 실행됩니다.
- Event Storage는 TArray의 thread-safe append를 사용하여 안전합니다.
- Event Dispatch 이후 즉시 `CollisionEvents.Reset()`이 호출되어 다음 프레임을 위해 버퍼를 비웁니다.

---

## 주요 컴포넌트

### 1. UParticleModuleCollision

**역할**: 파티클의 월드 충돌 검사 및 물리 반응 처리

**주요 기능**:
- Scene query를 통한 충돌 검사 (Line trace / Sweep)
- 충돌 시 속도 반사 및 감쇠 (Bounce)
- 최대 충돌 횟수 추적 및 완료 동작 (Kill, Freeze 등)
- 충돌한 액터에 물리 임펄스 적용 (선택적)
- 거리 기반 LOD 및 가시성 컬링

**파일 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\Collision\ParticleModuleCollision.h`
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleModules_Collision.cpp`

**Per-Particle Data**:
```cpp
struct FParticleCollisionPayload {
    FVector3f UsedDampingFactor;          // 12 bytes - 적용된 감쇠 값
    FVector3f UsedDampingFactorRotation;  // 12 bytes - 회전 감쇠 값
    int32 UsedCollisions;                  // 4 bytes - 남은 충돌 횟수
    float Delay;                           // 4 bytes - 충돌 시작 지연 시간
};  // Total: 32 bytes per particle
```

### 2. UParticleModuleEventGenerator

**역할**: 충돌 이벤트 생성 및 필터링

**주요 기능**:
- 다양한 이벤트 타입 지원 (Collision, Spawn, Death, Burst)
- 이벤트 필터링 (FirstTimeOnly, LastTimeOnly, Frequency)
- 커스텀 이벤트 이름 지정
- 게임 특화 페이로드 데이터 첨부

**파일 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\Event\ParticleModuleEventGenerator.h`
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleModules_Event.cpp`

**Event Configuration**:
```cpp
struct FParticleEvent_GenerateInfo {
    EParticleEventType Type;                // Collision, Spawn, Death, Burst
    int32 Frequency;                         // 0 = every time, N = every Nth
    int32 ParticleFrequency;                 // Per-particle frequency
    bool FirstTimeOnly;                      // Only first collision per particle
    bool LastTimeOnly;                       // Only last collision per particle
    bool UseReflectedImpactVector;           // Use reflected vector
    bool bUseOrbitOffset;                    // Include orbit offset
    FName CustomName;                        // Event name for filtering
    TArray<UParticleModuleEventSendToGame*> ParticleModuleEventsToSendToGame;
};
```

### 3. FParticleEventCollideData

**역할**: 충돌 이벤트 데이터 저장

**데이터 구조**:
```cpp
// Base event data
struct FParticleEventData {
    int32 Type;                                          // EPET_Collision
    FName EventName;                                      // Custom name
    float EmitterTime;                                    // Emitter lifetime
    FVector Location;                                     // Event location
    FVector Velocity;                                     // Particle velocity
    TArray<UParticleModuleEventSendToGame*> EventData;   // Payload
};

// Extended for existing particles
struct FParticleExistingData : FParticleEventData {
    float ParticleTime;  // Particle lifetime (0-1)
    FVector Direction;   // Movement direction
};

// Collision-specific data
struct FParticleEventCollideData : FParticleExistingData {
    FVector Normal;             // Collision surface normal
    float Time;                 // Hit time along ray (0-1)
    int32 Item;                 // Primitive item index
    FName BoneName;             // Bone name (skeletal meshes)
    UPhysicalMaterial* PhysMat; // Physical material
};
```

**파일 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\ParticleSystemComponent.h:286-310`

### 4. UParticleSystemComponent

**역할**: 파티클 시스템 런타임 관리 및 이벤트 저장

**Event Storage**:
```cpp
class UParticleSystemComponent {
    // Event buffers (cleared each frame)
    TArray<FParticleEventSpawnData> SpawnEvents;
    TArray<FParticleEventDeathData> DeathEvents;
    TArray<FParticleEventCollideData> CollisionEvents;  // ← 충돌 이벤트
    TArray<FParticleEventBurstData> BurstEvents;
    TArray<FParticleEventKismetData> KismetEvents;

    // Event reporting methods
    void ReportEventCollision(
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
        UPhysicalMaterial* PhysMat
    );

    // Delegate
    FParticleCollisionSignature OnParticleCollide;
};
```

**파일 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\ParticleSystemComponent.h`

### 5. AParticleEventManager

**역할**: 중앙 이벤트 디스패처

**주요 기능**:
- World 싱글톤 패턴 (`UWorld::MyParticleEventManager`)
- 델리게이트 브로드캐스트
- 서브클래싱 가능한 가상 함수

**인터페이스**:
```cpp
class AParticleEventManager : public AActor {
public:
    virtual void HandleParticleSpawnEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventSpawnData>& SpawnEvents
    );

    virtual void HandleParticleDeathEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventDeathData>& DeathEvents
    );

    virtual void HandleParticleCollisionEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventCollideData>& CollisionEvents
    );

    virtual void HandleParticleBurstEvents(
        UParticleSystemComponent* Component,
        const TArray<FParticleEventBurstData>& BurstEvents
    );
};
```

**파일 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\ParticleEventManager.h`
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\ParticleEventManager.cpp`

---

## 성능 최적화 전략

Unreal Engine의 Particle Collision 시스템은 여러 레벨의 최적화를 적용합니다:

### 1. 거리 기반 LOD (Distance Culling)

```cpp
// 30 프레임마다 시스템 바운드 체크
if ((CurrentLODBoundsCheckCount == 0) || bForceCheck) {
    CurrentLODBoundsCheckCount = 30;

    FBox BoundingBox = Owner->Component->Bounds.GetBox();
    BoundingBox = BoundingBox.ExpandBy(MaxCollisionDistance);

    // 플레이어가 범위 내에 있는지 체크
    bool bAnyPlayerInRange = false;
    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator();
         Iterator; ++Iterator) {
        APlayerController* PlayerController = Iterator->Get();
        if (BoundingBox.IsInside(PlayerController->GetFocalLocation())) {
            bAnyPlayerInRange = true;
            break;
        }
    }

    if (!bAnyPlayerInRange) {
        return;  // Skip collision for all particles
    }
}
```

**효과**: 플레이어로부터 멀리 떨어진 파티클 시스템 전체의 충돌 검사를 스킵

### 2. Per-Particle Distance Check

```cpp
// MaxCollisionDistance가 설정된 경우
if (MaxCollisionDistance > 0.0f) {
    bool bInRange = false;

    for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator();
         Iterator; ++Iterator) {
        APlayerController* PlayerController = Iterator->Get();
        FVector PlayerLocation = PlayerController->GetFocalLocation();

        float DistSquared = (Location - PlayerLocation).SizeSquared();
        float MaxDistSquared = MaxCollisionDistance * MaxCollisionDistance;

        if (DistSquared < MaxDistSquared) {
            bInRange = true;
            break;
        }
    }

    if (!bInRange) {
        continue;  // Skip this particle
    }
}
```

**효과**: 플레이어로부터 먼 개별 파티클의 충돌 검사를 스킵

### 3. 가시성 기반 컬링 (Visibility Culling)

```cpp
if (bCollideOnlyIfVisible) {
    float TimeSinceLastRender = World->TimeSeconds - Owner->Component->LastRenderTime;
    float IgnoreInvisibleTime = GParticleCollisionIgnoreInvisibleTime;  // Default: 0.1s

    if (TimeSinceLastRender > IgnoreInvisibleTime) {
        return;  // Skip collision - not recently rendered
    }
}
```

**효과**: 화면에 보이지 않는 파티클 시스템의 충돌 검사를 스킵

### 4. 이벤트 빈도 필터링 (Event Frequency Filtering)

```cpp
// EventGenerator에서
if (EventGenInfo.Frequency > 0) {
    // N번째 충돌마다만 이벤트 생성
    if ((EventPayload->CollisionTrackingCount % EventGenInfo.Frequency) != 0) {
        continue;  // Skip event generation
    }
}
```

**효과**: 이벤트 생성 빈도를 줄여 오버헤드 감소

### 5. FirstTimeOnly / LastTimeOnly 필터링

```cpp
// FirstTimeOnly: 파티클당 첫 충돌만 이벤트 생성
if (EventGenInfo.FirstTimeOnly) {
    if (CollideParticle->Flags & STATE_Particle_CollisionHasOccurred) {
        continue;  // Already collided
    }
}

// LastTimeOnly: 파티클의 마지막 충돌만 이벤트 생성
if (EventGenInfo.LastTimeOnly) {
    if (CollidePayload->UsedCollisions != 0) {
        continue;  // Not the last collision yet
    }
}
```

**효과**: 불필요한 이벤트 생성을 줄여 메모리 및 처리 오버헤드 감소

### 6. 이벤트 배치 처리 (Batch Event Dispatch)

```cpp
// 프레임 중: 이벤트를 TArray에 축적만 함
Component->ReportEventCollision(...);  // Just appends to array

// 프레임 종료 시: 모든 이벤트를 한 번에 처리
if (CollisionEvents.Num() > 0) {
    EventManager->HandleParticleCollisionEvents(this, CollisionEvents);
}
CollisionEvents.Reset();  // Clear for next frame
```

**효과**: 델리게이트 호출 오버헤드를 최소화하고 캐시 지역성 향상

### 7. Thread-Safe 설계

```cpp
// Worker thread에서 실행 가능 (bApplyPhysics=false인 경우)
// - Collision detection
// - Event generation
// - Event storage (thread-safe TArray append)

// Game thread에서만 실행
// - Event dispatch
// - Delegate broadcast
// - Physics impulse application
```

**효과**: 충돌 검사를 워커 스레드에서 병렬 실행하여 성능 향상

### 성능 측정 결과 (예시)

| 파티클 수 | 최적화 전 | 최적화 후 | 개선율 |
|---------|----------|----------|-------|
| 1,000 | 2.5 ms | 0.8 ms | 68% ↓ |
| 5,000 | 12.0 ms | 3.5 ms | 71% ↓ |
| 10,000 | 25.0 ms | 7.0 ms | 72% ↓ |

**측정 환경**: Distance culling + Visibility culling + Frequency=3 적용 시

---

## 다음 문서

이제 시스템의 전체적인 구조를 파악했으니, 다음 문서에서는 각 컴포넌트를 상세히 분석합니다:

- **Part 2: CollisionModule** - UParticleModuleCollision의 충돌 검사 및 물리 반응 로직 상세 분석
- **Part 3: EventSystem** - Event Generator, Event Manager, 델리게이트 시스템 상세 분석
- **Part 4: Implementation** - Mundi Engine에 적용하기 위한 구현 가이드

---

## 참고 자료

**Unreal Engine 소스 코드 위치**:
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Classes\Particles\`
- `C:\UnrealEngine\Engine\Source\Runtime\Engine\Private\Particles\`

**관련 문서**:
- UE5_ParticleSystem_Analysis_Part1-4.md (기본 파티클 시스템 분석)
- Mundi Engine CLAUDE.md (프로젝트 코딩 규칙)

**작성 일자**: 2025-01-25
