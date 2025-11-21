# UE5 Particle System 분석 - Part 1: 개요 및 아키텍처

## 📋 분석 목적

Mundi Engine의 파티클 시스템 구현을 위해 **실제 UE5 소스코드**를 분석하여:
- 검증된 아키텍처 패턴 파악
- 메모리 레이아웃 및 성능 최적화 기법 이해
- Mundi Engine 구현 계획과의 차이점 비교
- 개선 권장사항 도출

**분석 대상**: `C:\Dev\UE5\UnrealEngine\Engine\Source\Runtime\Engine\`

---

## 📂 주요 파일 위치

### 1. 핵심 클래스 헤더
```
Classes/Particles/
├── ParticleSystem.h              // 파티클 시스템 최상위 에셋
├── ParticleEmitter.h             // 파티클 이미터 (Emitter 단위)
├── ParticleLODLevel.h            // LOD별 모듈 리스트 관리
├── ParticleModule.h              // 모든 모듈의 베이스 클래스 ⭐
├── ParticleModuleRequired.h      // 필수 모듈 (SpawnRate, EmitterDuration 등)
└── TypeData/
    └── ParticleModuleTypeDataBase.h  // TypeData 모듈 베이스

Classes/Particles/[Category]/
├── Acceleration/
│   └── ParticleModuleAcceleration.h
├── Color/
│   ├── ParticleModuleColor.h
│   └── ParticleModuleColorOverLife.h
├── Lifetime/
│   └── ParticleModuleLifetime.h
├── Location/
│   ├── ParticleModuleLocation.h
│   └── ParticleModuleLocationPrimitiveSphere.h
├── Size/
│   └── ParticleModuleSize.h
└── Velocity/
    └── ParticleModuleVelocity.h
```

### 2. 런타임 구현
```
Private/Particles/
├── ParticleEmitterInstances.cpp  // FParticleEmitterInstance 구현
├── ParticleModules.cpp            // 모듈 공통 구현
├── ParticleHelper.h               // 헬퍼 매크로 모음 ⭐
└── [Category]/
    ├── ParticleModuleAccelerationImpl.h
    ├── ParticleModuleColorImpl.h
    └── ...
```

### 3. 핵심 데이터 구조
```
Public/Particles/
└── ParticleEmitterInstance.h     // FParticleEmitterInstance, FBaseParticle ⭐
```

---

## 🏗️ 시스템 아키텍처

### 계층 구조

```
UParticleSystem (에셋)
└── UParticleEmitter (논리적 이미터)
    └── UParticleLODLevel (LOD 레벨별)
        └── TArray<UParticleModule*> Modules
            ├── UParticleModuleRequired (필수)
            ├── UParticleModuleSpawn (스폰)
            ├── UParticleModuleLifetime (생명)
            ├── UParticleModuleLocation (위치)
            └── ... (기타 모듈)

런타임:
UParticleSystemComponent
└── TArray<FParticleEmitterInstance*> EmitterInstances
    ├── FBaseParticle* ParticleData (메모리 풀)
    ├── int32 ActiveParticles
    ├── int32 MaxActiveParticles
    └── UParticleLODLevel* CurrentLODLevel
```

### 핵심 개념

#### 1. **에셋 vs 런타임 분리**
- **에셋 (UObject)**: `UParticleSystem`, `UParticleEmitter`, `UParticleModule`
  - 에디터에서 편집 가능
  - 여러 인스턴스에서 공유
- **런타임 (비-UObject)**: `FParticleEmitterInstance`, `FBaseParticle`
  - 실제 파티클 시뮬레이션
  - 인스턴스별 고유 데이터

#### 2. **LOD 시스템**
- `UParticleLODLevel`: LOD별로 다른 모듈 리스트 보유
- 런타임에 거리/성능에 따라 LOD 전환
- 각 LOD는 독립적인 모듈 리스트 소유

#### 3. **모듈 실행 순서**
```cpp
// UParticleModule 베이스 클래스
UPROPERTY()
uint8 bSpawnModule:1;          // Spawn 단계에서 실행
uint8 bUpdateModule:1;         // Update 단계에서 실행
uint8 bFinalUpdateModule:1;    // FinalUpdate 단계에서 실행
uint8 bCurvesAsColor:1;        // 커브를 컬러로 해석
```

**Mundi와의 차이점**:
- Mundi 계획: `ExecutionOrder` int32 값으로 정렬
- UE5: 불린 플래그로 실행 단계 구분 (더 유연함)

---

## 🧩 데이터 구조: FBaseParticle

```cpp
// Engine/Public/Particles/ParticleEmitterInstance.h
struct FBaseParticle
{
    // Core State (48 bytes)
    FVector Location;            // 12 bytes - 현재 위치
    FVector OldLocation;         // 12 bytes - 이전 위치 (트레일/충돌 용)
    FVector BaseVelocity;        // 12 bytes - 기본 속도
    FVector Velocity;            // 12 bytes - 현재 속도 (모듈 누적)

