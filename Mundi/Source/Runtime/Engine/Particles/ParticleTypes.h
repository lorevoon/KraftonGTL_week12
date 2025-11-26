#pragma once

#include <cmath>
#include <cstring>
#include <algorithm>
#include "Vector.h"
#include "Color.h"

// 파티클 이미터 타입
enum class EDynamicEmitterType : uint8
{
    Sprite,      // 스프라이트 파티클 (빌보드)
    Mesh,        // 메시 파티클 (3D 메시)
    Beam,        // 빔 파티클 (라인 세그먼트)
    Ribbon,      // 리본 파티클 (궤적 띠)
    Unknown
};

// 파티클 정렬 모드
enum class EParticleSortMode : uint8
{
    None,                    // 정렬 안 함
    ViewDistanceDepth,       // 카메라 거리 기준
    AgeOldestFirst,         // 나이 많은 순
    AgeNewestFirst          // 나이 적은 순
};

// 파티클 분포 타입 (위치/속도 분포)
enum class EDistributionType : uint8
{
    Constant,       // 고정값
    Uniform,        // 균등 분포
    ConstantCurve,  // 시간별 커브 (선택)
    Particle        // 파티클별 랜덤
};

// 파티클 충돌 반응 타입
enum class EParticleCollisionResponse : uint8
{
    Kill,           // 충돌 시 파티클 제거
    Bounce,         // 충돌 시 반사
    Stop            // 충돌 시 정지
};

// 파티클 플래그 비트마스크
// - FBaseParticle::Flags에 사용
namespace EParticleFlags
{
    constexpr int32 None           = 0;
    constexpr int32 Dead           = 1 << 0;  // 파티클 사망 예정
    constexpr int32 Collided       = 1 << 1;  // 이번 프레임에 충돌 발생
    constexpr int32 IgnoreCollision = 1 << 2; // 충돌 검사 무시
}

// 전방 선언
class UParticleEmitter;
class UParticleLODLevel;
class UParticleSystemComponent;
class FSceneView;
struct FDynamicEmitterDataBase;

// 파티클 데이터 구조
// - 연속된 메모리 블록에 저장되는 파티클 데이터
// - Stride 기반 인덱싱으로 동적 페이로드 지원
struct FBaseParticle
{
    // Core State (48 bytes)
    FVector Location;            // 12 bytes - 현재 위치
    FVector OldLocation;         // 12 bytes - 이전 위치 (트레일/충돌 용)
    FVector BaseVelocity;        // 12 bytes - 기본 속도
    FVector Velocity;            // 12 bytes - 현재 속도

    // Size/Rotation (32 bytes)
    FVector Size;                // 12 bytes - 현재 크기
    FVector BaseSize;            // 12 bytes - 초기 크기
    float Rotation;              // 4 bytes - 현재 회전
    float BaseRotationRate;      // 4 bytes - 초기 회전 속도

    // Color (32 bytes)
    FLinearColor Color;          // 16 bytes - 현재 컬러
    FLinearColor BaseColor;      // 16 bytes - 초기 컬러

    // Lifecycle (12 bytes)
    float RelativeTime;          // 0.0 ~ 1.0 (생명 비율)
    float OneOverMaxLifetime;    // 1.0 / MaxLifetime (최적화)
    int32 Flags;                 // 파티클 플래그

    // Padding to align to 16 bytes (4 bytes)
    uint32 Padding;              // 16바이트 정렬용 패딩

    // Total: 128 bytes (16-byte aligned)

    FBaseParticle()
        : Location(FVector::Zero())
        , OldLocation(FVector::Zero())
        , BaseVelocity(FVector::Zero())
        , Velocity(FVector::Zero())
        , Size(FVector::One())
        , BaseSize(FVector::One())
        , Rotation(0.0f)
        , BaseRotationRate(0.0f)
        , Color(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        , BaseColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f))
        , RelativeTime(0.0f)
        , OneOverMaxLifetime(0.0f)
        , Flags(0)
        , Padding(0)
    {
    }
};

// ============================================================
// Trail/Ribbon 파티클 페이로드
// ============================================================

// Trail 파티클 플래그
namespace ETrailParticleFlags
{
    constexpr uint32 None          = 0;
    constexpr uint32 Head          = 1 << 0;  // Trail의 HEAD 파티클 (리더)
    constexpr uint32 Tail          = 1 << 1;  // Trail의 TAIL 파티클
    constexpr uint32 Middle        = 1 << 2;  // Trail의 MIDDLE 파티클
    constexpr uint32 DeadTrail     = 1 << 3;  // 죽은 Trail (제거 예정)
    constexpr uint32 Interpolated  = 1 << 4;  // 보간된 파티클 (테셀레이션)
}

// Trail 베이스 페이로드
// - 모든 Trail/Ribbon 파티클이 FBaseParticle 뒤에 가지는 추가 데이터
struct FTrailsBaseTypeDataPayload
{
    uint32 Flags;                      // ETrailParticleFlags (4 bytes)
    int32  TrailIndex;                 // 트레일 인덱스 (HEAD만 사용) (4 bytes)
    int32  TriangleCount;              // 트레일의 총 삼각형 수 (HEAD만 사용) (4 bytes)
    float  SpawnTime;                  // 스폰 시간 (4 bytes)
    int32  RenderingInterpCount;       // 렌더링 시 보간 개수 (4 bytes)
    float  TiledU;                     // UV 타일링 (4 bytes)
    bool   bInterpolatedSpawn;         // 보간 스폰 여부 (1 byte)
    uint8  Padding[7];                 // 16바이트 정렬용 패딩 (7 bytes)

    // Total: 32 bytes (16-byte aligned)

    FTrailsBaseTypeDataPayload()
        : Flags(ETrailParticleFlags::None)
        , TrailIndex(-1)
        , TriangleCount(0)
        , SpawnTime(0.0f)
        , RenderingInterpCount(0)
        , TiledU(0.0f)
        , bInterpolatedSpawn(false)
    {
        memset(Padding, 0, sizeof(Padding));
    }
};

// Ribbon 전용 페이로드
// - FTrailsBaseTypeDataPayload + 추가 데이터
struct FRibbonTypeDataPayload : public FTrailsBaseTypeDataPayload
{
    FVector Tangent;       // 탄젠트 벡터 (Trail 방향) (12 bytes)
    FVector Up;            // 업 벡터 (리본 면 방향) (12 bytes)
    int32   SourceIndex;   // 소스 인덱스 (멀티 트레일 시) (4 bytes)
    int32   Next;          // Trail에서 다음 파티클의 ActiveIndex (-1이면 TAIL) (4 bytes)

    // Total: 32 (base) + 32 = 64 bytes (16-byte aligned)

    FRibbonTypeDataPayload()
        : FTrailsBaseTypeDataPayload()
        , Tangent(FVector(1, 0, 0))
        , Up(FVector(0, 0, 1))
        , SourceIndex(0)
        , Next(-1)
    {
    }
};
