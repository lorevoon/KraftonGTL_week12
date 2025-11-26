# Ribbon 파티클 구현 원리

## 개요

Ribbon 파티클은 움직이는 오브젝트의 **궤적(Trail)**을 시각화하는 파티클 타입입니다. 검, 총알, 미사일 등의 이동 경로를 부드러운 리본 형태로 표현합니다.

핵심 원리:
- **거리 기반 스폰**: 시간이 아닌 이동 거리에 따라 파티클 생성
- **Hermite 보간**: Spawn time에 곡선 보간을 적용하여 부드러운 궤적 생성
- **Trail 체인 구조**: 링크드 리스트로 파티클들을 연결
- **단순 렌더링**: 이미 곡선으로 배치된 파티클들을 직선으로 연결

---

## 1. Trail 체인 구조

### 1.1 링크드 리스트 기반 설계

Ribbon 파티클은 일반 배열이 아닌 **단방향 링크드 리스트**로 관리됩니다:

```
HEAD (최신) → MIDDLE → MIDDLE → TAIL (최초)
  ↓              ↓        ↓        ↓
[DataIdx:5]  [DataIdx:2] [DataIdx:7] [DataIdx:1]
```

**왜 링크드 리스트인가?**
- 파티클이 수명대로 죽지 않음 (TAIL부터 순차적으로 죽지 않음)
- 중간 파티클이 먼저 죽을 수 있음 (Lifetime 랜덤)
- 배열 인덱스로는 순서 보장 불가 → DataIndex 기반 연결 필요

### 1.2 FRibbonTypeDataPayload 구조

각 파티클은 `FBaseParticle` 뒤에 추가 데이터를 가집니다:

```cpp
struct FRibbonTypeDataPayload
{
    int32 Next;           // 다음 파티클의 DataIndex (-1이면 TAIL)
    FVector Tangent;      // 이동 방향 (정규화된 벡터)
    float SpawnTime;      // 생성 시각
    uint32 Flags;         // Head/Middle/Tail 구분
};
```

**메모리 레이아웃:**
```
ParticleData 배열:
[DataIndex 0] → FBaseParticle(128B) + FRibbonTypeDataPayload(32B)
[DataIndex 1] → FBaseParticle(128B) + FRibbonTypeDataPayload(32B)
...
```

### 1.3 HEAD 파티클 추적

`SpawnPerUnit` 모듈은 현재 HEAD 파티클을 추적합니다:

```cpp
struct FSpawnPerUnitInstancePayload
{
    int32 HeadParticleDataIndex;  // 가장 최신 파티클의 DataIndex
    FVector PreviousLocation;      // 이전 프레임 위치
    FVector LastTangent;           // 이전 Tangent (Hermite 보간용)
    float AccumulatedDistance;     // 누적 이동 거리
};
```

**새 파티클 추가 시:**
```cpp
// 1. 새 파티클 생성
NewParticle->Next = HeadParticleDataIndex;  // 기존 HEAD를 가리킴

// 2. 기존 HEAD를 MIDDLE로 변경
OldHead->Flags &= ~ETrailParticleFlags::Head;
OldHead->Flags |= ETrailParticleFlags::Middle;

// 3. 새 파티클을 HEAD로 설정
NewParticle->Flags = ETrailParticleFlags::Head;
HeadParticleDataIndex = NewDataIndex;
```

---

## 2. 거리 기반 스폰 (SpawnPerUnit)

### 2.1 왜 거리 기반인가?

**시간 기반 스폰의 문제점:**
```
SpawnRate = 10/sec
속도 = 1m/s  → 0.1m 간격 (좋음)
속도 = 100m/s → 10m 간격 (구멍 뚫림!)
```

**거리 기반 스폰:**
```
SpawnPerUnit = 20개/m
어떤 속도든 → 0.05m 간격 유지
```

### 2.2 누적 거리 시스템

매 프레임마다:

```cpp
// 1. 이동 거리 계산
FVector Delta = CurrentLocation - PreviousLocation;
float Distance = Delta.Size();

// 2. 누적 거리에 추가
AccumulatedDistance += Distance;

// 3. SpawnPerUnit에 따라 생성 개수 계산
float SpawnCount = (AccumulatedDistance / UnitScalar) * SpawnPerUnit;
// 예: AccumulatedDistance=0.15m, UnitScalar=1.0m, SpawnPerUnit=20
//     → SpawnCount = 0.15 * 20 = 3개

// 4. 실제 스폰
int32 ActualSpawnCount = (int32)SpawnCount;  // 3개 생성

// 5. 소모한 거리 차감
float ConsumedDistance = (ActualSpawnCount * UnitScalar) / SpawnPerUnit;
AccumulatedDistance -= ConsumedDistance;
// 0.15m - 0.15m = 0m (정확히 소모)
```

