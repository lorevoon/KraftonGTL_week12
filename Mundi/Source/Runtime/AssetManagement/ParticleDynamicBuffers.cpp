#include "pch.h"
#include "ParticleDynamicBuffers.h"

IMPLEMENT_CLASS(UParticleDynamicBuffers)

UParticleDynamicBuffers::~UParticleDynamicBuffers()
{
    ReleaseResources();
}

void UParticleDynamicBuffers::Initialize(ID3D11Device* InDevice)
{
    if (!InDevice)
        return;

    Device = InDevice;

    // 버퍼는 필요 시 Lazy 생성
    // Quad 버퍼는 첫 GetQuadVertexBuffer() 호출 시 생성됨
}

ID3D11Buffer* UParticleDynamicBuffers::GetOrCreateSpriteInstanceBuffer(
    uint32 RequiredInstances,
    uint32& OutMaxInstances)
{
    if (!Device)
    {
        OutMaxInstances = 0;
        return nullptr;
    }

    // 버퍼가 없거나 용량이 부족하면 재생성
    if (!SpriteInstanceBuffer || SpriteMaxInstances < RequiredInstances)
    {
        // 기존 버퍼 해제
        if (SpriteInstanceBuffer)
        {
            SpriteInstanceBuffer->Release();
            SpriteInstanceBuffer = nullptr;
        }

        // 1.5배 여유분 확보
        SpriteMaxInstances = RequiredInstances * 3 / 2;
        if (SpriteMaxInstances < 64)
            SpriteMaxInstances = 64;  // 최소 64개

        // FParticleSpriteInstance 크기: 48 bytes (ParticleHelper.h 참조)
        const uint32 InstanceSize = 48;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = InstanceSize * SpriteMaxInstances;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = Device->CreateBuffer(&desc, nullptr, &SpriteInstanceBuffer);
        if (FAILED(hr))
        {
            SpriteMaxInstances = 0;
            OutMaxInstances = 0;
            return nullptr;
        }
    }

    OutMaxInstances = SpriteMaxInstances;
    return SpriteInstanceBuffer;
}

ID3D11Buffer* UParticleDynamicBuffers::GetOrCreateMeshInstanceBuffer(
    uint32 RequiredInstances,
    uint32& OutMaxInstances)
{
    if (!Device)
    {
        OutMaxInstances = 0;
        return nullptr;
    }

    // 버퍼가 없거나 용량이 부족하면 재생성
    if (!MeshInstanceBuffer || MeshMaxInstances < RequiredInstances)
    {
        // 기존 버퍼 해제
        if (MeshInstanceBuffer)
        {
            MeshInstanceBuffer->Release();
            MeshInstanceBuffer = nullptr;
        }

        // 1.5배 여유분 확보
        MeshMaxInstances = RequiredInstances * 3 / 2;
        if (MeshMaxInstances < 64)
            MeshMaxInstances = 64;  // 최소 64개

        // FMeshInstance 크기: 80 bytes (FMatrix 64 + FVector4 16)
        const uint32 InstanceSize = 80;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = InstanceSize * MeshMaxInstances;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;

        HRESULT hr = Device->CreateBuffer(&desc, nullptr, &MeshInstanceBuffer);
        if (FAILED(hr))
        {
            MeshMaxInstances = 0;
            OutMaxInstances = 0;
            return nullptr;
        }
    }

    OutMaxInstances = MeshMaxInstances;
    return MeshInstanceBuffer;
}

ID3D11Buffer* UParticleDynamicBuffers::GetOrCreateBeamVertexBuffer(
    uint32 RequiredVertices,
    uint32& OutMaxVertices)
{
    if (!Device)
    {
        OutMaxVertices = 0;
        return nullptr;
    }

    // 버퍼가 없거나 용량이 부족하면 재생성
    if (!BeamVertexBuffer || BeamMaxVertices < RequiredVertices)
    {
        if (BeamVertexBuffer)
        {
            BeamVertexBuffer->Release();
            BeamVertexBuffer = nullptr;
        }

        // 1.5배 여유분 확보
        BeamMaxVertices = RequiredVertices * 3 / 2;
        if (BeamMaxVertices < 128)
            BeamMaxVertices = 128;

        // FParticleBeamVertex 크기: 40 bytes
        const uint32 VertexSize = 40;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = VertexSize * BeamMaxVertices;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = Device->CreateBuffer(&desc, nullptr, &BeamVertexBuffer);
        if (FAILED(hr))
        {
            BeamMaxVertices = 0;
            OutMaxVertices = 0;
            return nullptr;
        }
    }

    OutMaxVertices = BeamMaxVertices;
    return BeamVertexBuffer;
}

