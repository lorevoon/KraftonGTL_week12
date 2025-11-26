#include "pch.h"
#include "ParticleHelper.h"
#include "SceneView.h"
#include "D3D11RHI.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ParticleLODLevel.h"
#include "Modules/TypeData/ParticleModuleTypeDataMesh.h"
#include "Modules/TypeData/ParticleModuleTypeDataBeam.h"
#include "Modules/ParticleModuleBeamNoise.h"
#include "StaticMesh.h"
#include "ParticleDynamicBuffers.h"
#include "ParticleSystemComponent.h"
#include "ResourceManager.h"

// ============================================================
// 헬퍼 함수들
// ============================================================
// (CreateParticleQuadBuffers는 UParticleDynamicBuffers로 이동됨)


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

    // bUseLocalSpace일 때만 컴포넌트 월드 위치를 더함
    const bool bUseLocalSpace = Source->UseLocalSpace();
    const FVector ComponentWorldLocation = bUseLocalSpace ? Source->GetComponentWorldLocation() : FVector::Zero();

    // 각 파티클의 인스턴스 데이터 생성
    for (int32 i = 0; i < ParticleCount; ++i)
    {
        FBaseParticle* Particle = Source->GetParticle(i);
        if (!Particle)
            continue;

        // Local Space면 월드로 변환, World Space면 그대로 사용
        FVector WorldPosition = Particle->Location + ComponentWorldLocation;

        // 인스턴스 데이터 설정 (명시적 float 필드로 변경)
        Instances[i].WorldPositionX = WorldPosition.X;
        Instances[i].WorldPositionY = WorldPosition.Y;
        Instances[i].WorldPositionZ = WorldPosition.Z;
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

void FDynamicSpriteEmitterData::Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool)
{
    if (Instances.Num() == 0)
    {
        UE_LOG("[debug] FDynamicSpriteEmitterData::Render - No instances to render");
        return;
    }

    if (!BufferPool)
    {
        UE_LOG("[error] BufferPool is null!");
        return;
    }

    if (false)
    {
        UE_LOG("[debug] FDynamicSpriteEmitterData::Render - Rendering %d instances", Instances.Num());
    }

    ID3D11Device* Device = RHI->GetDevice();
    ID3D11DeviceContext* Context = RHI->GetDeviceContext();

    // 1. BufferPool에서 쿼드 버퍼 가져오기 (모든 Sprite 이미터가 공유)
    ID3D11Buffer* QuadVertexBuffer = BufferPool->GetQuadVertexBuffer();
    ID3D11Buffer* QuadIndexBuffer = BufferPool->GetQuadIndexBuffer();

    // 2. BufferPool에서 인스턴스 버퍼 가져오기 (자동 확장)
    uint32 MaxInstances = 0;
    uint32 RequiredInstances = Instances.Num();
    ID3D11Buffer* InstanceBuffer = BufferPool->GetOrCreateSpriteInstanceBuffer(RequiredInstances, MaxInstances);

    if (!InstanceBuffer)
    {
        UE_LOG("[error] Failed to create sprite instance buffer!");
        return;
    }

    // 3. Map으로 인스턴스 데이터 업데이트
    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Instances.GetData(), sizeof(FParticleSpriteInstance) * Instances.Num());
    Context->Unmap(InstanceBuffer, 0);

    // 4. Rasterizer 상태 설정 (양면 렌더링)
    RHI->RSSetState(ERasterizerMode::Solid_NoCull);

    // 4.5. 블렌딩 상태 설정
    RHI->OMSetBlendState(true);

    // 4.6. Depth/Stencil 상태 설정 (반투명 파티클: depth test 활성화, depth write 비활성화)
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);

    // 5. 셰이더 바인딩 - 파티클 전용 쉐이더 강제 사용 (Material과 무관)
    static UShader* SpriteShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleSprite.hlsl");
    if (SpriteShader)
    {
        RHI->PrepareShader(SpriteShader);
        UE_LOG("[debug] Shader bound: %s", SpriteShader ? "Valid" : "Null");
    }
    else
    {
        UE_LOG("[error] Shader is null!");
    }

    // 6. 텍스처 바인딩 (Material이 있으면 Material에서, 없으면 nullptr)
    ID3D11ShaderResourceView* DiffuseSRV = nullptr;
    if (Material)
    {
        if (UTexture* DiffuseTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse))
        {
            DiffuseSRV = DiffuseTexture->GetShaderResourceView();
            UE_LOG("[debug] Texture SRV from Material: %s", DiffuseSRV ? "Valid" : "Null");
        }
        else
        {
            UE_LOG("[warn] No diffuse texture in material");
        }
    }
    else
    {
        UE_LOG("[debug] Material is null, using no texture (will render as white)");
    }
    Context->PSSetShaderResources(0, 1, &DiffuseSRV);

    // 7. 샘플러 바인딩
    ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);
    Context->PSSetSamplers(0, 1, &DefaultSampler);

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
        Instances.Empty();
        return;
    }

    BuildMeshInstances();
}

