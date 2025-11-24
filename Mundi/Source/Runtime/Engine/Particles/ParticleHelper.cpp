#include "pch.h"
#include "ParticleHelper.h"
#include "SceneView.h"
#include "D3D11RHI.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ParticleLODLevel.h"
#include "Modules/TypeData/ParticleModuleTypeDataMesh.h"
#include "StaticMesh.h"

// ============================================================
// 헬퍼 함수들
// ============================================================

// 쿼드 버텍스/인덱스 버퍼 생성 (static, 1회만 호출)
static void CreateParticleQuadBuffers(ID3D11Device* Device, ID3D11Buffer** OutVertexBuffer, ID3D11Buffer** OutIndexBuffer)
{
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

    D3D11_SUBRESOURCE_DATA vbData = {};
    vbData.pSysMem = QuadVertices;

    Device->CreateBuffer(&vbDesc, &vbData, OutVertexBuffer);

    // 인덱스 (2개 삼각형)
    uint32 QuadIndices[6] = { 0, 1, 2, 0, 2, 3 };

    // 인덱스 버퍼 생성
    D3D11_BUFFER_DESC ibDesc = {};
    ibDesc.ByteWidth = sizeof(QuadIndices);
    ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
    ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA ibData = {};
    ibData.pSysMem = QuadIndices;

    Device->CreateBuffer(&ibDesc, &ibData, OutIndexBuffer);
}


// ============================================================
// FDynamicSpriteEmitterData 구현
// ============================================================

void FDynamicSpriteEmitterData::UpdateRenderData(FSceneView* View)
{
    if (!Source || Source->ActiveParticles == 0)
    {
        Instances.Empty();
        return;
    }

    BuildSpriteInstances(View);
}

void FDynamicSpriteEmitterData::BuildSpriteInstances(FSceneView* View)
{
    int32 ParticleCount = Source->ActiveParticles;
    Instances.SetNum(ParticleCount);

    static bool bLoggedParticles = false;
    bool bShouldLog = !bLoggedParticles && ParticleCount > 0;
    if (false /*bShouldLog*/)
    {
        UE_LOG("[debug] === PARTICLE DEBUG INFO ===");
        UE_LOG("[debug] Active Particles: %d", ParticleCount);
        bLoggedParticles = true;
    }

    // 각 파티클의 인스턴스 데이터 생성
    for (int32 i = 0; i < ParticleCount; ++i)
    {
        FBaseParticle* Particle = Source->GetParticle(i);
        if (!Particle)
            continue;

        // 인스턴스 데이터 설정 (명시적 float 필드로 변경)
        Instances[i].WorldPositionX = Particle->Location.X;
        Instances[i].WorldPositionY = Particle->Location.Y;
        Instances[i].WorldPositionZ = Particle->Location.Z;
        Instances[i].Padding0 = 0.0f;

        Instances[i].SizeX = Particle->Size.X;
        Instances[i].SizeY = Particle->Size.Y;
        Instances[i].Rotation = Particle->Rotation;
        Instances[i].Padding1 = 0.0f;

        Instances[i].ColorR = Particle->Color.R;
        Instances[i].ColorG = Particle->Color.G;
        Instances[i].ColorB = Particle->Color.B;
        Instances[i].ColorA = Particle->Color.A;

        // 처음 3개 파티클 로그
        if (false /*i < 3 && bShouldLog*/)
        {
            UE_LOG("[debug] Particle[%d] - Pos: (%.2f, %.2f, %.2f), Size: (%.2f, %.2f), Color: (%.2f, %.2f, %.2f, %.2f)",
                i,
                Particle->Location.X, Particle->Location.Y, Particle->Location.Z,
                Particle->Size.X, Particle->Size.Y,
                Particle->Color.R, Particle->Color.G, Particle->Color.B, Particle->Color.A);
        }
    }
}