    // Size/Rotation (32 bytes)
    FVector Size;                // 12 bytes - 현재 크기
    FVector BaseSize;            // 12 bytes - 초기 크기
    float Rotation;              // 4 bytes
    float BaseRotationRate;      // 4 bytes

    // Color (32 bytes)
    FLinearColor Color;          // 16 bytes - 현재 컬러
    FLinearColor BaseColor;      // 16 bytes - 초기 컬러

    // Lifecycle (12 bytes)
    float RelativeTime;          // 0.0 ~ 1.0 (생명 비율)
    float OneOverMaxLifetime;    // 1.0 / MaxLifetime (최적화)
    int32 Flags;                 // 파티클 플래그

    // Total: ~128 bytes (정렬 포함)
};
```

### 핵심 패턴: **Dual State System**
- `Base*` 필드: 스폰 시 초기값
- 현재 필드: 매 프레임 갱신
- **목적**: 모듈이 additive하게 동작 가능
  ```cpp
  // Example: ColorOverLife 모듈
  Particle.Color = Particle.BaseColor * CurveValue;  // Base 기반 계산
  ```

**Mundi 계획과의 차이**:
- Mundi: Base 필드 없음 → 모듈이 절대값으로만 설정 가능
- UE5: Base 필드 있음 → 곱셈/가산 모듈 자연스럽게 구현

---

## 🔄 파티클 생명주기

### 1. Spawn Phase
```cpp
for (UParticleModule* Module : CurrentLODLevel->SpawnModules)
{
    if (Module->bSpawnModule)
    {
        Module->Spawn(Owner, Offset, SpawnTime, &BaseParticle);
    }
}
```

### 2. Update Phase
```cpp
BEGIN_UPDATE_LOOP
{
    for (UParticleModule* Module : CurrentLODLevel->UpdateModules)
    {
        if (Module->bUpdateModule)
        {
            Module->Update(Owner, Offset, DeltaTime);
        }
    }

    // Physics integration
    Particle.OldLocation = Particle.Location;
    Particle.Location += Particle.Velocity * DeltaTime;
}
END_UPDATE_LOOP
```

### 3. Final Update Phase
```cpp
// Rendering 직전 최종 처리
for (UParticleModule* Module : CurrentLODLevel->FinalUpdateModules)
{
    if (Module->bFinalUpdateModule)
    {
        Module->FinalUpdate(Owner, Offset, DeltaTime);
    }
}
```

---

## 📊 메모리 레이아웃

### Stride 기반 인덱싱
```cpp
// FParticleEmitterInstance
uint8* ParticleData;         // 연속된 메모리 블록
uint32 ParticleStride;       // sizeof(FBaseParticle) + 추가 페이로드
int32* ParticleIndices;      // 활성 파티클 인덱스 배열

// 접근 방식
FBaseParticle* GetParticle(int32 Index)
{
    return (FBaseParticle*)(ParticleData + ParticleIndices[Index] * ParticleStride);
}
```

### 장점
- ✅ 메모리 연속성 → 캐시 효율
- ✅ 동적 페이로드 (모듈별 추가 데이터)
- ✅ 인덱스 기반 관리 → 삭제/정렬 효율

**Mundi 적용**:
- Mundi 계획도 stride 기반 인덱싱 사용 예정 ✅
- `FParticleEmitterInstance` 구조 참고

---

## 🎯 다음 Part 예고

**Part 2**에서는 핵심 클래스 상세 분석:
- `UParticleModule` 가상 함수 인터페이스
- Context Pattern (FSpawnContext, FUpdateContext)
- `FParticleEmitterInstance` 전체 구조
- 실제 모듈 구현 예제 (`UParticleModuleColor`, `UParticleModuleLifetime`)

---

**작성일**: 2025-11-21
**분석 기준**: Unreal Engine 5.x Source Code
**작성자**: Claude Code Analysis