void FDynamicMeshEmitterData::BuildMeshInstances()
{
    int32 ParticleCount = Source->ActiveParticles;
    Instances.SetNum(ParticleCount);

    // bUseLocalSpace일 때만 컴포넌트 월드 위치를 더함
    const bool bUseLocalSpace = Source->UseLocalSpace();
    const FVector ComponentWorldLocation = bUseLocalSpace ? Source->GetComponentWorldLocation() : FVector::Zero();

    for (int32 i = 0; i < ParticleCount; ++i)
    {
        FBaseParticle* Particle = Source->GetParticle(i);
        if (!Particle)
            continue;

        // Local Space면 월드로 변환, World Space면 그대로 사용
        FVector WorldPosition = Particle->Location + ComponentWorldLocation;

        // Transform 행렬 생성 (Scale, Rotation, Translation)
        FMatrix ScaleMatrix = FMatrix::MakeScale(Particle->Size);

        // Rotation (Z축 회전)
        FMatrix RotationMatrix = FMatrix::Identity();
        if (Particle->Rotation != 0.0f)
        {
            float CosRot = cosf(Particle->Rotation);
            float SinRot = sinf(Particle->Rotation);
            RotationMatrix.M[0][0] = CosRot;
            RotationMatrix.M[0][1] = -SinRot;
            RotationMatrix.M[1][0] = SinRot;
            RotationMatrix.M[1][1] = CosRot;
        }

        // Translation (월드 좌표 사용)
        FMatrix TranslationMatrix = FMatrix::MakeTranslation(WorldPosition);

        // 최종 변환: Scale * Rotation * Translation
        FMatrix Transform = ScaleMatrix * RotationMatrix * TranslationMatrix;

        Instances[i].Transform = Transform;
        Instances[i].Color = FVector4(Particle->Color.R, Particle->Color.G, Particle->Color.B, Particle->Color.A);
    }
}

