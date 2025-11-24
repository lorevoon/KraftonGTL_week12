#pragma once
#include "ResourceBase.h"

// 파티클 렌더링을 위한 동적 버퍼 관리 클래스
// UDynamicMesh와 동일한 패턴으로 D3D11 버퍼 생명주기 관리
class UParticleDynamicBuffers : public UResourceBase
{
public:
    DECLARE_CLASS(UParticleDynamicBuffers, UResourceBase)

    UParticleDynamicBuffers() = default;
    virtual ~UParticleDynamicBuffers() override;

    // 초기화 (Device 필요)
    void Initialize(ID3D11Device* InDevice);

    // Sprite 인스턴스 버퍼 가져오기/생성 (자동 확장)
    // @param RequiredInstances: 필요한 인스턴스 수
    // @param OutMaxInstances: 현재 버퍼의 최대 인스턴스 수 (출력)
    // @return 인스턴스 버퍼 포인터
    ID3D11Buffer* GetOrCreateSpriteInstanceBuffer(
        uint32 RequiredInstances,
        uint32& OutMaxInstances);

    // Mesh 인스턴스 버퍼 가져오기/생성 (자동 확장)
    // @param RequiredInstances: 필요한 인스턴스 수
    // @param OutMaxInstances: 현재 버퍼의 최대 인스턴스 수 (출력)
    // @return 인스턴스 버퍼 포인터
    ID3D11Buffer* GetOrCreateMeshInstanceBuffer(
        uint32 RequiredInstances,
        uint32& OutMaxInstances);

    // Quad 버퍼 가져오기 (한 번만 생성, 모든 Sprite 파티클 공유)
    ID3D11Buffer* GetQuadVertexBuffer();
    ID3D11Buffer* GetQuadIndexBuffer();

private:
    // Quad 버퍼 생성 (한 번만 호출)
    void CreateQuadBuffers();

    // 모든 버퍼 해제
    void ReleaseResources();

    ID3D11Device* Device = nullptr;

    // Sprite용 인스턴스 버퍼
    ID3D11Buffer* SpriteInstanceBuffer = nullptr;
    uint32 SpriteMaxInstances = 0;

    // Mesh용 인스턴스 버퍼
    ID3D11Buffer* MeshInstanceBuffer = nullptr;
    uint32 MeshMaxInstances = 0;

    // 공용 Quad 버퍼 (모든 Sprite 파티클 공유)
    ID3D11Buffer* QuadVertexBuffer = nullptr;
    ID3D11Buffer* QuadIndexBuffer = nullptr;
    bool bQuadBuffersInitialized = false;
};