void FDynamicSpriteEmitterData::Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material)
{
    if (Instances.Num() == 0)
    {
        UE_LOG("[debug] FDynamicSpriteEmitterData::Render - No instances to render");
        return;
    }
    
    if (false)
    {
        UE_LOG("[debug] FDynamicSpriteEmitterData::Render - Rendering %d instances", Instances.Num());
    }

    ID3D11Device* Device = RHI->GetDevice();
    ID3D11DeviceContext* Context = RHI->GetDeviceContext();

    // 1. 쿼드 버퍼 (static, 모든 Sprite 이미터가 공유)
    static ID3D11Buffer* QuadVertexBuffer = nullptr;
    static ID3D11Buffer* QuadIndexBuffer = nullptr;
    if (!QuadVertexBuffer)
    {
        CreateParticleQuadBuffers(Device, &QuadVertexBuffer, &QuadIndexBuffer);
    }

    // 2. 인스턴스 버퍼 (static, 모든 Sprite 이미터가 공유)
    static ID3D11Buffer* InstanceBuffer = nullptr;
    static uint32 MaxInstances = 0;

    uint32 RequiredInstances = Instances.Num();
    if (!InstanceBuffer || MaxInstances < RequiredInstances)
    {
        // 버퍼가 없거나 작으면 재생성 (여유분 1.5배)
        if (InstanceBuffer)
        {
            InstanceBuffer->Release();
            InstanceBuffer = nullptr;
        }

        MaxInstances = RequiredInstances * 3 / 2;  // 1.5배 여유

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(FParticleSpriteInstance) * MaxInstances;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Device->CreateBuffer(&desc, nullptr, &InstanceBuffer);
    }

    // 3. Map으로 인스턴스 데이터 업데이트
    if (InstanceBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        Context->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, Instances.GetData(), sizeof(FParticleSpriteInstance) * Instances.Num());
        Context->Unmap(InstanceBuffer, 0);
    }

    // 4. Rasterizer 상태 설정 (양면 렌더링)
    RHI->RSSetState(ERasterizerMode::Solid_NoCull);

    // 4.5. 블렌딩 상태 설정
    RHI->OMSetBlendState(true);

    // 4.6. Depth/Stencil 상태 설정 (반투명 파티클: depth test 활성화, depth write 비활성화)
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);

    // 5. Material 바인딩 (셰이더, 텍스처, 샘플러)
    if (Material)
    {
        // 셰이더 바인딩
        UShader* Shader = Material->GetShader();
        if (Shader)
        {
            RHI->PrepareShader(Shader);
            UE_LOG("[debug] Shader bound: %s", Shader ? "Valid" : "Null");
        }
        else
        {
            UE_LOG("[error] Shader is null!");
        }

        // 텍스처 바인딩
        ID3D11ShaderResourceView* DiffuseSRV = nullptr;
        if (UTexture* DiffuseTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse))
        {
            DiffuseSRV = DiffuseTexture->GetShaderResourceView();
            UE_LOG("[debug] Texture SRV: %s", DiffuseSRV ? "Valid" : "Null");
        }
        else
        {
            UE_LOG("[warn] No diffuse texture in material");
        }
        Context->PSSetShaderResources(0, 1, &DiffuseSRV);

        // 샘플러 바인딩
        ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);
        Context->PSSetSamplers(0, 1, &DefaultSampler);
    }
    else
    {
        UE_LOG("[error] Material is null!");
    }

    // 6. 버퍼 바인딩
    UINT Strides[2] = { sizeof(FVector2D), sizeof(FParticleSpriteInstance) };
    UINT Offsets[2] = { 0, 0 };
    ID3D11Buffer* Buffers[2] = { QuadVertexBuffer, InstanceBuffer };

    if (false)
    {
        UE_LOG("[debug] Vertex stride: %d bytes (expected 8), Instance stride: %d bytes (expected 48)",
           Strides[0], Strides[1]);
    }

    Context->IASetVertexBuffers(0, 2, Buffers, Strides, Offsets);
    Context->IASetIndexBuffer(QuadIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 7. DrawIndexedInstanced
    Context->DrawIndexedInstanced(6, Instances.Num(), 0, 0, 0);

    // 8. 렌더 상태 복원
    RHI->RSSetState(ERasterizerMode::Solid);
    RHI->OMSetBlendState(false);
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);
}

// ============================================================
// FDynamicMeshEmitterData 구현
// ============================================================

void FDynamicMeshEmitterData::UpdateRenderData(FSceneView* View)
{
    if (!Source || Source->ActiveParticles == 0)
    {
        InstanceTransforms.Empty();
        InstanceColors.Empty();
        return;
    }

    BuildMeshInstances();
}

void FDynamicMeshEmitterData::BuildMeshInstances()
{
    int32 ParticleCount = Source->ActiveParticles;
    InstanceTransforms.SetNum(ParticleCount);
    InstanceColors.SetNum(ParticleCount);

    for (int32 i = 0; i < ParticleCount; ++i)
    {
        FBaseParticle* Particle = Source->GetParticle(i);
        if (!Particle)
            continue;

        // Transform 행렬 생성 (Scale, Rotation, Translation)
        FMatrix ScaleMatrix = FMatrix::MakeScale(Particle->Size);

        // Rotation (Z축 회전)
        FMatrix RotationMatrix = FMatrix::Identity();
        if (Particle->Rotation != 0.0f)
        {
            float CosRot = cosf(Particle->Rotation);
            float SinRot = sinf(Particle->Rotation);
            RotationMatrix.M[0][0] = CosRot;
            RotationMatrix.M[0][1] = SinRot;
            RotationMatrix.M[1][0] = -SinRot;
            RotationMatrix.M[1][1] = CosRot;
        }

        // Translation
        FMatrix TranslationMatrix = FMatrix::MakeTranslation(Particle->Location);

        // 최종 변환: Scale * Rotation * Translation
        FMatrix Transform = ScaleMatrix * RotationMatrix * TranslationMatrix;

        InstanceTransforms[i] = Transform;
        InstanceColors[i] = FVector4(Particle->Color.R, Particle->Color.G, Particle->Color.B, Particle->Color.A);
    }
}