**부동소수점 오차 누적 방지:**
- 남은 거리를 다음 프레임으로 이월
- 정확한 간격 유지

---

## 3. Hermite 보간 (Spawn Time)

### 3.1 왜 Hermite 보간인가?

**문제:** 직선으로만 파티클을 배치하면 곡선 이동 시 각진 Trail 생성

```
직선 배치:
  P0 -------- P1
              |
              |
              P2

실제 이동 경로:
  P0
    \
      \ (곡선)
        \
          P2
```

**해결:** Hermite Cubic Interpolation으로 부드러운 곡선 생성

```
Hermite 보간:
  P0
    \___
        \___  (부드러운 곡선)
            \
              P2
```

### 3.2 Hermite 공식

4개의 제어점으로 곡선 생성:
- `P0`: 이전 위치 (PreviousLocation)
- `T0`: 이전 Tangent (이전 이동 방향)
- `P1`: 현재 위치 (CurrentLocation)
- `T1`: 현재 Tangent (현재 이동 방향)

```cpp
// Hermite Basis Functions (alpha ∈ [0, 1])
float H1 = 2*alpha³ - 3*alpha² + 1      // P0 계수
float H2 = alpha³ - 2*alpha² + alpha     // T0 계수
float H3 = -2*alpha³ + 3*alpha²          // P1 계수
float H4 = alpha³ - alpha²               // T1 계수

// 보간된 위치
SpawnLocation = P0*H1 + T0*H2 + P1*H3 + T1*H4
```

### 3.3 실제 구현 (SpawnParticlesAlongMovement)

```cpp
// 현재 Tangent 계산
FVector CurrentTangent = MovementDelta.GetSafeNormal() * Distance;

// 이전 Tangent 가져오기 (첫 이동 시 초기화)
FVector PreviousTangent = InstanceData->LastTangent;
if (PreviousTangent.IsZero())
    PreviousTangent = CurrentTangent;

// ActualSpawnCount개의 파티클을 곡선상에 배치
for (int32 i = 0; i < ActualSpawnCount; ++i)
{
    float Alpha = (float)(i + 1) / (float)ActualSpawnCount;

    // Hermite 계산
    float Alpha2 = Alpha * Alpha;
    float Alpha3 = Alpha2 * Alpha;
    float H1 = 2.0f * Alpha3 - 3.0f * Alpha2 + 1.0f;
    float H2 = Alpha3 - 2.0f * Alpha2 + Alpha;
    float H3 = -2.0f * Alpha3 + 3.0f * Alpha2;
    float H4 = Alpha3 - Alpha2;

    FVector SpawnLocation =
        InstanceData->PreviousLocation * H1 +  // P0
        PreviousTangent * H2 +                  // T0
        CurrentLocation * H3 +                  // P1
        CurrentTangent * H4;                    // T1

    SpawnTrailParticle(Owner, SpawnLocation, ...);
}

// 다음 프레임을 위해 Tangent 저장
InstanceData->LastTangent = CurrentTangent;
```

**핵심:**
- **Spawn time에만 보간 수행** (1회)
- 렌더링은 단순 직선 연결 (매 프레임)
- ~98% 연산량 감소

---

## 4. Tangent 재계산 (RecalculateTangents)

### 4.1 왜 필요한가?

새 파티클 추가 시, 기존 파티클들의 Tangent가 부정확할 수 있습니다:

```
[이전]
P0 → P1 (Tangent: P0→P1 방향)

[P2 추가 후]
P0 → P1 → P2
     ↑
     Tangent를 P1→P2 방향으로 수정 필요
```

### 4.2 구현

```cpp
void RecalculateTangents(FParticleEmitterInstance* Owner, int32 HeadDataIndex)
{
    int32 CurrentDataIndex = HeadDataIndex;
    FVector PrevLocation = FVector::Zero();

    // HEAD부터 TAIL까지 순회
    while (CurrentDataIndex != -1)
    {
        FBaseParticle* Current = GetParticleByDataIndex(CurrentDataIndex);
        FRibbonTypeDataPayload* Payload = GetPayloadByDataIndex(CurrentDataIndex);

        int32 NextDataIndex = Payload->Next;

        if (NextDataIndex != -1)
        {
            // MIDDLE/HEAD: 다음 파티클 방향
            FBaseParticle* Next = GetParticleByDataIndex(NextDataIndex);
            FVector TangentVec = Next->Location - Current->Location;
            Payload->Tangent = TangentVec.GetSafeNormal();
        }
        else
        {
            // TAIL: 이전 파티클에서 오는 방향
            if (bHasPrev)
            {
                FVector TangentVec = Current->Location - PrevLocation;
                Payload->Tangent = TangentVec.GetSafeNormal();
            }
        }

        PrevLocation = Current->Location;
        CurrentDataIndex = NextDataIndex;
    }
}
```