void FDynamicMeshEmitterData::Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool)
{
    if (Instances.Num() == 0)
        return;

    if (!BufferPool)
    {
        UE_LOG("[error] BufferPool is null!");
        return;
    }

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

    uint32 MaxInstances = 0;
    uint32 RequiredInstances = Instances.Num();
    ID3D11Buffer* InstanceBuffer = BufferPool->GetOrCreateMeshInstanceBuffer(RequiredInstances, MaxInstances);

    if (!InstanceBuffer)
    {
        UE_LOG("[error] Failed to create mesh instance buffer!");
        return;
    }

    // 3. Map으로 인스턴스 데이터 업로드
    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(InstanceBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Instances.GetData(), sizeof(FParticleMeshInstance) * Instances.Num());
    Context->Unmap(InstanceBuffer, 0);

    // 4. Rasterizer 상태 설정
    RHI->RSSetState(ERasterizerMode::Solid);

    // 5. 블렌딩 상태 설정
    RHI->OMSetBlendState(true);

    // 6. Depth/Stencil 상태 설정
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);

    // 7. 버퍼 바인딩 (Mesh vertex buffer + Instance buffer)
    UINT VertexStride = ParticleMesh->GetVertexStride();
    UINT InstanceStride = sizeof(FParticleMeshInstance);
    UINT Offsets[2] = { 0, 0 };
    ID3D11Buffer* Buffers[2] = { ParticleMesh->GetVertexBuffer(), InstanceBuffer };
    UINT Strides[2] = { VertexStride, InstanceStride };

    Context->IASetVertexBuffers(0, 2, Buffers, Strides, Offsets);
    Context->IASetIndexBuffer(ParticleMesh->GetIndexBuffer(), DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    UINT InstanceCount = Instances.Num();
    ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);

    // 파티클 전용 쉐이더 강제 사용 (머티리얼의 쉐이더 무시)
    static UShader* MeshShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleMesh.hlsl");

    if (TypeDataMesh->bUseMeshMaterials)
    {
        // 메시 머티리얼 사용 (SubMesh별 렌더링)
        // 셰이더는 파티클 머티리얼(ParticleMesh.hlsl) 고정, 텍스처만 메시 머티리얼에서 가져옴
        const TArray<FGroupInfo>& GroupInfos = ParticleMesh->GetMeshGroupInfo();

        // 파티클 셰이더 바인딩 (인스턴싱 + 단일 RT 출력 지원)
        if (MeshShader)
        {
            RHI->PrepareShader(MeshShader);
        }
        else
        {
			UE_LOG("[error] MeshShader is null!");
        }

        for (uint32 i = 0; i < GroupInfos.size(); ++i)
        {
            const FGroupInfo& Group = GroupInfos[i];

            // 해당 SubMesh의 머티리얼에서 텍스처만 가져옴
            UMaterialInterface* MeshMaterial = UResourceManager::GetInstance().Load<UMaterial>(Group.InitialMaterialName);

            ID3D11ShaderResourceView* DiffuseSRV = nullptr;
            if (MeshMaterial)
            {
                if (UTexture* DiffuseTexture = MeshMaterial->GetTexture(EMaterialTextureSlot::Diffuse))
                {
                    DiffuseSRV = DiffuseTexture->GetShaderResourceView();
                }
            }
            Context->PSSetShaderResources(0, 1, &DiffuseSRV);
            Context->PSSetSamplers(0, 1, &DefaultSampler);

            // SubMesh별 Draw
            Context->DrawIndexedInstanced(Group.IndexCount, InstanceCount, Group.StartIndex, 0, 0);
        }
    }
    else
    {
        // 파티클 머티리얼 사용 (기존 방식 - 전체 메시 한번에)
        // 쉐이더는 파티클 전용 쉐이더 강제 사용 (Material과 무관)
        if (MeshShader)
        {
            RHI->PrepareShader(MeshShader);
        }

        // 텍스처 바인딩 (Material이 있으면 Material에서, 없으면 nullptr)
        ID3D11ShaderResourceView* DiffuseSRV = nullptr;
        if (Material)
        {
            if (UTexture* DiffuseTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse))
            {
                DiffuseSRV = DiffuseTexture->GetShaderResourceView();
            }
        }
        Context->PSSetShaderResources(0, 1, &DiffuseSRV);
        Context->PSSetSamplers(0, 1, &DefaultSampler);

        UINT IndexCount = ParticleMesh->GetIndexCount();
        Context->DrawIndexedInstanced(IndexCount, InstanceCount, 0, 0, 0);
    }

    // 10. 렌더 상태 복원
    RHI->RSSetState(ERasterizerMode::Solid);
    RHI->OMSetBlendState(false);
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);
}

// ============================================================
// FDynamicBeamEmitterData 구현
// ============================================================

void FDynamicBeamEmitterData::UpdateRenderData(FSceneView* View)
{
    if (!Source || Source->ActiveParticles == 0)
    {
        Vertices.Empty();
        Indices.Empty();
        return;
    }

    BuildBeamVertices(View);
}