ID3D11Buffer* UParticleDynamicBuffers::GetOrCreateBeamIndexBuffer(
    uint32 RequiredIndices,
    uint32& OutMaxIndices)
{
    if (!Device)
    {
        OutMaxIndices = 0;
        return nullptr;
    }

    // 버퍼가 없거나 용량이 부족하면 재생성
    if (!BeamIndexBuffer || BeamMaxIndices < RequiredIndices)
    {
        if (BeamIndexBuffer)
        {
            BeamIndexBuffer->Release();
            BeamIndexBuffer = nullptr;
        }

        // 1.5배 여유분 확보
        BeamMaxIndices = RequiredIndices * 3 / 2;
        if (BeamMaxIndices < 256)
            BeamMaxIndices = 256;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(uint32) * BeamMaxIndices;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT hr = Device->CreateBuffer(&desc, nullptr, &BeamIndexBuffer);
        if (FAILED(hr))
        {
            BeamMaxIndices = 0;
            OutMaxIndices = 0;
            return nullptr;
        }
    }

    OutMaxIndices = BeamMaxIndices;
    return BeamIndexBuffer;
}

ID3D11Buffer* UParticleDynamicBuffers::GetQuadVertexBuffer()
{
    if (!bQuadBuffersInitialized)
    {
        CreateQuadBuffers();
    }
    return QuadVertexBuffer;
}

ID3D11Buffer* UParticleDynamicBuffers::GetQuadIndexBuffer()
{
    if (!bQuadBuffersInitialized)
    {
        CreateQuadBuffers();
    }
    return QuadIndexBuffer;
}

void UParticleDynamicBuffers::CreateQuadBuffers()
{
    if (!Device || bQuadBuffersInitialized)
        return;

    // 쿼드 정점 (로컬 좌표 -0.5 ~ 0.5)
    FVector2D QuadVertices[4] = {
        FVector2D(-0.5f, -0.5f),  // 왼쪽 아래
        FVector2D( 0.5f, -0.5f),  // 오른쪽 아래
        FVector2D( 0.5f,  0.5f),  // 오른쪽 위
        FVector2D(-0.5f,  0.5f)   // 왼쪽 위
    };

    // 정점 버퍼 생성
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = sizeof(QuadVertices);
    vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbDesc.CPUAccessFlags = 0;
    vbDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = QuadVertices;

    HRESULT hr = Device->CreateBuffer(&vbDesc, &vbData, &QuadVertexBuffer);
    if (FAILED(hr))
    {
        QuadVertexBuffer = nullptr;
        return;
    }

    // 인덱스 (2개 삼각형)
    uint32 QuadIndices[6] = { 0, 1, 2, 0, 2, 3 };

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(QuadIndices);
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibDesc.CPUAccessFlags = 0;
    ibDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = QuadIndices;

    hr = Device->CreateBuffer(&ibDesc, &ibData, &QuadIndexBuffer);
    if (FAILED(hr))
    {
        if (QuadVertexBuffer)
        {
            QuadVertexBuffer->Release();
            QuadVertexBuffer = nullptr;
        }
        QuadIndexBuffer = nullptr;
        return;
    }

    bQuadBuffersInitialized = true;
}

void UParticleDynamicBuffers::ReleaseResources()
{
    if (SpriteInstanceBuffer)
    {
        SpriteInstanceBuffer->Release();
        SpriteInstanceBuffer = nullptr;
    }
    SpriteMaxInstances = 0;

    if (MeshInstanceBuffer)
    {
        MeshInstanceBuffer->Release();
        MeshInstanceBuffer = nullptr;
    }
    MeshMaxInstances = 0;

    if (BeamVertexBuffer)
    {
        BeamVertexBuffer->Release();
        BeamVertexBuffer = nullptr;
    }
    if (BeamIndexBuffer)
    {
        BeamIndexBuffer->Release();
        BeamIndexBuffer = nullptr;
    }
    BeamMaxVertices = 0;
    BeamMaxIndices = 0;

    if (QuadVertexBuffer)
    {
        QuadVertexBuffer->Release();
        QuadVertexBuffer = nullptr;
    }

    if (QuadIndexBuffer)
    {
        QuadIndexBuffer->Release();
        QuadIndexBuffer = nullptr;
    }

    bQuadBuffersInitialized = false;
    Device = nullptr;
}