---

## 5. 파티클 Kill 처리

### 5.1 체인에서 제거

파티클이 죽으면 링크드 리스트에서 제거해야 합니다:

```cpp
void OnParticleKilled(FParticleEmitterInstance* Owner, int32 DyingDataIndex)
{
    // 1. 죽는 파티클의 Next 가져오기
    FRibbonTypeDataPayload* DyingPayload = GetPayload(DyingDataIndex);
    int32 DyingNext = DyingPayload->Next;

    // 2. 이전 파티클을 찾아서 Next를 DyingNext로 변경
    for (int32 i = 0; i < Owner->ActiveParticles; ++i)
    {
        int32 DataIndex = Owner->ParticleIndices[i];
        if (DataIndex == DyingDataIndex) continue;

        FRibbonTypeDataPayload* Payload = GetPayload(DataIndex);

        if (Payload->Next == DyingDataIndex)
        {
            Payload->Next = DyingNext;  // 건너뛰기
            break;
        }
    }

    // 3. HEAD가 죽는 경우 Next를 새 HEAD로
    if (HeadParticleDataIndex == DyingDataIndex)
    {
        HeadParticleDataIndex = DyingNext;
    }
}
```

**결과:**
```
[Before] P0 → P1 → P2 (P1 dies)
                    ↑
[After]  P0 -----→ P2
```

---

## 6. 렌더링 파이프라인

### 6.1 BuildRibbonVertices

**이전 (비효율적):**
- 파티클 수집 → Hermite 보간 (tessellation) → 버텍스 생성
- 매 프레임 곡선 계산

**현재 (최적화):**
- 파티클 수집 → 직선 연결 → 버텍스 생성
- 이미 곡선상에 배치됨

```cpp
void BuildRibbonVertices(...)
{
    // 1. Trail 체인 순회하여 TrailPoint 수집
    TArray<FTrailPoint> TrailPoints;
    int32 CurrentDataIndex = HeadDataIndex;

    while (CurrentDataIndex != -1)
    {
        FBaseParticle* Particle = GetParticle(CurrentDataIndex);
        FRibbonTypeDataPayload* Payload = GetPayload(CurrentDataIndex);

        TrailPoints.Add({
            Particle->Location,
            Payload->Tangent,
            Particle->Color,
            Particle->Size.X
        });

        CurrentDataIndex = Payload->Next;
    }

    // 2. 각 TrailPoint에서 좌우 버텍스 생성
    for (int32 i = 0; i < TrailPoints.Num(); ++i)
    {
        FVector Position = TrailPoints[i].Position;
        FVector Tangent = TrailPoints[i].Tangent;
        float Width = RibbonModule->Width * TrailPoints[i].Size;

        // RenderAxis에 따라 Right 벡터 계산
        FVector Right;
        if (RenderAxis == CameraUp)
            Right = FVector::Cross(CameraDir, Tangent).GetSafeNormal();
        else if (RenderAxis == WorldUp)
            Right = FVector::Cross(FVector::ZAxis, Tangent).GetSafeNormal();

        // 좌우 버텍스
        FVector Left = Position - Right * Width * 0.5f;
        FVector RightPos = Position + Right * Width * 0.5f;

        // UV 계산 (TilingDistance 기반)
        float U = AccumulatedDistance / TilingDistance;

        Vertices.Add({Left, FVector2D(U, 0.0f), Color});
        Vertices.Add({RightPos, FVector2D(U, 1.0f), Color});

        AccumulatedDistance += SegmentLength;
    }

    // 3. Triangle Strip 인덱스 생성
    // [0,1,2] [2,1,3] [2,3,4] [4,3,5] ...
}
```

### 6.2 RenderAxis

리본의 방향 결정:

**CameraUp (기본):**
```
Camera
  ↓
  ↓ CameraDir
  ↓
Ribbon (항상 카메라를 향함)
```

**WorldUp:**
```
      Z↑
       |
Ribbon (항상 수평)
```

**SourceUp:**
```
Custom Up Vector (오브젝트 기준)
```