void FDynamicBeamEmitterData::BuildBeamVertices(FSceneView* View)
{
    UParticleLODLevel* LODLevel = Source->CurrentLODLevel;
    if (!LODLevel || !LODLevel->TypeDataModule)
        return;

    UParticleModuleTypeDataBeam* TypeDataBeam = Cast<UParticleModuleTypeDataBeam>(LODLevel->TypeDataModule);
    if (!TypeDataBeam)
        return;

    int32 ParticleCount = Source->ActiveParticles;
    int32 Segments = FMath::Max(1, TypeDataBeam->Segments);
    float Width = TypeDataBeam->Width;
    float Length = TypeDataBeam->Length;
    float TaperFactor = TypeDataBeam->TaperFactor;

    // BeamNoise 모듈에서 노이즈 데이터 가져오기
    UParticleModuleBeamNoise* NoiseModule = nullptr;
    FBeamNoiseCache* NoiseCache = nullptr;
    for (UParticleModule* Module : LODLevel->Modules)
    {
        if (UParticleModuleBeamNoise* BeamNoise = Cast<UParticleModuleBeamNoise>(Module))
        {
            NoiseModule = BeamNoise;
            NoiseCache = BeamNoise->GetNoiseCache(Source);
            break;
        }
    }

    bool bHasNoise = NoiseModule && NoiseModule->HasNoise() && NoiseCache;
    float NoiseLockTime = NoiseModule ? NoiseModule->NoiseLockTime : 0.0f;
    bool bSmooth = NoiseModule ? NoiseModule->bSmooth : false;

    int32 VerticesPerBeam = (Segments + 1) * 2;
    int32 IndicesPerBeam = Segments * 6;

    Vertices.SetNum(ParticleCount * VerticesPerBeam);
    Indices.SetNum(ParticleCount * IndicesPerBeam);

    const bool bUseLocalSpace = Source->UseLocalSpace();
    const FVector ComponentWorldLocation = bUseLocalSpace ? Source->GetComponentWorldLocation() : FVector::Zero();
    const FVector CameraPos = View->ViewLocation;

    for (int32 p = 0; p < ParticleCount; ++p)
    {
        FBaseParticle* Particle = Source->GetParticle(p);
        if (!Particle)
            continue;

        FVector StartPos = Particle->Location + ComponentWorldLocation;

        FVector BeamDir;
        if (Particle->Velocity.SizeSquared() > 0.001f)
        {
            BeamDir = Particle->Velocity.GetSafeNormal();
        }
        else
        {
            BeamDir = FVector(1.0f, 0.0f, 0.0f);
        }
        FVector EndPos = StartPos + BeamDir * Length;

        FLinearColor ParticleColor(Particle->Color.R, Particle->Color.G, Particle->Color.B, Particle->Color.A);

        for (int32 s = 0; s <= Segments; ++s)
        {
            float T = (float)s / (float)Segments;
            FVector SegPos = FMath::Lerp(StartPos, EndPos, T);

            FVector ToCamera = (CameraPos - SegPos).GetSafeNormal();
            FVector Right = FVector::Cross(ToCamera, BeamDir).GetSafeNormal();
            if (Right.SizeSquared() < 0.001f)
            {
                Right = FVector::Cross(FVector(0, 0, 1), BeamDir).GetSafeNormal();
            }
            FVector Up = FVector::Cross(Right, BeamDir).GetSafeNormal();

            // 노이즈 적용 (시작/끝점 제외)
            if (bHasNoise && s > 0 && s < Segments && p < NoiseCache->NoisePoints.Num())
            {
                int32 NoiseIdx = FMath::Min(s - 1, NoiseCache->NoisePoints[p].Num() - 1);
                if (NoiseIdx >= 0 && NoiseIdx < NoiseCache->NoisePoints[p].Num())
                {
                    FVector NoiseOffset;
                    if (bSmooth && NoiseLockTime > 0.0f && p < NoiseCache->NextNoisePoints.Num() && NoiseIdx < NoiseCache->NextNoisePoints[p].Num())
                    {
                        float LerpAlpha = NoiseCache->NoiseTimers[p] / NoiseLockTime;
                        NoiseOffset = FMath::Lerp(NoiseCache->NoisePoints[p][NoiseIdx], NoiseCache->NextNoisePoints[p][NoiseIdx], LerpAlpha);
                    }
                    else
                    {
                        NoiseOffset = NoiseCache->NoisePoints[p][NoiseIdx];
                    }
                    SegPos += Right * NoiseOffset.X + Up * NoiseOffset.Y;
                }
            }

            float TaperScale = 1.0f - (TaperFactor * T);
            float HalfWidth = Width * 0.5f * Particle->Size.X * TaperScale;

            int32 BaseVertex = p * VerticesPerBeam + s * 2;

            Vertices[BaseVertex].Position = SegPos + Right * HalfWidth;
            Vertices[BaseVertex].UV = FVector2D(T, 1.0f);
            Vertices[BaseVertex].Color = ParticleColor;

            Vertices[BaseVertex + 1].Position = SegPos - Right * HalfWidth;
            Vertices[BaseVertex + 1].UV = FVector2D(T, 0.0f);
            Vertices[BaseVertex + 1].Color = ParticleColor;
        }

        for (int32 s = 0; s < Segments; ++s)
        {
            int32 BaseVertex = p * VerticesPerBeam + s * 2;
            int32 BaseIndex = p * IndicesPerBeam + s * 6;

            Indices[BaseIndex + 0] = BaseVertex;
            Indices[BaseIndex + 1] = BaseVertex + 2;
            Indices[BaseIndex + 2] = BaseVertex + 1;

            Indices[BaseIndex + 3] = BaseVertex + 1;
            Indices[BaseIndex + 4] = BaseVertex + 2;
            Indices[BaseIndex + 5] = BaseVertex + 3;
        }
    }
}