void FDynamicMeshEmitterData::Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material)
{
    if (InstanceTransforms.Num() == 0)
        return;

    ID3D11Device* Device = RHI->GetDevice();
    ID3D11DeviceContext* Context = RHI->GetDeviceContext();

    // 1. TypeDataModule에서 Mesh 가져오기
    UParticleLODLevel* LODLevel = Source->CurrentLODLevel;
    if (!LODLevel || !LODLevel->TypeDataModule)
        return;

    UParticleModuleTypeDataMesh* TypeDataMesh = Cast<UParticleModuleTypeDataMesh>(LODLevel->TypeDataModule);
    if (!TypeDataMesh || !TypeDataMesh->Mesh)
        return;

    UStaticMesh* ParticleMesh = TypeDataMesh->Mesh;

    // 2. 인스턴스 버퍼 생성 (Transform + Color를 하나의 구조체로)
    // 구조체: FMatrix(64 bytes) + FVector4(16 bytes) = 80 bytes per instance
    struct FMeshInstance
    {
        FMatrix Transform;
        FVector4 Color;
    };

    static ID3D11Buffer* InstanceBuffer = nullptr;
    static uint32 MaxInstances = 0;

    uint32 RequiredInstances = InstanceTransforms.Num();
    if (!InstanceBuffer || MaxInstances < RequiredInstances)
    {
        if (InstanceBuffer)
        {
            InstanceBuffer->Release();
            InstanceBuffer = nullptr;
        }

        MaxInstances = RequiredInstances * 3 / 2;

        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = sizeof(FMeshInstance) * MaxInstances;
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        Device->CreateBuffer(&desc, nullptr, &InstanceBuffer);
    }

    // 3. Map으로 인스턴스 데이터 업로드
    if (InstanceBuffer)
    {
        D3D11_MAPPED_SUBRESOURCE mapped;
        Context->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);

        FMeshInstance* InstanceData = (FMeshInstance*)mapped.pData;
        for (int32 i = 0; i < RequiredInstances; ++i)
        {
            InstanceData[i].Transform = InstanceTransforms[i];
            InstanceData[i].Color = InstanceColors[i];
        }

        Context->Unmap(InstanceBuffer, 0);
    }

    // 4. Rasterizer 상태 설정
    RHI->RSSetState(ERasterizerMode::Solid_NoCull);

    // 5. 블렌딩 상태 설정
    RHI->OMSetBlendState(true);

    // 6. Depth/Stencil 상태 설정
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);

    // 7. Material 바인딩
    if (Material)
    {
        UShader* Shader = Material->GetShader();
        if (Shader)
        {
            RHI->PrepareShader(Shader);
        }

        ID3D11ShaderResourceView* DiffuseSRV = nullptr;
        if (UTexture* DiffuseTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse))
        {
            DiffuseSRV = DiffuseTexture->GetShaderResourceView();
        }
        Context->PSSetShaderResources(0, 1, &DiffuseSRV);

        ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);
        Context->PSSetSamplers(0, 1, &DefaultSampler);
    }

    // 8. 버퍼 바인딩 (Mesh vertex buffer + Instance buffer)
    UINT VertexStride = ParticleMesh->GetVertexStride();
    UINT InstanceStride = sizeof(FMeshInstance);
    UINT Offsets[2] = { 0, 0 };
    ID3D11Buffer* Buffers[2] = { ParticleMesh->GetVertexBuffer(), InstanceBuffer };
    UINT Strides[2] = { VertexStride, InstanceStride };

    Context->IASetVertexBuffers(0, 2, Buffers, Strides, Offsets);
    Context->IASetIndexBuffer(ParticleMesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // 9. DrawIndexedInstanced
    UINT IndexCount = ParticleMesh->GetIndexCount();
    UINT InstanceCount = InstanceTransforms.Num();
    Context->DrawIndexedInstanced(IndexCount, InstanceCount, 0, 0, 0);

    // 10. 렌더 상태 복원
    RHI->RSSetState(ERasterizerMode::Solid);
    RHI->OMSetBlendState(false);
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);
}