---

## 7. 주요 프로퍼티

### SpawnPerUnit 모듈

| 프로퍼티 | 타입 | 기본값 | 설명 |
|---------|------|--------|------|
| `SpawnPerUnit` | float | 20.0 | 단위당 스폰 개수 |
| `UnitScalar` | float | 1.0 | 단위 크기 (미터) |
| `MaxFrameDistance` | int32 | 200 | 프레임당 최대 생성 개수 |
| `bSpawnOnMovementStart` | bool | true | 첫 이동 시 파티클 생성 |

**예시:**
- `SpawnPerUnit=20, UnitScalar=1.0` → 1m당 20개 (0.05m 간격)
- `SpawnPerUnit=10, UnitScalar=2.0` → 2m당 10개 (0.2m 간격)

### RibbonTypeData 모듈

| 프로퍼티 | 타입 | 기본값 | 설명 |
|---------|------|--------|------|
| `Width` | float | 0.5 | 리본 너비 (미터) |
| `MaxParticleInTrailCount` | int32 | 2000 | Trail당 최대 파티클 수 |
| `RenderAxis` | enum | CameraUp | 렌더링 축 |
| `TilingDistance` | float | 5.0 | UV 타일링 거리 (미터) |

---

## 8. 성능 최적화

### 8.1 Hermite 보간 최적화

**Before:**
- 렌더링: 매 프레임 전체 Trail Hermite 보간 (60fps * N개 파티클)
- 연산량: O(N * Tessellation) per frame

**After:**
- Spawn: 새 파티클 생성 시에만 Hermite 보간 (이동 거리당 1회)
- 렌더링: 직선 연결만 (O(N))
- **~98% 연산량 감소**

### 8.2 메모리 레이아웃

**DataIndex 기반 접근:**
```cpp
// 빠른 접근 (캐시 친화적)
uint8* ParticleBytes = ParticleData + DataIndex * ParticleStride;
FBaseParticle* Particle = (FBaseParticle*)ParticleBytes;
FRibbonTypeDataPayload* Payload = (FRibbonTypeDataPayload*)(ParticleBytes + 128);
```

**ParticleStride 정렬:**
- FBaseParticle: 128바이트 (16바이트 정렬)
- FRibbonTypeDataPayload: 32바이트
- 총 ParticleStride: 160바이트

---

## 9. 사용 예시

### 검 궤적 (좁고 빠름)

```cpp
// SpawnPerUnit
SpawnPerUnit = 30.0f;  // 조밀하게
UnitScalar = 1.0f;

// Ribbon
Width = 0.2f;  // 좁게
MaxParticleInTrailCount = 1000;

// Lifetime
LifetimeMin = 0.3f;  // 짧게 (잔상 효과)
LifetimeMax = 0.3f;
```

### 미사일 연기 (넓고 길게)

```cpp
// SpawnPerUnit
SpawnPerUnit = 10.0f;  // 적당히
UnitScalar = 1.0f;

// Ribbon
Width = 1.5f;  // 넓게
MaxParticleInTrailCount = 3000;  // 길게 유지

// Lifetime
LifetimeMin = 3.0f;  // 오래 유지
LifetimeMax = 3.0f;
```

---

## 10. 디버깅

### 10.1 bRenderSpawnPoints

Trail 파티클 위치를 시각적으로 표시:
```cpp
if (bRenderSpawnPoints)
{
    for (TrailPoint in TrailPoints)
        DrawDebugSphere(TrailPoint.Position, 0.1f, Red);
}
```

### 10.2 bRenderTangents

Tangent 방향 표시:
```cpp
if (bRenderTangents)
{
    for (TrailPoint in TrailPoints)
        DrawDebugLine(TrailPoint.Position,
                     TrailPoint.Position + TrailPoint.Tangent,
                     Blue);
}
```

---

## 요약

**Ribbon 파티클 핵심:**
1. **거리 기반 스폰** - 속도 무관하게 일정 간격 유지
2. **Hermite 보간** - Spawn time에 1회만 적용하여 부드러운 곡선 생성
3. **Trail 체인** - DataIndex 기반 링크드 리스트로 순서 관리
4. **단순 렌더링** - 이미 배치된 파티클을 직선 연결
5. **Tangent 관리** - 연속성 있는 곡선을 위한 방향 벡터 저장

**성능 이점:**
- Hermite 보간: 매 프레임 → 1회로 감소 (~98% 절감)
- 렌더링: O(N * Tessellation) → O(N)
- 메모리: DataIndex 기반 캐시 친화적 접근