void FDynamicBeamEmitterData::Render(D3D11RHI* RHI, FSceneView* View, UMaterialInterface* Material, UParticleDynamicBuffers* BufferPool)
{
    if (Vertices.Num() == 0 || Indices.Num() == 0)
        return;

    if (!BufferPool)
    {
        UE_LOG("[error] BufferPool is null!");
        return;
    }

    ID3D11Device* Device = RHI->GetDevice();
    ID3D11DeviceContext* Context = RHI->GetDeviceContext();

    // 동적 버텍스/인덱스 버퍼 가져오기
    uint32 MaxVertices = 0, MaxIndices = 0;
    ID3D11Buffer* VertexBuffer = BufferPool->GetOrCreateBeamVertexBuffer(Vertices.Num(), MaxVertices);
    ID3D11Buffer* IndexBuffer = BufferPool->GetOrCreateBeamIndexBuffer(Indices.Num(), MaxIndices);

    if (!VertexBuffer || !IndexBuffer)
    {
        UE_LOG("[error] Failed to create beam buffers!");
        return;
    }

    // 버텍스 데이터 업로드
    D3D11_MAPPED_SUBRESOURCE mapped;
    Context->Map(VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Vertices.GetData(), sizeof(FParticleBeamInstance) * Vertices.Num());
    Context->Unmap(VertexBuffer, 0);

    // 인덱스 데이터 업로드
    Context->Map(IndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, Indices.GetData(), sizeof(uint32) * Indices.Num());
    Context->Unmap(IndexBuffer, 0);

    // 렌더 상태 설정
    RHI->RSSetState(ERasterizerMode::Solid_NoCull);
    RHI->OMSetBlendState(true);
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqualReadOnly);

    // 셰이더 바인딩 - 파티클 전용 쉐이더 강제 사용 (Material과 무관)
    static UShader* BeamShader = UResourceManager::GetInstance().Load<UShader>("Shaders/Effects/ParticleBeam.hlsl");
    if (BeamShader)
    {
        RHI->PrepareShader(BeamShader);
    }

    // 텍스처 바인딩 (Material이 있으면 Material에서, 없으면 nullptr)
    ID3D11ShaderResourceView* DiffuseSRV = nullptr;
    if (Material)
    {
        if (UTexture* DiffuseTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse))
        {
            DiffuseSRV = DiffuseTexture->GetShaderResourceView();
        }
    }
    Context->PSSetShaderResources(0, 1, &DiffuseSRV);

    ID3D11SamplerState* DefaultSampler = RHI->GetSamplerState(RHI_Sampler_Index::Default);
    Context->PSSetSamplers(0, 1, &DefaultSampler);

    // 버퍼 바인딩
    UINT Stride = sizeof(FParticleBeamInstance);
    UINT Offset = 0;
    Context->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
    Context->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
    Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Draw
    Context->DrawIndexed(Indices.Num(), 0, 0);

    // 렌더 상태 복원
    RHI->RSSetState(ERasterizerMode::Solid);
    RHI->OMSetBlendState(false);
    RHI->OMSetDepthStencilState(EComparisonFunc::LessEqual);
}
